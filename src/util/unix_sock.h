#pragma once
#include <system_error>
#ifdef _WIN32
# include <winsock2.h>
# include <inttypes.h>
#endif

#ifdef _WIN32
# define PRIdSOCKET PRIdPTR
# define SCNdSOCKET SCNdPTR
#else
# define PRIdSOCKET "d"
# define SCNdSOCKET "d"
typedef int SOCKET;
static constexpr int INVALID_SOCKET = -1;
#endif

class unix_sock {
 public:
  unix_sock();
  ~unix_sock();

  explicit unix_sock(SOCKET sock);

  unix_sock(const unix_sock &) = delete;
  unix_sock &operator=(const unix_sock &) = delete;

  unix_sock(unix_sock &&o);
  unix_sock &operator=(unix_sock &&o);

  void reset(SOCKET sock = INVALID_SOCKET);
  SOCKET get() const;
  SOCKET operator*() const;

 private:
  SOCKET sock_ = INVALID_SOCKET;
};

const std::error_category &socket_category() noexcept;
int socket_errno() noexcept;

void unix_socketpair(int domain, int type, int protocol, unix_sock s[2]);

#include "unix_sock.tcc"