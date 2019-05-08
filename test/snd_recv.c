// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>

#include "cmd_opts.h"

static bool s_cb_invoked = false;

static void my_cb (const char* buf, const size_t len)
{
  printf ("Callback received msg (len is %lu): %s\n", len, buf);

  s_cb_invoked = true;
}

int main (int argc, char** argv)
{
  const char* peer_ip     = NULL;
  int         peer_port   = 0;
  int         own_port    = 0;
  const char* own_ip      = NULL;

  bool        use_cb      = false;

  if (!get_options (argc, argv, &peer_ip, &peer_port, &own_port, &own_ip))
  {
    printf ("Could not read options");
    exit (1);
  }

  get_use_cb (argc, argv, &use_cb);

  printf ("Own IP is %s, own port is %d\n", own_ip, own_port);
  printf ("Peer IP is %s, peer port is %d\n", peer_ip, peer_port);

  // 1. get a connection handle to another peer
  weeml_t handle = weeml_create_cntn (own_ip, peer_ip, own_port, peer_port, 13);
  if (!handle)
  {
    exit (1);
  }

  if (use_cb)
  { 
    weeml_register_cb (handle, my_cb);
  }

  // 2. send a message
  const char* msg = "Hello from sending peer";
  weeml_send_msg (handle, msg, strlen (msg));

  printf ("Sent initial message\n");

  if (use_cb)
  {
    while (!s_cb_invoked)
    {
      sleep (1);
    }
  }
  else
  {
    // 3. after sending - check if there are any messages in the recv queue so to speak
    int msg_size = weeml_peek (handle);

    while (msg_size < 1)
    {
      printf ("Checking new msgs, msg_size is %d\n", msg_size);
      msg_size = weeml_peek (handle);

      sleep (1);
    } 

    printf ("Recevied new msg notification, size is %d\n", msg_size);

    char buffer [msg_size + 1];
    memset (buffer, '\0', msg_size + 1);

    int rc = weeml_recv_msg (handle, buffer, msg_size);

    printf ("Received msg: %s\n", buffer);

  }

  weeml_release (handle);

  return 0;
}
