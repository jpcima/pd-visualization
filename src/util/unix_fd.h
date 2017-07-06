#pragma once

class unix_fd {
 public:
  constexpr unix_fd() noexcept;
  explicit constexpr unix_fd(int fd) noexcept;
  ~unix_fd();

  unix_fd(const unix_fd &) = delete;
  unix_fd &operator=(const unix_fd &) = delete;

  unix_fd(unix_fd &&o) noexcept;
  unix_fd &operator=(unix_fd &&o) noexcept;

  void reset(int fd = -1) noexcept;
  int get() const noexcept;
  int operator*() const noexcept;
  explicit operator bool() const noexcept;

 private:
  int fd_ = -1;
};

void unix_pipe(unix_fd p[2]);

#include "unix_fd.tcc"
