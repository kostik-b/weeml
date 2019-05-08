// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "cmd_opts.h"

int main (int argc, char** argv)
{
  const char* peer_ip     = NULL;
        int   peer_port   = 0;
        int   own_port    = 0;
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

  const size_t  max_size      = 1024*1024*10; // 10Mb
  size_t        current_size  = 8; // start with 1Kb
  char          current_char  = 'a';

  char* msg           = (char*)malloc (max_size); // a 10Mb message max
  char* resp_buffer   = (char*)malloc (max_size); // a 10Mb message max
  memset (msg,         '\0', max_size);
  memset (resp_buffer, '\0', max_size);

  while (current_size < max_size)
  {
    memset (msg, current_char, current_size);
    
    int rc = weeml_send_msg (handle, msg, current_size);

    if (rc > -1)
    {
      printf ("Sent message, size is %lu, rc is %d\n", current_size, rc);
    }
    else
    {
      printf ("Failed to send message, rc is %d\n", rc);

      break;
    }

    // 3. after sending - check if there are any messages in the recv queue
    int reply_size = weeml_peek (handle);

    while (reply_size < 1)
    {
      //printf ("Checking new msgs, msg_size is %d\n", msg_size);
      reply_size = weeml_peek (handle);

      usleep (1000*10); // 1ms
    } 

    printf ("Recevied new msg notification, size is %d\n", reply_size);

    rc = weeml_recv_msg (handle, resp_buffer, max_size);

    printf ("Recv size is %d\n", rc);

    if (rc != current_size)
    {
      printf ("The return size is not the same as was sent\n");
      break;
    }

    bool tripped = false;

    for (int i = 0; i < reply_size; ++i)
    {
      if (resp_buffer[i] != current_char)
      {
        printf ("That's not the message I received!!!\n");
        tripped = true;
        break;
      }
    }

    if (!tripped)
    {
      printf ("The contents seem to be OK\n");
    }

    printf ("----------\n");

    // the increment bit
    current_size = current_size << 1;

    ++current_char;
    if (current_char > 'z')
    {
      current_char = 'a'; // wrap around
    }

  } // while ...

  weeml_release (handle);

  free (msg);

  return 0;
}
