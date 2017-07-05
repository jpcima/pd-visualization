#include "visu~.h"
#include "visu~-remote.h"
#include "util/self_path.h"

t_visu::t_visu() {
}

t_visu::~t_visu() {
}

void visu_init(t_visu *x, VisuType t) {
  x->x_visutype = t;
  x->x_remote.reset(new RemoteVisu);
}

void visu_free(t_visu *x) {
  if (x->x_cleanup)
    x->x_cleanup(x);
}

void visu_bang(t_visu *x) {
  // TODO not RT safe
  // TODO not exception safe
  RemoteVisu &remote = *x->x_remote;
  if (remote.is_running()) {
    remote.toggle_visibility();
  } else {
    std::string pgm = self_relative("visu~-gui");
    remote.start(pgm.c_str(), x->x_visutype, x->x_title.c_str());
  }
}

void visu_dsp(t_visu *x, t_signal **sp) {
  t_int elts[] = { (t_int)x, (t_int)sp[0]->s_vec, sp[0]->s_n };
  dsp_addv(visu_perform, sizeof(elts) / sizeof(*elts), elts);
}

t_int *visu_perform(t_int *w) {
  ++w;
  t_visu *x = (t_visu *)(*w++);
  t_sample *in = (t_sample *)(*w++);
  unsigned n = (uintptr_t)(*w++);

  RemoteVisu &remote = *x->x_remote;
  bool sendok = remote.send_samples(sys_getsr(), in, n);

#if 0  // not RT safe
  if (!sendok && remote.is_running())
    error("error writing to socket, is buffer full?");
#endif

  return w;
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
