#include "unix_sock.h"
#include "util/scope_guard.h"
#include <errno.h>
#include <unistd.h>
#ifdef _WIN32
# include "win32_socketpair.h"
#else
# include <sys/socket.h>
#endif

inline constexpr unix_sock::unix_sock() noexcept {
}

inline unix_sock::~unix_sock() {
  if (sock_ != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(sock_);
#else
    close(sock_);
#endif
  }
}

inline constexpr unix_sock::unix_sock(SOCKET sock) noexcept
    : sock_(sock) {
}

inline unix_sock::unix_sock(unix_sock &&o) noexcept
    : sock_(o.sock_) {
  o.sock_ = INVALID_SOCKET;
}

inline unix_sock &unix_sock::operator=(unix_sock &&o) noexcept {
  reset(o.sock_);
  o.sock_ = INVALID_SOCKET;
  return *this;
}

inline void unix_sock::reset(SOCKET sock) noexcept {
  if (sock != sock_) {
    if (sock_ != INVALID_SOCKET) {
#ifdef _WIN32
      closesocket(sock_);
#else
      close(sock_);
#endif
    }
    sock_ = sock;
  }
}

inline SOCKET unix_sock::get() const noexcept {
  return sock_;
}

inline SOCKET unix_sock::operator*() const noexcept {
  return sock_;
}

inline unix_sock::operator bool() const noexcept {
  return sock_ != INVALID_SOCKET;
}

#ifdef _WIN32
inline const std::error_category &socket_category() noexcept {
  return std::system_category();
}
inline int socket_errno() noexcept {
  return WSAGetLastError();
}
#else
inline const std::error_category &socket_category() noexcept {
  return std::generic_category();
}
inline int socket_errno() noexcept {
  return errno;
}
#endif

inline void unix_socketpair(int domain, int type, int protocol, unix_sock s[2]) {
  SOCKET sa[2];
  if (socketpair(domain, type, protocol, sa) == -1)
    throw std::system_error(errno, socket_category(), "socketpair");
  s[0].reset(sa[0]);
  s[1].reset(sa[1]);
}
