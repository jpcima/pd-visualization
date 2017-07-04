#include "wfvisu~.h"
#include "visu~-remote.h"
#include "util/self_path.h"
#include "util/scope_guard.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#endif

static t_class *wfvisu_class;

void wfvisu_tilde_setup() {
  wfvisu_class = class_new(
      gensym("wfvisu~"),
      (t_newmethod)&wfvisu_new, (t_method)&wfvisu_free, sizeof(t_wfvisu),
      CLASS_DEFAULT, A_GIMME, A_NULL);
  CLASS_MAINSIGNALIN(wfvisu_class, t_wfvisu, x_signalin);
  class_addbang(wfvisu_class, &wfvisu_bang);
  class_addmethod(
      wfvisu_class, (t_method)wfvisu_dsp, gensym("dsp"), A_CANT, A_NULL);
}

void *wfvisu_new(t_symbol *s, int argc, t_atom *argv) {
  t_wfvisu *x = (t_wfvisu *)pd_new(wfvisu_class);
  if (!x)
    return nullptr;
  x->x_init = false;

  try {
    new (x) t_wfvisu;
    x->x_init = true;

    bool success = false;
    scope(exit) { if (!success) pd_free((t_pd *)x); };

    if (argc == 1) {
      char buf[128];
      atom_string(&argv[0], buf, sizeof(buf));
      x->x_title.assign(buf);
    } else if (argc > 0) {
      return nullptr;
    }

    x->x_remote.reset(new RemoveVisu);
    success = true;
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  return x;
}

void wfvisu_free(t_wfvisu *x) {
  if (x->x_init)
    x->~t_wfvisu();
}

void wfvisu_bang(t_wfvisu *x) {
  // TODO not RT safe
  // TODO not exception safe
  RemoveVisu &remote = *x->x_remote;
  if (remote.is_running()) {
    remote.toggle_visibility();
  } else {
    std::string pgm = self_relative("visu~-gui");
    remote.start(pgm.c_str(), x->x_title.c_str());
  }
}

void wfvisu_dsp(t_wfvisu *x, t_signal **sp) {
  t_int elts[] = { (t_int)x, (t_int)sp[0]->s_vec, sp[0]->s_n };
  dsp_addv(wfvisu_perform, sizeof(elts) / sizeof(*elts), elts);
}

t_int *wfvisu_perform(t_int *w) {
  ++w;
  t_wfvisu *x = (t_wfvisu *)(*w++);
  t_sample *in = (t_sample *)(*w++);
  unsigned n = (uintptr_t)(*w++);

  RemoveVisu &remote = *x->x_remote;
  bool sendok = remote.send_samples(sys_getsr(), in, n);

#if 0  // not RT safe
  if (!sendok && remote.is_running())
    error("error writing to socket, is buffer full?");
#endif

  return w;
}

#ifdef _WIN32
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
