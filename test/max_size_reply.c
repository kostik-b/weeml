// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    printf ("No handle was created\n");
    exit (1);
  }

  const size_t  buf_size = 1024*1024*10; // 10Mb
  char*   buffer      = (char*)malloc (buf_size); // a 10Mb message max
  memset (buffer, '\0', buf_size);

  while (1)
  {
    int msg_size = weeml_peek (handle);

    while (msg_size < 1)
    {
      // printf ("Checking new msgs\n");
      msg_size = weeml_peek (handle);

      usleep (10*1000);
    } 

    printf ("Received new msg notification, size is %d\n", msg_size);

    int rc = weeml_recv_msg (handle, buffer, buf_size);

    if (rc > 0)
    {
      printf ("Received msg, rc is %d\n", rc);

      weeml_send_msg (handle, buffer, rc);

      printf ("Reply sent\n");
      printf ("-----------\n");
    }
    else
    {
      printf ("Could not receive message, rc is %d\n", rc);
    }

  }

  weeml_release (handle);

  return 0;
}
