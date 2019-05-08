// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "cmd_opts.h"

static const char* s_peer_ip        = NULL;
static int         s_peer_base_port = 0;

void* thread_run (void* param)
{
  // generate a random number to sleep before starting - up to 20 seconds.
  // establish a unique character and unique conv_id
  // sleep
  // create a wml <- get peer params - thread number - offset from the base
  // in a loop:
  // - send messages up to 2 MB
  // - wait for reply
  // - sleep up to 3 secs, start again

  const int   tid     = *((int*)param);
  const int   conv_id = 13 + tid;
  const char  letter  = 'A' + tid; // NB: be careful!
  const int   port    = s_peer_base_port + tid;

  srandom (tid);

  const int initial_sleep = random () % 20;

  printf ("Thread %d, conv_id %d, port %d, letter %c, initial sleep %d secs\n",
            tid, conv_id, port, letter, initial_sleep);

  sleep (initial_sleep);
  // get a connection handle to another peer
  weeml_t handle = weeml_create_cntn (NULL, s_peer_ip, 0, port, conv_id);
  if (!handle)
  {
    printf ("TID %d no handle was created\n", tid);
    exit (1);
  }

  const size_t  max_size      = 1024*1024*3; // 3Mb
  size_t        current_size  = 8;

  char* msg           = (char*)malloc (max_size); // a 10Mb message max
  char* resp_buffer   = (char*)malloc (max_size); // a 10Mb message max
  memset (msg,         '\0', max_size);
  memset (resp_buffer, '\0', max_size);

  while (current_size < max_size)
  {
    memset (msg, letter, current_size);
    
    int rc = weeml_send_msg (handle, msg, current_size);

    if (rc > -1)
    {
      printf ("TID %d: Sent message, size is %lu, rc is %d\n", tid, current_size, rc);
    }
    else
    {
      printf ("TID %d: Failed to send message, rc is %d\n", tid, rc);

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

    printf ("TID %d: Recevied new msg notification, size is %d\n", tid, reply_size);

    rc = weeml_recv_msg (handle, resp_buffer, max_size);

    printf ("TID %d: Recv size is %d\n", tid, rc);

    if (rc != current_size)
    {
      printf ("TID %d: The return size is not the same as was sent\n", tid);
      break;
    }

    bool tripped = false;

    for (int i = 0; i < reply_size; ++i)
    {
      if (resp_buffer[i] != letter)
      {
        printf ("TID %d: That's not the message I received!!!\n", tid);
        tripped = true;
        break;
      }
    }

    if (!tripped)
    {
      printf ("TID %d: The contents seem to be OK\n", tid);
    }

    printf ("TID %d, ----------\n", tid);

    // the increment bit
    current_size = current_size << 1;

    int short_sleep = random () % 3;
    sleep (short_sleep);
  } // while ...

  weeml_release (handle);

  free (msg);
  free (resp_buffer);
}

int main (int argc, char** argv)
{
  // these are ignored
  int         own_port    = 0;
  const char* own_ip      = NULL;


  if (!get_options (argc, argv, &s_peer_ip, &s_peer_base_port, &own_port, &own_ip))
  {
    printf ("Could not read options");
    exit (1);
  }
 
  printf ("Peer IP is %s, peer base port is %d\n", s_peer_ip, s_peer_base_port);

  const size_t max_threads = 20;

  pthread_t threads[max_threads];

  for (int i = 0; i < max_threads; ++i)
  {
    int* tid_p = (int*)malloc(sizeof(int));
    *tid_p = i;

    pthread_create (&threads[i], NULL, thread_run, tid_p);
  }

  for (int i = 0; i < max_threads; ++i)
  {
    pthread_join (threads[i], NULL);
  }

  printf ("All threads finished\n");

  return 0;
}
