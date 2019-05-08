// Copyright QUB 2019

#include "weeml.h"
#include "wml.h"
#include "poller.h"

#if 0
static void on_fd_cb (struct ev_loop* loop, ev_io* w, int revents)
{
  // first check if there is a msg at all (by using MSG_PEEK)

  //WDBG("--On FD CB.\n");
  // the below will only be called by libev thread - hence no locking necessary
  static const size_t  BUFFER_SIZE  = 65536 ; // max udp size
  static char*         buffer       = (char*)malloc (BUFFER_SIZE);

  int rc = recvfrom(w->fd, buffer, BUFFER_SIZE, // this is the max data size
                    MSG_DONTWAIT, NULL, NULL);

  if (rc == 0) // the remote connection has been closed
  {
    printf ("ERROR: on_fd - recvfrom returned 0\n");
    return; // not sure what to do yet
  }

  if (rc < 0)
  {
    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
    {
      // all good, just return later
    }
    else
    {
      printf ("ERROR: on_fd recvfrom returned some error: %d\n", errno);
    }
    return;
  }

  weeml_t handle = (weeml_t)w->data;

  WDBG("--Passing the packet to KCP.\n");

  SPIN_LOCK

  int ic = ikcp_input (handle->m_kcp, buffer, rc);

  SPIN_UNLOCK

  WDBG("--Input RC is %d\n", ic);

  if (handle->m_cb)
  {
    do_handle_cb (handle);
  }
}

static void on_timer_cb (struct ev_loop* loop, ev_timer* w, int revents)
{
  weeml_t handle = (weeml_t)w->data;

  // WDBG ("--OnTimer: iclock is %d\n", iclock ());

  SPIN_LOCK

  ikcp_update (handle->m_kcp, iclock ());

  uint32_t check_interval = ikcp_check (handle->m_kcp, iclock ());

  SPIN_UNLOCK

  double next_interval = double(check_interval - iclock ()) / 1000.0f;

  // since we are using 32-bit integer, the iclock will eventually wrap around
  if ((next_interval > 1.0f) || (next_interval < 0.0f))
  {
    next_interval = 0.1f;
  }

  // WDBG("--OnTimer: check_interval is %d, timer is scheduled for %4.4f\n",
  //           check_interval, next_interval);

  w->repeat = next_interval;

  ev_timer_again (loop, w);
}

#endif

#define WCAST(handle) static_cast<Wml*>(handle)

#ifdef __cplusplus
extern "C" {
#endif

using namespace WML;

weeml_t weeml_create_cntn (
  const char*     own_ip,
  const char*     peer_ip,
  const int       own_port,
  const int       peer_port,
  const uint32_t  conn_num)
{
  // create the Wml object and return its pointer
  Wml* w = NULL;

  try
  {
    w = new Wml (own_ip, peer_ip, own_port, peer_port, conn_num);
  }
  catch (std::runtime_error& err)
  {
    fprintf (stderr, "Could not create a WML object, error is %s\n", err.what ());
    return NULL;
  }

  Poller::get_instance ().add_wml (w);

  return w;
}

int weeml_send_msg (
  weeml_t       handle,
  const char*   msg,
  const size_t  msg_len)
{
  // first some sanity checks
  if (handle == NULL)
  {
    return -1;
  }

  return WCAST(handle)->send_msg (msg, msg_len);
}

int weeml_peek (weeml_t handle)
{
  if (handle == NULL)
  {
    return -1;
  }

  return WCAST(handle)->peek ();
}

int  weeml_recv_msg (
  weeml_t       handle,
  char*         buffer,
  const size_t  buf_len)
{
  if (handle == NULL)
  {
    return -1;
  }

  return WCAST(handle)->recv_msg (buffer, buf_len);
}

void weeml_register_cb         
  (weeml_t      handle,
   user_cb_func cb)
{
  if (handle == NULL)
  {
    return;
  }

  return WCAST(handle)->register_cb (cb);
}

void weeml_release (weeml_t handle)
{
  if (handle == NULL)
  {
    return;
  }

  // call poller and instruct it to destruct
  // the Wml object
  WCAST(handle)->register_cb (NULL);

  Poller::get_instance ().remove_wml (WCAST(handle));
}

#ifdef __cplusplus
}
#endif

