// Copyright QUB 2019

#ifndef WML_WML_UtilsHH
#define WML_WML_UtilsHH

#include <sys/time.h>

#ifdef WEEML_DBG
  #define WDBG(...) printf (__VA_ARGS__);
#else
  #define WDBG(...)
#endif

namespace WML
{
/* get system time */
// **taken from kcp example**
static inline void itimeofday(long *sec, long *usec)
{
  struct timeval time;
  gettimeofday(&time, NULL);
  if (sec) *sec = time.tv_sec;
  if (usec) *usec = time.tv_usec;
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
  long s, u;
  s = u = 0;
  IINT64 value = 0;
  itimeofday(&s, &u);
  value = ((IINT64)s) * 1000 + (u / 1000);
  return value;
}

static inline IUINT32 iclock()
{
  return (IUINT32)(iclock64() & 0xfffffffful);
}

} // namespace WML

#endif
