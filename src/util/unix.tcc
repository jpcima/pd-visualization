#include "unix.h"
#include <utility>
#include <errno.h>
#include <unistd.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <poll.h>
#endif

#if !defined(_WIN32) || _WIN32_WINNT >= 0x600
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
#endif

inline int select1(SOCKET readfd, SOCKET writefd, SOCKET exceptfd, struct timeval *timeout) {
  SOCKET nfds = 0;
  SOCKET fds[3] { readfd, writefd, exceptfd };
  fd_set sets[3];
  fd_set *setp[3] {};
  for (unsigned i = 0; i < 3; ++i) {
    if (fds[i] != INVALID_SOCKET) {
      FD_ZERO(&sets[i]);
      FD_SET(fds[i], &sets[i]);
      setp[i] = &sets[i];
      nfds = std::max(nfds, fds[i] + 1);
    }
  }
  return select(nfds, setp[0], setp[1], setp[2], timeout);
}

inline int socksetblocking(SOCKET fd, bool block) {
#ifdef _WIN32
  u_long wbio = !block;
  bool fail = ioctlsocket(fd, FIONBIO, &wbio) == SOCKET_ERROR;
#else
  int flags = fcntl(fd, F_GETFL);
  bool fail = flags == -1;
  if (!fail) {
    int newflags = block ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    if (flags != newflags)
      fail = fcntl(fd, F_SETFL, newflags) == -1;
  }
#endif
  return fail ? -1 : 0;
}

template <class F, class... A>
auto posix_retry(const F &f, A &&... args) {
  auto ret = f(std::forward<A>(args)...);
  while (ret == -1 && errno == EINTR)
    ret = f(std::forward<A>(args)...);
  return ret;
}

template <class F, class... A>
auto socket_retry(const F &f, A &&... args) {
  auto ret = f(std::forward<A>(args)...);
  while (ret == -1 && errno == SOCK_ERR(EINTR))
    ret = f(std::forward<A>(args)...);
   return ret;
 }
