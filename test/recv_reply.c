// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>

#include "cmd_opts.h"

int main (int argc, char** argv)
{
  const char* peer_ip     = NULL;
  int         peer_port   = 0;
  int         own_port    = 0;
  const char* own_ip      = NULL;

  if (!get_options (argc, argv, &peer_ip, &peer_port, &own_port, &own_ip))
  {
    printf ("Could not read options");
    exit (1);
  }

  printf ("Own IP is %s, own port is %d\n", own_ip, own_port);
  printf ("Peer IP is %s, peer port is %d\n", peer_ip, peer_port);

  // 1. get a connection handle to another peer
  weeml_t handle = weeml_create_cntn (own_ip, peer_ip, own_port, peer_port, 13);
  if (!handle)
  {
    exit (1);
  }
  // 3. after sending - check if there are any messages in the recv queue so to speak
  int msg_size = weeml_peek (handle);

  printf ("initial weeml_peek returned %d\n", msg_size);

  while (msg_size < 1)
  {
    printf ("Checking new msgs\n");
    msg_size = weeml_peek (handle);

    sleep (1);
  } 

  printf ("Recevied new msg notification, size is %d\n", msg_size);

  char buffer [msg_size + 1];
  memset (buffer, '\0', msg_size + 1);

  int rc = weeml_recv_msg (handle, buffer, msg_size);

  printf ("Received msg: %s\n", buffer);

  char reply_buffer[256];

  sprintf (reply_buffer, "Hello friend, this is the message I received from you: %s", buffer);

  weeml_send_msg (handle, reply_buffer, strlen (reply_buffer));

  printf ("Reply sent, closing connection\n");

  weeml_release (handle);

  return 0;
}
