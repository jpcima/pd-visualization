#pragma once
#include "visu~-common.h"
#include <m_pd.h>
#include <string>
#include <memory>

#ifdef WIN32
# define EXPORT __declspec(dllexport)
#else
# define EXPORT [[gnu::visibility("default")]]
#endif

extern "C" {
  EXPORT void wfvisu_tilde_setup();
}

///
class RemoveVisu;

///
struct t_wfvisu {
  t_object x_obj;
  float x_signalin = 0;
  std::string x_title;
  std::unique_ptr<RemoveVisu> x_remote;
  bool x_init = false;
};

void *wfvisu_new(t_symbol *s, int argc, t_atom *argv);
void wfvisu_free(t_wfvisu *x);
void wfvisu_bang(t_wfvisu *x);
void wfvisu_dsp(t_wfvisu *x, t_signal **sp);
t_int *wfvisu_perform(t_int *w);
