#pragma once

class unix_fd {
 public:
  unix_fd();
  ~unix_fd();
  explicit unix_fd(int fd);

  unix_fd(const unix_fd &) = delete;
  unix_fd &operator=(const unix_fd &) = delete;

  unix_fd(unix_fd &&o);
  unix_fd &operator=(unix_fd &&o);

  void reset(int fd = -1);
  int get() const;
  int operator*() const;
  explicit operator bool() const;

 private:
  int fd_ = -1;
};

void unix_pipe(unix_fd p[2]);

#include "unix_fd.tcc"
