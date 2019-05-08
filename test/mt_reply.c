// Copyright QUB 2019

#include <weeml.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "cmd_opts.h"

static const char* s_own_ip        = NULL;
static int         s_own_base_port = 0;

void* thread_run (void* param)
{
  // establish unique conv_id
  // sleep
  // create a wml <- get peer params - thread number - offset from the base
  // in a loop:
  // wait for a message, send it back

  const int   tid     = *((int*)param);
  const int   conv_id = 13 + tid;
  const int   port    = s_own_base_port + tid;

  printf ("TID %d: Thread started conv_id %d, port %d\n", tid, conv_id, port);

  // get a connection handle to another peer
  weeml_t handle = weeml_create_cntn (s_own_ip, NULL, port, 0, conv_id);
  if (!handle)
  {
    printf ("TID %d: No handle was created\n", tid);
    exit (1);
  }

  const size_t  buf_size = 1024*1024*3; // 3Mb
  char*   buffer      = (char*)malloc (buf_size); // a 3Mb message max
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

    printf ("TID %d: Received new msg notification, size is %d\n", tid, msg_size);

    int rc = weeml_recv_msg (handle, buffer, buf_size);

    if (rc > 0)
    {
      printf ("TID %d: Received msg, rc is %d\n", tid, rc);

      weeml_send_msg (handle, buffer, rc);

      printf ("TID %d: Reply sent\n", tid);
      printf ("TID %d: -----------\n", tid);
    }
    else
    {
      printf ("TID %d: Could not receive message, rc is %d\n", tid, rc);
    }
  }

  weeml_release (handle);

  free (buffer);
}

int main (int argc, char** argv)
{
  // these are ignored
  const char* peer_ip     = NULL;
  int         peer_port   = 0;

  if (!get_options (argc, argv, &peer_ip, &peer_port, &s_own_base_port, &s_own_ip))
  {
    printf ("Could not read options");
    exit (1);
  }
 
  printf ("Own IP is %s, own base port is %d\n", s_own_ip, s_own_base_port);

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
