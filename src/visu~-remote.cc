#include "visu~-remote.h"
#include "visu~-common.h"
#include "util/scope_guard.h"
#include "util/unix.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <system_error>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef _WIN32
# include "util/win32_argv.h"
# include <windows.h>
#else
# include <signal.h>
# include <spawn.h>
# include <sys/wait.h>
#endif

extern "C" {
  extern char **environ;
}

struct RemoteVisu::Impl {
#ifdef _WIN32
  HANDLE hprocess = nullptr;
#else
  pid_t pid = -1;
#endif
  unix_sock sock;
  std::unique_ptr<MessageHeader, void(*)(void *)> msg{nullptr, &::free};
  bool send_message(const MessageHeader *m);

  std::mutex sock_mutex;
  SOCKET acquire_socket(std::unique_lock<std::mutex> &lock);
  SOCKET try_acquire_socket(std::unique_lock<std::mutex> &lock);
};

RemoteVisu::RemoteVisu()
    : P(new Impl) {
  P->msg.reset((MessageHeader *)malloc(msgmax));
  if (!P->msg)
    throw std::bad_alloc();
}

RemoteVisu::~RemoteVisu() {
  stop();
}

bool RemoteVisu::is_running() const {
#ifdef _WIN32
  HANDLE hprocess = P->hprocess;
  if (!hprocess)
    return false;
  DWORD exitcode = 0;
  if (!(GetExitCodeProcess(hprocess, &exitcode) && exitcode == STILL_ACTIVE))
    return false;
#else
  pid_t pid = P->pid;
  if (pid == -1)
    return false;
  int wstate = 0;
  if (posix_retry(waitpid, pid, &wstate, WNOHANG) > 0 &&
      (WIFEXITED(wstate) || WIFSIGNALED(wstate))) {
    P->pid = -1;
    return false;
  }
#endif
  return true;
}

void RemoteVisu::start(const char *pgm, VisuType type, const char *title) {
  if (is_running())
    return;

  std::unique_lock<std::mutex> lock;
  P->acquire_socket(lock);

  bool success = false;

  unix_sock sockpair[2];
#if !defined(__APPLE__)
  unix_socketpair(AF_UNIX, SOCK_DGRAM, PF_UNIX, sockpair);
#else
  unix_socketpair(AF_UNIX, SOCK_DGRAM, PF_UNSPEC, sockpair);
#endif

  SOCKET rfd = sockpair[0].get();
  SOCKET wfd = sockpair[1].get();

  if (socksetblocking(wfd, false) == -1)
    throw std::system_error(socket_errno(), socket_category(), "socksetblocking");

#ifndef _WIN32
  int wfdflags = fcntl(wfd, F_GETFD);
  if (wfdflags == -1 || fcntl(wfd, F_SETFD, wfdflags|FD_CLOEXEC) == -1)
    throw std::system_error(socket_errno(), socket_category(), "fcntl");
#endif
  if (setsockopt(
          wfd, SOL_SOCKET, SO_SNDBUF, (const char *)&sockbuf, sizeof(sockbuf)) == -1)
    throw std::system_error(socket_errno(), socket_category(), "setsockopt");

  char rfd_str[16];
  sprintf(rfd_str, "%" PRIdSOCKET, rfd);

#ifdef _WIN32
  char ppid_str[24];
  sprintf(ppid_str, "%lu", GetCurrentProcessId());
#else
  char ppid_str[16];
  sprintf(ppid_str, "%d", getpid());
#endif

  char visu_str[16];
  sprintf(visu_str, "%d", (int)type);

  char *ps_argv[16];
  unsigned ps_argc = 0;
  ps_argv[ps_argc++] = (char *)pgm;
  ps_argv[ps_argc++] = (char *)"--fd";
  ps_argv[ps_argc++] = rfd_str;
  ps_argv[ps_argc++] = (char *)"--ppid";
  ps_argv[ps_argc++] = ppid_str;
  ps_argv[ps_argc++] = (char *)"--visu";
  ps_argv[ps_argc++] = visu_str;
  if (title) {
    ps_argv[ps_argc++] = (char *)"--title";
    ps_argv[ps_argc++] = (char *)title;
  }
  ps_argv[ps_argc++] = nullptr;

  ///
#ifdef _WIN32
  STARTUPINFOA sinfo {};
  sinfo.cb = sizeof(sinfo);
  sinfo.dwFlags = STARTF_USESTDHANDLES;
  sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  sinfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  sinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION pinfo;

  char *cmdline = strdup(argv_to_commandA(ps_argv).c_str());
  if (!cmdline)
    throw std::bad_alloc();
  scope(exit) { free(cmdline); };

  if (!CreateProcessA(
          nullptr, cmdline,
          nullptr, nullptr,
          true, CREATE_NO_WINDOW,
          nullptr, nullptr,
          &sinfo, &pinfo))
    throw std::system_error(GetLastError(), std::system_category(), "CreateProcess");

  HANDLE hprocess = pinfo.hProcess;
  scope(exit) { if (hprocess) CloseHandle(hprocess); };
  CloseHandle(pinfo.hThread);
#else
  posix_spawn_file_actions_t ps_fact;
  if (posix_spawn_file_actions_init(&ps_fact) == -1)
    throw std::bad_alloc();
  scope(exit) { posix_spawn_file_actions_destroy(&ps_fact); };
  ///
  posix_spawnattr_t ps_attr;
  if (posix_spawnattr_init(&ps_attr) == -1)
    throw std::bad_alloc();
  scope(exit) { posix_spawnattr_destroy(&ps_attr); };

  ///
  pid_t pid;
  int ps_err = posix_spawn(&pid, pgm, &ps_fact, &ps_attr, ps_argv, environ);
  if (ps_err != 0)
    throw std::system_error(ps_err, std::generic_category(), "posix_spawn");
#endif

  scope(exit) {
    if (!success) {
#ifdef _WIN32
      TerminateProcess(hprocess, 1);
#else
      kill(pid, SIGTERM);
      posix_retry(waitpid, pid, nullptr, 0);
#endif
    }
  };

  // wait until ready to prevent startup hiccups
  bool ready = false;
  const unsigned ready_timeout = 10;
  typedef std::chrono::steady_clock clock;
  clock::time_point t1 = clock::now();
  while (!ready) {
    timeval tv { ready_timeout, 0 };
    socket_retry(select1, wfd, INVALID_SOCKET, INVALID_SOCKET, &tv);
    char bytebuf {};
    size_t n = socket_retry(recv, wfd, &bytebuf, 1, 0);
    if ((ssize_t)n == -1) {
      int err = socket_errno();
      if (err != SOCK_ERR(EWOULDBLOCK))
        throw std::system_error(err, socket_category(), "recv");
    } else if (n == 1) {
      if (bytebuf != '!')
        throw std::runtime_error("error in communication protocol");
      ready = true;
    }
    if (clock::now() - t1 > std::chrono::seconds(ready_timeout))
      break;
  }

  if (!ready)
    throw std::runtime_error("timeout waiting for message from child process");

#ifdef _WIN32
  P->hprocess = hprocess;
  hprocess = nullptr;
#else
  P->pid = pid;
#endif
  P->sock = std::move(sockpair[1]);
  success = true;
}

void RemoteVisu::stop() {
#ifdef _WIN32
  HANDLE hprocess = P->hprocess;
  if (!hprocess)
    return;
#else
  pid_t pid = P->pid;
  if (pid == -1)
    return;
#endif

  { std::unique_lock<std::mutex> lock;
    P->acquire_socket(lock);
    P->sock.reset(); }

#ifdef _WIN32
  TerminateProcess(hprocess, 1);
  WaitForSingleObject(hprocess, INFINITE);
  CloseHandle(hprocess);
  P->hprocess = nullptr;
#else
  kill(pid, SIGTERM);
  waitpid(pid, nullptr, 0);
  P->pid = -1;
#endif
}

bool RemoteVisu::toggle_visibility() {
  MessageHeader *msg = P->msg.get();
  msg->tag = MessageTag_Toggle;
  msg->len = 0;
  return P->send_message(msg);
}

bool RemoteVisu::set_position(float x, float y)
{
  MessageHeader *msg = P->msg.get();
  msg->tag = MessageTag_Position;
  msg->len = 2 * sizeof(float);
  msg->f[0] = x;
  msg->f[1] = y;
  return P->send_message(msg);
}

bool RemoteVisu::set_size(float w, float h)
{
  MessageHeader *msg = P->msg.get();
  msg->tag = MessageTag_Size;
  msg->len = 2 * sizeof(float);
  msg->f[0] = w;
  msg->f[1] = h;
  return P->send_message(msg);
}

bool RemoteVisu::set_border(bool b)
{
  MessageHeader *msg = P->msg.get();
  msg->tag = MessageTag_Border;
  msg->len = sizeof(int);
  msg->i[0] = b;
  return P->send_message(msg);
}

bool RemoteVisu::send_frames(
    float fs, const float *data[], unsigned nframes, unsigned nchannels) {
  MessageHeader *msg = P->msg.get();
  msg->tag = MessageTag_SampleRate;
  msg->len = sizeof(float);
  msg->f[0] = fs;
  if (!P->send_message(msg))
    return false;

  constexpr unsigned maxdatalen = msgmax - sizeof(MessageHeader);
  constexpr unsigned headerlen = sizeof(uint32_t);
  constexpr unsigned maxsamples = (maxdatalen - headerlen) / sizeof(float);
  const unsigned maxframes = maxsamples / nchannels;

  msg->tag = MessageTag_Frames;
  for (unsigned i = 0; i < nframes;) {
      unsigned nsend = std::min(nframes - i, maxframes);
      msg->len = headerlen + nsend * nchannels * sizeof(float);
      float *buf = &msg->f[0];
      // header
      *(uint32_t *)buf++ = nchannels;
      // frames
      for (unsigned j = i + nsend; i < j; ++i) {
          for (unsigned c = 0; c < nchannels; ++c)
              *buf++ = data[c][i];
      }
      if (!P->send_message(msg))
          return false;
  }

  return true;
}

bool RemoteVisu::Impl::send_message(const MessageHeader *m) {
  std::unique_lock<std::mutex> lock;
  SOCKET wfd = this->try_acquire_socket(lock);
  if (wfd == INVALID_SOCKET)
    return -1;
  return socket_retry(
      send, wfd, (const char *)m, sizeof(MessageHeader) + m->len, 0) != -1;
}

SOCKET RemoteVisu::Impl::acquire_socket(std::unique_lock<std::mutex> &lock) {
  lock = std::unique_lock<std::mutex>(this->sock_mutex);
  return this->sock.get();
}

SOCKET RemoteVisu::Impl::try_acquire_socket(std::unique_lock<std::mutex> &lock) {
  lock = std::unique_lock<std::mutex>(this->sock_mutex, std::try_to_lock);
  if (!lock.owns_lock())
    return INVALID_SOCKET;
  return this->sock.get();
}
