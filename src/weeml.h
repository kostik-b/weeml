// Copyright QUB 2019

#ifndef weeml_weeml_h
#define weeml_weeml_h

#include <stdlib.h>
#include <inttypes.h>

/*
  The goal of this library is to enable the sending of simple point-to-point messages
  without having to worry about the intricancies of UDP/TCP etc. It is meant
  to be as simple to use as possible.

  Internally, there is 1 thread that serves the needs of the kcp implementation.
  The callbacks are invoked on that thread, so please, make sure you do
  not keep it busy for too long. All of the weeml connections are still served
  by that one thread.

  This interface is meant to be expanded. First, there is a simple p2p
  connection. Second, there might be a listening socket-like interface.
*/

#ifdef __cplusplus
extern "C" {
#endif

// forward declaration
struct _weeml_t;
typedef struct _weeml_t* weeml_t;

// get connection handle.
// if own_port is 0, then bind is not called
// if own_port is not 0, but own_ip is NULL, 
// the sockaddr's IP is set to INADDR_ANY
weeml_t   weeml_create_cntn         (const char*     own_ip,
                                     const char*     peer_ip,
                                     const int       own_port,
                                     const int       peer_port,
                                     const uint32_t  conn_num);

// returns negative value for error, zero for success
int      weeml_send_msg            (weeml_t handle, const char*  msg,
                                                    const size_t msg_len);

// returns negative value if empty, message len otherwise
int      weeml_peek                 (weeml_t handle);

// returns the number of bytes received,
// negative number for EAGAIN
int      weeml_recv_msg             (weeml_t handle, char*        buffer,
                                                     const size_t buf_len);

void     weeml_register_cb          (weeml_t handle, 
                                     void (*cb)(const char*  buf,
                                                const size_t len));

void     weeml_release              (weeml_t handle);

#ifdef __cplusplus
}
#endif

#endif
