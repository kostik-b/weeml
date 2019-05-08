// Copyright QUB 2019

#include "the_buffer.h"

WML::TheBuffer::TheBuffer ()
  : m_ring_buffer (100) // an arbitrary value
{
}
#if 0
TheBuffer& TheBuffer::get_instance ()
{
  static TheBuffer* instance = new TheBuffer;

  return *instance;
}
#endif
void WML::TheBuffer::push_back (Wml* w)
{
  std::lock_guard<std::mutex> lock (m_mutex);

  m_ring_buffer.push_back (w);
}

WML::Wml* WML::TheBuffer::pop_front ()
{
  std::lock_guard<std::mutex> lock (m_mutex);

  if (m_ring_buffer.occupancy() != 0)
  {
    return m_ring_buffer.pop_front ();
  }
  else
  {
    return NULL;
  }
}

bool WML::TheBuffer::is_full   ()
{
  std::lock_guard<std::mutex> lock (m_mutex);

  return m_ring_buffer.is_full ();
}

bool WML::TheBuffer::is_empty   ()
{
  std::lock_guard<std::mutex> lock (m_mutex);

  return m_ring_buffer.empty ();
}
