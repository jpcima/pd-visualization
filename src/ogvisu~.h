#pragma once
#include "visu~.h"

#ifdef WIN32
# define EXPORT __declspec(dllexport)
#else
# define EXPORT [[gnu::visibility("default")]]
#endif

extern "C" {
  EXPORT void ogvisu_tilde_setup();
}

///
struct t_ogvisu : t_visu {};

void *ogvisu_new(t_symbol *s, int argc, t_atom *argv);
void ogvisu_free(t_ogvisu *x);
bool ogvisu_opts(t_ogvisu *x, int argc, t_atom *argv);
void ogvisu_bang(t_ogvisu *x);
void ogvisu_dsp(t_ogvisu *x, t_signal **sp);
