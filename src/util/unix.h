#pragma once
#include "util/unix_sock.h"

int poll1(SOCKET fd, int timeout, int events);
template <class F, class... A> auto eintr_retry(const F &f, A &&... args);

#include "unix.tcc"
