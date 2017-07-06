#pragma once
#include <system_error>
#ifdef _WIN32
# include <winsock2.h>
# include <inttypes.h>
#endif

#ifdef _WIN32
# define PRIdSOCKET PRIdPTR
# define SCNdSOCKET SCNdPTR
# define SOCK_ERR(x) WSA##x
#else
# define PRIdSOCKET "d"
# define SCNdSOCKET "d"
# define SOCK_ERR(x) x
typedef int SOCKET;
static constexpr int INVALID_SOCKET = -1;
#endif

class unix_sock {
 public:
  constexpr unix_sock() noexcept;
  explicit constexpr unix_sock(SOCKET sock) noexcept;
  ~unix_sock();

  unix_sock(const unix_sock &) = delete;
  unix_sock &operator=(const unix_sock &) = delete;

  unix_sock(unix_sock &&o) noexcept;
  unix_sock &operator=(unix_sock &&o) noexcept;

  void reset(SOCKET sock = INVALID_SOCKET) noexcept;
  SOCKET get() const noexcept;
  SOCKET operator*() const noexcept;
  explicit operator bool() const noexcept;

 private:
  SOCKET sock_ = INVALID_SOCKET;
};

const std::error_category &socket_category() noexcept;
int socket_errno() noexcept;

void unix_socketpair(int domain, int type, int protocol, unix_sock s[2]);

#include "unix_sock.tcc"
