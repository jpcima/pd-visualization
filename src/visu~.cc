#include "visu~.h"
#include "visu~-remote.h"
#include "util/self_path.h"

t_visu::t_visu() {
}

t_visu::~t_visu() {
  if (this->x_commander.joinable()) {
    SOCKET fd = this->x_comm[1].get();
    char msg = 0;
    for (size_t n; (n = socket_retry(send, fd, &msg, 1, 0)) == 0 ||
             ((ssize_t)n == -1 && socket_errno() == SOCK_ERR(EWOULDBLOCK));)
      select1(INVALID_SOCKET, fd, INVALID_SOCKET, nullptr);
    try { this->x_commander.join(); }
    catch (std::system_error &) { /* must not throw in destructor */ }
  }
}

void visu_init(t_visu *x, VisuType t) {
  x->x_visutype = t;
  for (unsigned i = 1, n = x->x_channels; i < n; ++i)
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
  x->x_remote.reset(new RemoteVisu);
  unix_socketpair(AF_UNIX, SOCK_DGRAM, 0, x->x_comm);
  if (socksetblocking(x->x_comm[1].get(), false) == -1)
    throw std::system_error(socket_errno(), socket_category(), "socksetblocking");
  x->x_commander = std::thread(&t_visu::commander_thread_routine, x);
}

void visu_free(t_visu *x) {
  if (x->x_cleanup)
    x->x_cleanup(x);
}

void visu_bang(t_visu *x) {
  SOCKET fd = x->x_comm[1].get();
  char msg = 1;
  send(fd, &msg, 1, 0);
}

void visu_dsp(t_visu *x, t_signal **sp) {
  t_int elts[2 + channelmax];
  t_int *eltp = elts;
  *eltp++ = (t_int)x;
  for (unsigned i = 0, n = x->x_channels; i < n; ++i)
      *eltp++ = (t_int)sp[i]->s_vec;
  *eltp++ = sp[0]->s_n;
  dsp_addv(visu_perform, eltp - elts, elts);
}

t_int *visu_perform(t_int *w) {
  ++w;
  t_visu *x = (t_visu *)(*w++);
  unsigned channels = x->x_channels;
  const t_sample *in[channelmax];
  for (unsigned c = 0; c < channels; ++c)
      in[c] = (t_sample *)(*w++);
  unsigned n = (uintptr_t)(*w++);

  RemoteVisu &remote = *x->x_remote;
  bool sendok = remote.send_frames(sys_getsr(), in, n, channels);

#if 0  // not RT safe
  if (!sendok && remote.is_running())
    error("error writing to socket, is buffer full?");
#else
  (void)sendok;
#endif

  return w;
}

void t_visu::commander_thread_routine() {
  SOCKET fd = this->x_comm[0].get();
  RemoteVisu &remote = *this->x_remote;

  for (;;) {
    char msg {};
    size_t n = socket_retry(recv, fd, &msg, 1, 0);

    if ((ssize_t)n == -1)
      throw std::system_error(socket_errno(), socket_category(), "recv");

    if (n != 1)
      throw std::runtime_error("error in communication protocol");

    if (msg == 0) {  // quit
      remote.stop();
      break;
    }

    if (msg == 1) {  // bang
      if (remote.is_running()) {
        remote.toggle_visibility();
      } else {
        std::string pgm = self_relative("visu~-gui");
        remote.start(pgm.c_str(), this->x_visutype, this->x_title.c_str());
      }
    }
  }
}

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>

static bool wsainit = false;

extern "C" __declspec(dllexport)
BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID) {
  if (dwReason == DLL_PROCESS_ATTACH) {
    WSADATA wsadata {};
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
      return false;
    wsainit = true;
  }

  if (dwReason == DLL_PROCESS_DETACH) {
    if (wsainit)
      WSACleanup();
  }

  return true;
}
#endif
