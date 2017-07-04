#include "unix.h"
#include <utility>
#include <errno.h>
#include <unistd.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <poll.h>
#endif

inline int poll1(SOCKET fd, int timeout, int events) {
#ifdef _WIN32
  WSAPOLLFD pfd;
#else
  pollfd pfd;
#endif
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
#ifdef _WIN32
  int ret = WSAPoll(&pfd, 1, timeout);
#else
  int ret = poll(&pfd, 1, timeout);
#endif
  if (ret > 0)
    ret = pfd.revents;
  return ret;
}

template <class F, class... A>
auto eintr_retry(const F &f, A &&... args) {
  auto ret = f(std::forward<A>(args)...);
#if !defined(_WIN32)
  while (ret == -1 && errno == EINTR)
    ret = f(std::forward<A>(args)...);
#endif
  return ret;
}
