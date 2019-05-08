// Copyright QUB 2018

#ifndef the_buffer_class_hh
#define the_buffer_class_hh

#include <mutex>

#include "wml.h"
#include "circ_array.hpp"

namespace WML
{

// This is a thread-safe circular buffer class
class TheBuffer
{
public:
  TheBuffer ();
  //static TheBuffer& get_instance ();

  void      push_back (Wml* w);

  Wml*      pop_front ();

  bool      is_full   ();

  bool      is_empty  ();

  size_t    occupancy () {return m_ring_buffer.occupancy ();}

private:
  TheBuffer (const TheBuffer& copy) = delete;
  TheBuffer& operator=(const TheBuffer& rhs) = delete;

  typedef CircArray<Wml*> RingBuffer;
  RingBuffer                 m_ring_buffer;

  std::mutex            m_mutex;
}; // class TheBuffer

} // namespace WML

#endif
