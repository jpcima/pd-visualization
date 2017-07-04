#include "unix_fd.h"
#include <system_error>
#include <unistd.h>

inline unix_fd::unix_fd() {
}

inline unix_fd::~unix_fd() {
  if (fd_ != -1)
    close(fd_);
}

inline unix_fd::unix_fd(int fd)
    : fd_(fd) {
}

inline unix_fd::unix_fd(unix_fd &&o)
    : fd_(o.fd_) {
  o.fd_ = -1;
}

inline unix_fd &unix_fd::operator=(unix_fd &&o) {
  reset(o.fd_);
  o.fd_ = -1;
  return *this;
}

inline void unix_fd::reset(int fd) {
  if (fd != fd_) {
    if (fd_ != -1)
      close(fd_);
    fd_ = fd;
  }
}

inline int unix_fd::get() const {
  return fd_;
}

inline int unix_fd::operator*() const {
  return fd_;
}

inline void unix_pipe(unix_fd p[2]) {
  int pa[2];
  if (pipe(pa) == -1)
    throw std::system_error(errno, std::generic_category());
  p[0].reset(pa[0]);
  p[1].reset(pa[1]);
}
