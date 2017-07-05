#pragma once
#include "visu~.h"

#ifdef WIN32
# define EXPORT __declspec(dllexport)
#else
# define EXPORT [[gnu::visibility("default")]]
#endif

extern "C" {
  EXPORT void sgvisu_tilde_setup();
}

///
struct t_sgvisu : t_visu {};

void *sgvisu_new(t_symbol *s, int argc, t_atom *argv);
void sgvisu_free(t_sgvisu *x);
bool sgvisu_opts(t_sgvisu *x, int argc, t_atom *argv);
void sgvisu_bang(t_sgvisu *x);
void sgvisu_dsp(t_sgvisu *x, t_signal **sp);
