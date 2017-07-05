#pragma once
#include "visu~.h"

#ifdef WIN32
# define EXPORT __declspec(dllexport)
#else
# define EXPORT [[gnu::visibility("default")]]
#endif

extern "C" {
  EXPORT void wfvisu_tilde_setup();
}

///
struct t_wfvisu : t_visu {};

void *wfvisu_new(t_symbol *s, int argc, t_atom *argv);
void wfvisu_free(t_wfvisu *x);
bool wfvisu_opts(t_wfvisu *x, int argc, t_atom *argv);
void wfvisu_bang(t_wfvisu *x);
void wfvisu_dsp(t_wfvisu *x, t_signal **sp);
