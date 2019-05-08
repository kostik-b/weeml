// Copyright QUB 2019

#include "poller.h"
#include "wml_utils.h"
#include <unistd.h>


static const uint32_t EPSILON  = 10; // that is 0.01 secs

static void* start_run (void* param)
{
  WML::Poller* p = reinterpret_cast<WML::Poller*>(param);

  p->run ();
}

WML::Poller::Poller ()
{
  m_delete_wmls       = false;
  m_rebuild_timeouts  = true;

  // start the poller's thread here
  // TODO: use c++11 thread?
  pthread_create (&m_thread, NULL, start_run, this);

  WDBG ("Created Poller instance.\n");
}

WML::Poller::~Poller ()
{
  for (auto iter = m_wmls.begin (); iter != m_wmls.end (); ++iter)
  {
    (*iter)->mark_for_removal ();
  }

  delete_wmls ();
}

// called by the user thread
void WML::Poller::add_wml    (Wml* w)
{
  m_input_buffer.push_back (w);

  WDBG ("Poller: pushed back new wml, peer port is %d.\n",
        ntohs(w->get_peer_address().sin_port));
}

inline void WML::Poller::check_add_new_wmls ()
{
  if (m_input_buffer.is_empty ())
  {
    return;
  }

  Wml* w = m_input_buffer.pop_front ();

  if (!w)
  {
    fprintf (stderr, "ERROR: WML - new wml is empty!\n");
    return;
  }

  m_wmls.push_back (w);
  struct pollfd pfd {w->get_fd (), POLLIN, 0};
  m_poll_fds.push_back (pfd);

  m_timeouts.resize (m_poll_fds.size());

  m_rebuild_timeouts = true;

  WDBG ("Poller: added new wml (peer port %d). Timeouts size is %lu, "
          "pollfds size is %lu, conns size is %lu\n",
              ntohs(w->get_peer_address().sin_port),
              m_timeouts.size (), m_poll_fds.size(),
                m_wmls.size ());
}

/*
  TODO: deleting element in the middle of m_poll_fds is ineffecient. How to fix this:
        move (by swapping) all pollfds to the end of the vector (do the same for m_wmls)
        Then could start iterating from the end and just delete the elements at the end.
*/
void WML::Poller::delete_wmls ()
{
  m_delete_wmls = false;

  // prior read/writes stay where there are
  // following writes stay where they are
  std::atomic_thread_fence (std::memory_order_release);

  auto iter_w       = m_wmls.begin ();
  auto iter_w_end   = m_wmls.end ();

  auto iter_fd      = m_poll_fds.begin ();
  auto iter_fd_end  = m_poll_fds.end ();

  for ( ; (iter_w != iter_w_end) && (iter_fd != iter_fd_end);
                ++iter_w, ++iter_fd)
  {
    if ((*iter_w)->is_to_be_removed ())
    {
      WDBG ("Poller: deleting wml (peer port %d)\n", ntohs((*iter_w)->get_peer_address().sin_port));
      delete (*iter_w); // this will release the kcp object and close the socket
      iter_w  = m_wmls.erase (iter_w);
      iter_fd = m_poll_fds.erase (iter_fd);
    }
  }

  m_rebuild_timeouts = true;
}

// called by the user thread
void WML::Poller::remove_wml (Wml* w)
{
  // we can't actually remove the wml right
  // here as Poller's thread can be using it
  // so we mark it for removal and let Poller's thread do it
  if (w)
  {
    w->mark_for_removal ();
  }

  // prior read/writes stay where there are
  // following writes stay where they are
  std::atomic_thread_fence (std::memory_order_release);

  m_delete_wmls = true;
}

uint32_t WML::Poller::build_timeouts_from_scratch ()
{
  // iterate over the wmls
  // for each wml call check, if it is less than .01 secs away
  // call update immediately, then check again, then update the timer and move
  // onto the next item
  // all the while calculating the min next interval

  m_timeouts.resize (m_poll_fds.size ()); // just in case, this was already done before
  m_rebuild_timeouts = false;

  uint32_t       now      = iclock ();
  uint32_t       next_min = now + 1000; // that is 1 sec
  auto           iter_end = m_wmls.end ();

  WDBG ("--- Rebuild from scratch\n");

  int i = 0;
  for (auto iter = m_wmls.begin (); iter != iter_end; ++iter, ++i)
  {
    WDBG ("---- NOW is %u\n", now);
    uint32_t next_update = (*iter)->check ();
    if (now + EPSILON < next_update)
    {
      m_timeouts[i].first   = next_update;
      m_timeouts[i].second  = *iter;
      // update the next min time to wait
      next_min = (next_update < next_min) ? next_update : next_min;
      WDBG ("---- NU(C) is %u\n", next_update);
    }
    else
    {
      // need to update it now
      (*iter)->update ();
      // just assign the time regardless
      next_update = (*iter)->check ();
      m_timeouts[i].first   = next_update;
      m_timeouts[i].second  = *iter;
      // update the next min time to wait
      next_min = (next_update < next_min) ? next_update : next_min;
      // "update" may have taken a while
      // need to update the current time
      now = iclock ();
      WDBG ("---- NU(U) is %u\n", next_update);
    }
  }

  return next_min;
}

uint32_t WML::Poller::update_timeouts ()
{
  // iterate over timeouts
  // if a wml is less than a .01 away, call update, then check,
  // all the while keeping track of the min next interval
  uint32_t       now      = iclock ();
  uint32_t       next_min = now + 1000; // that is 1 sec

  WDBG ("--- Update timeouts\n");

  for (int i = 0; i < m_timeouts.size (); ++i)
  {
    WDBG ("---- NOW is %u\n", now);
    uint32_t next_update = m_timeouts[i].first;
    if (now + EPSILON < next_update)
    {
      // update the next min time to wait
      next_min = (next_update < next_min) ? next_update : next_min;
      WDBG ("---- NU(C) is %u\n", next_update);
    }
    else
    {
      // need to call an update
      Wml* w = m_timeouts[i].second;
      w->update ();
      next_update = w->check ();
      // assign the value regardless
      m_timeouts[i].first = next_update;
      // update the next min time to wait
      next_min = (next_update < next_min) ? next_update : next_min;
      // "update" may have taken a while
      // need to update the current time
      now = iclock ();
      WDBG ("---- NU(U) is %u\n", next_update);
    }
  }

  return next_min;
}

void WML::Poller::run ()
{
  while (true) // main loop it is
  {
    // 0. do we have to add any new wmls or remove the old ones?
    if (m_delete_wmls == true)
    {
      delete_wmls ();
    }
    check_add_new_wmls ();

    // if there are no wmls to service, we just sleep
    // TODO: use std::condition_variable to wait here
    if (m_poll_fds.empty ())
    {
      sleep (1);
      continue;
    }
    
    // 1. recalculate timeouts
    uint32_t next_check_time = 0;
    if (m_rebuild_timeouts)
    {
      next_check_time = build_timeouts_from_scratch ();
    }
    else
    {
      next_check_time = update_timeouts ();
    }

    // 2. poll on all the fds with that waiting time
    uint32_t timeout = next_check_time - iclock ();

    WDBG ("-- Timeout is %u.\n", timeout);

    if ((timeout < 0) || (timeout > 1000/*1 sec*/))
    {
      timeout = 0; // return immediately
    }
    int prc = poll (m_poll_fds.data (), m_poll_fds.size (), timeout);
    // 3. >0 -> some data/event ---- read it now
    //     0 -> timeout ------------ this will be handled in step 1 of next loop
    //    <0 -> some error --------- TODO: what do we do here?
    if (prc > 0) // we've got some events
    {
      check_and_reset_fds ();
    }
    else if (prc < 0)
    {
      // TODO: some error - what do we do???
    }
  } // while (true)
}

void WML::Poller::check_and_reset_fds ()
{
  for (int i = 0; i < m_poll_fds.size (); ++i)
  {
    short rev = m_poll_fds[i].revents;
    if (rev == POLLIN)
    {
      // read from the socket and call input on Wml
      Wml* w = m_timeouts[i].second;
      read_from_fd (w);
      m_poll_fds[i].revents = 0; // reset the revents
    }
    else if ((rev == POLLERR) || (rev = POLLHUP) || (rev == POLLNVAL))
    {
      // TODO: how do we handle errors here?
    }
    else
    {
      fprintf (stderr, "POLLER: unknown event (%d) returned for fd (%d)\n", rev,
                    m_poll_fds[i].fd);
    }
  }
}

void WML::Poller::read_from_fd (Wml* w)
{
  static const size_t  BUFFER_SIZE  = 65536 ; // max udp size
  static char*         buffer       = (char*)malloc (BUFFER_SIZE);

  // TODO: write a wrapper for malloc that would throw an exception
  // if the allocation failed

  int rc = 0;
  // TODO: in the future the connection will be primed with a test message and the peer
  // address will be set there once and for all
  sockaddr_in peer_addr;

  if (w->get_peer_address().sin_port == 0) // i.e. no peer address is set
  {
    socklen_t peer_len = sizeof (peer_addr);
    rc = recvfrom(w->get_fd(), buffer, BUFFER_SIZE, // this is the max data size
                      MSG_DONTWAIT, (struct sockaddr *)&peer_addr, &peer_len);
  }
  else
  {
    rc = recvfrom(w->get_fd(), buffer, BUFFER_SIZE, // this is the max data size
                      MSG_DONTWAIT, NULL, NULL);
  }

  if (rc == 0) // the remote connection has been closed
  {
    fprintf (stderr, "ERROR: on_fd - recvfrom returned 0\n");
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
      fprintf (stderr, "ERROR: on_fd recvfrom returned some error: %d\n", errno);
    }
    return;
  }

  // set peer address
  if (w->get_peer_address().sin_port == 0)
  {
    w->set_peer_address (peer_addr);
  }

  // call input on wml, this will in effect pass the data
  // onto kcp and will invoke the user's callback function if need be

  rc = w->input (buffer, rc);
  if (rc < 0)
  {
    fprintf (stderr, "POLLER: an error (%d) calling Wml::input\n", rc);
  }
}

WML::Poller& WML::Poller::get_instance ()
{
  static Poller* poller = new Poller;
  return *poller;
}
