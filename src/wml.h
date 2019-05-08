// Copyright QUB 2019

#ifndef WML_wml_hh
#define WML_wml_hh

#include <ikcp.h>
#include <netinet/in.h>
#include <pthread.h>

// this is done to make Wml compatible
// with "struct _weeml_t" forward declaration
struct _weeml_t
{
};

namespace WML
{

typedef void (*user_cb_func)(const char*  buf,
                   const size_t len);

class Wml : public _weeml_t
{
public:
  Wml (const char*     own_ip,
       const char*     peer_ip,
       const int       own_port,
       const int       peer_port,
       const uint32_t  conn_num);

  ~Wml ();

  // returns negative value for error, zero for success
  int      send_msg         (const char*  msg,
                             const size_t msg_len);

  // returns negative value if empty, message len otherwise
  int      peek             ();

  // returns the number of bytes received,
  // negative number for EAGAIN
  int      recv_msg         (char*        buffer,
                             const size_t buf_len);

  void     register_cb      (user_cb_func cb);

  // the below methods are expected to be called by polling thread only
  // whereas the above methods are only invoked by the user thread

  void     update           (); // updates kcp

  uint32_t check            (); // when should be call the next update?

  // returns negative for error, zero for success
  int      input            (const char* data,
                             const long  size); // pass the udp msg to kcp

  void     do_handle_cb     ();

  int      get_fd           () {return m_sock_fd; }

  const sockaddr_in&
           get_peer_address () { return m_peer_address; }

  void     set_peer_address (sockaddr_in& addr)
                               { m_peer_address = addr; }

  void     mark_for_removal () { m_to_be_removed = true; }

  bool     is_to_be_removed () { return m_to_be_removed; }

private:
  ikcpcb*             m_kcp; // the underlying kcp object

  pthread_spinlock_t  m_spinlock; // to manage concurrent access to kcp 
  sockaddr_in         m_peer_address; // needed by sendto function

  int                 m_sock_fd; // our udp socket

  bool                m_to_be_removed;

  user_cb_func        m_cb;

  char*               m_cb_buffer;
  size_t              m_cb_buf_len;

}; // class Wml

} // namespace WML

#endif
