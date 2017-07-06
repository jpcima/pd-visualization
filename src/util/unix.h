#pragma once
#include "util/unix_fd.h"
#include "util/unix_sock.h"

#if !defined(_WIN32) || _WIN32_WINNT >= 0x600
int poll1(SOCKET fd, int timeout, int events);
#endif
int select1(SOCKET readfd, SOCKET writefd, SOCKET exceptfd, struct timeval *timeout);
int socksetblocking(SOCKET fd, bool block);
template <class F, class... A> auto posix_retry(const F &f, A &&... args);
template <class F, class... A> auto socket_retry(const F &f, A &&... args);

#include "unix.tcc"
