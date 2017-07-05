#include "sgvisu~.h"
#include "util/scope_guard.h"

PD_LAYOUT_CHECK(t_sgvisu);

static t_class *sgvisu_class;

void sgvisu_tilde_setup() {
  sgvisu_class = class_new(
      gensym("sgvisu~"),
      (t_newmethod)&sgvisu_new, (t_method)&sgvisu_free, sizeof(t_sgvisu),
      CLASS_DEFAULT, A_GIMME, A_NULL);
  CLASS_MAINSIGNALIN(
      sgvisu_class, t_sgvisu, x_signalin);
  class_addbang(
      sgvisu_class, &sgvisu_bang);
  class_addmethod(
      sgvisu_class, (t_method)&sgvisu_dsp, gensym("dsp"), A_CANT, A_NULL);
}

void *sgvisu_new(t_symbol *s, int argc, t_atom *argv) {
  t_sgvisu *x = (t_sgvisu *)pd_new(sgvisu_class);
  if (!x)
    return nullptr;
  x->x_cleanup = nullptr;

  bool success = false;
  scope(exit) { if (!success) pd_free((t_pd *)x); };

  ///
  try {
    new (x) t_sgvisu;
    x->x_cleanup = [](t_visu *b) {
      static_cast<t_sgvisu *>(b)->~t_sgvisu(); };
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  ///
  try {
    if (!sgvisu_opts(x, argc, argv))
      return nullptr;
    visu_init(x, Visu_Spectrogram);
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  success = true;
  return x;
}

void sgvisu_free(t_sgvisu *x) {
  visu_free(x);
}

bool sgvisu_opts(t_sgvisu *x, int argc, t_atom *argv) {
  switch (argc) {
    case 0:
      break;
    case 1: {
      char buf[128];
      atom_string(&argv[0], buf, sizeof(buf));
      x->x_title.assign(buf);
      break;
    }
    default:
      return false;
  }
  return true;
}

void sgvisu_bang(t_sgvisu *x) {
  visu_bang(x);
}

void sgvisu_dsp(t_sgvisu *x, t_signal **sp) {
  visu_dsp(x, sp);
}
