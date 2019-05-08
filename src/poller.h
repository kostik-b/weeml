// Copyright QUB 2019

#ifndef wml_poller_hh
#define wml_poller_hh

#include <list>
#include <vector>
#include <atomic>
#include "the_buffer.h"
#include "wml.h"
#include <poll.h>
#include <pthread.h>

namespace WML
{

class Poller
{
private:
  Poller ();
  ~Poller ();

public:
  static Poller& get_instance ();

  void add_wml    (Wml* w);
  void remove_wml (Wml* w);

  void      run                         ();
private:

  void      check_add_new_wmls          ();
  void      delete_wmls                 ();
  uint32_t  build_timeouts_from_scratch ();
  uint32_t  update_timeouts             ();
  void      check_and_reset_fds         ();
  void      read_from_fd                (Wml* w);

private:
  typedef std::pair<uint32_t, Wml*> TimeoutPair;

  std::list<Wml*>           m_wmls;
  std::vector<pollfd>       m_poll_fds;
  std::vector<TimeoutPair>  m_timeouts;

  TheBuffer                 m_input_buffer;

  bool                      m_delete_wmls;

  bool                      m_rebuild_timeouts;

  pthread_t                 m_thread;

}; // class Poller

} // namespace WML

#endif
