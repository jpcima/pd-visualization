#pragma once
#include "visu~.h"

#ifdef WIN32
# define EXPORT __declspec(dllexport)
#else
# define EXPORT [[gnu::visibility("default")]]
#endif

extern "C" {
  EXPORT void tfvisu_tilde_setup();
}

///
struct t_tfvisu : t_visu {};

void *tfvisu_new(t_symbol *s, int argc, t_atom *argv);
void tfvisu_free(t_tfvisu *x);
bool tfvisu_opts(t_tfvisu *x, int argc, t_atom *argv);
void tfvisu_bang(t_tfvisu *x);
void tfvisu_dsp(t_tfvisu *x, t_signal **sp);
