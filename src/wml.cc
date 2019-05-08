// Copyright QUB 2019

#include <wml.h>
#include <ikcp.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <unistd.h>
#include "wml_utils.h"

#include <stdio.h>
#include <cstring>
#include <stdexcept>

#define SPIN_LOCK   pthread_spin_lock (&m_spinlock); 
#define SPIN_UNLOCK pthread_spin_unlock (&m_spinlock);

#ifdef __cplusplus
extern "C" {
#endif

static void kcp_writelog(const char *log, struct IKCPCB *kcp, void *user)
{
  printf ("[KCP]: %s\n", log);
}

static int send_udp_msg (
  const char* buf,
  int         len,
  ikcpcb*     kcp,
  void*       user)
{
  WML::Wml* handle = reinterpret_cast<WML::Wml*>(user);

  const sockaddr_in& peer_address = handle->get_peer_address ();

  int rc = sendto(handle->get_fd (), buf, len,  
            MSG_CONFIRM, (const struct sockaddr *) &peer_address,
            sizeof (peer_address));

  if (rc < 1)
  {
    fprintf (stderr, "ERROR: sending a UDB msg - sendto returned: %d\n", rc);
  }
}

#ifdef __cplusplus
}
#endif

static bool init_own_socket (
  const char* own_ip,
  const int   own_port,
  int&        sockfd) // a return value
{
  // Create datagram socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
  { 
    fprintf (stderr, "ERROR: socket creation failed\n");
    return false; 
  } 

  // only set the port if it is supplied
  if (own_port > 0)
  {
    struct sockaddr_in own_address;
    memset(&own_address, 0, sizeof(own_address)); 
      
    own_address.sin_family       = AF_INET; // IPv4 
    if (own_ip)
    {
      inet_pton(AF_INET, own_ip, &(own_address.sin_addr));
    }
    else
    {
      own_address.sin_addr.s_addr  = INADDR_ANY;
    }
    own_address.sin_port         = htons(own_port); 
      
    // bind the socket
    if (bind(sockfd, (const struct sockaddr *)&own_address, sizeof(own_address)) < 0 ) 
    { 
      fprintf(stderr, "ERROR: bind failed"); 
      close (sockfd);
      return false; 
    }

  }
/* FIXME: blocking or not?

  // set the socket to be non blocking
  int flags = 0;
  if ((flags = fcntl (sockfd, F_GETFL, 0)) < 0)
  {
    printf("fcntl failed"); 
    close (sockfd);
    return false; 
  }

  if (fcntl (sockfd, F_SETFL, flags|O_NONBLOCK) < 0)
  {
    printf("fcntl failed"); 
    close (sockfd);
    return false; 
  }
*/
  return true;
}

void WML::Wml::do_handle_cb ()
{
  WDBG("--Attempting to do a user callback...\n");

  // first, peek to see if there is a new msg available

  int rc = ikcp_peeksize (m_kcp);

  if (rc < 1)
  {
    WDBG("--No, no data yet\n");
    return;
  }
  // adjust the buffer size
  if (rc > m_cb_buf_len)
  {
    m_cb_buffer    = (char*)realloc (m_cb_buffer, rc);
    m_cb_buf_len   = rc;
  }

  WDBG("--We have a message of %d bytes to invoke user callback for\n", rc);

  // second read it and return to the user
  rc = ikcp_recv(m_kcp, m_cb_buffer, m_cb_buf_len);

  if ((rc > m_cb_buf_len) || (rc < 1)) // just one last sanity check
  {
    return;
  }

  (*m_cb)(m_cb_buffer, rc);
}

WML::Wml::Wml (
  const char*     own_ip,
  const char*     peer_ip,
  const int       own_port,
  const int       peer_port,
  const uint32_t  conn_num)
{
  // this object will be accessed by both user and Poller's thread
  pthread_spin_init(&m_spinlock, PTHREAD_PROCESS_PRIVATE);

  WDBG ("-Creating weeml...\n");

  if ((peer_port < 1) && (own_port < 1))
  {
    fprintf (stderr, "ERROR: no own or peer port supplied\n");
    throw std::runtime_error ("No own or peer port supplied");
  }

  // now convert the peer address struct
  memset(&(m_peer_address), 0, sizeof(m_peer_address)); 

  if (peer_ip && peer_port)
  {
    m_peer_address.sin_family = AF_INET; // IPv4 
    inet_pton(AF_INET, peer_ip, &(m_peer_address.sin_addr));
    m_peer_address.sin_port   = htons(peer_port); 
  } // else this will be set later upon the arrival of the first message

  // now setup own socket
  if (!init_own_socket (own_ip, own_port, m_sock_fd))
  {
    fprintf (stderr, "ERROR: Could not set up own socket\n");
    throw std::runtime_error ("Could not set up own socket");
  }

  WDBG ("--Created socket, peer ip is %s, peer port is %d\n"
        "  own ip is %s, own port is %d\n", peer_ip, peer_port, own_ip, own_port);

  // create an actual kcp object
  ikcpcb* kcp = ikcp_create (conn_num, this);

  if (!kcp)
  {
    fprintf (stderr, "ERROR: Could not allocate kcpcb object\n");
    close (m_sock_fd);
    throw std::runtime_error ("Could not allocate kcp object");
  }

  // kcp will call this, when sending a UDP message
  ikcp_setoutput  (kcp, send_udp_msg);
  // ikcp_wndsize    (kcp, 256, 256); // max number of fragments is 256
  ikcp_setmtu     (kcp, 65507);// the kcp segment size - the size of the actual send buffer
                               //  (should ideally fit into 1 udp packet)

  //kcp->writelog = kcp_writelog;
  //kcp->logmask = 0xffffffff;

  m_kcp = kcp;
  m_cb  = NULL;

  m_to_be_removed = false;

  m_cb_buf_len  = 1024;
  m_cb_buffer   = (char*)malloc (m_cb_buf_len);
  if (!m_cb_buffer)
  {
    m_cb_buf_len = 0;
  }
}

WML::Wml::~Wml ()
{
  ikcp_release (m_kcp);

  close (m_sock_fd);

  if (m_cb_buffer)
  {
    free (m_cb_buffer);
  }
}

int WML::Wml::send_msg (
  const char*  msg,
  const size_t msg_len)
{
  if ((!msg) || (msg_len < 1))
  {
    return -1;
  }
  // acquire lock and send
  SPIN_LOCK

  int rc = ikcp_send(m_kcp, msg, msg_len); 

  ikcp_flush (m_kcp);

  SPIN_UNLOCK

  return rc;
}

int WML::Wml::peek ()
{
  SPIN_LOCK

  int rc = ikcp_peeksize (m_kcp);

  SPIN_UNLOCK

  return rc;
}

int WML::Wml::recv_msg (
  char*        buffer,
  const size_t buf_len)
{
  SPIN_LOCK

  int rc = ikcp_recv(m_kcp, buffer, buf_len);

  SPIN_UNLOCK

  return rc;
}

int WML::Wml::input (const char* data, const long size)
{
  SPIN_LOCK

  int rc = ikcp_input (m_kcp, data, size);

  if (m_cb)
  {
    do_handle_cb ();
  }
 
  SPIN_UNLOCK

  return rc;
}

void WML::Wml::register_cb (user_cb_func cb)
{
  SPIN_LOCK

  m_cb = cb;

  SPIN_UNLOCK
}

uint32_t WML::Wml::check ()
{
  SPIN_LOCK
  uint32_t check_time = ikcp_check (m_kcp, iclock ());
  SPIN_UNLOCK

  return check_time;
}

void WML::Wml::update ()
{
  SPIN_LOCK
  ikcp_update (m_kcp, iclock ());
  SPIN_UNLOCK
}
