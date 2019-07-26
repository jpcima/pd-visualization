#include "tfvisu~.h"
#include "util/scope_guard.h"

PD_LAYOUT_CHECK(t_tfvisu);

static t_class *tfvisu_class;

void tfvisu_tilde_setup() {
  tfvisu_class = class_new(
      gensym("tfvisu~"),
      (t_newmethod)&tfvisu_new, (t_method)&tfvisu_free, sizeof(t_tfvisu),
      CLASS_DEFAULT, A_GIMME, A_NULL);
  CLASS_MAINSIGNALIN(
      tfvisu_class, t_tfvisu, x_signalin);
  class_addbang(
      tfvisu_class, &tfvisu_bang);
  class_addmethod(
      tfvisu_class, (t_method)&tfvisu_dsp, gensym("dsp"), A_CANT, A_NULL);
  visu_setup_generic_methods<t_tfvisu>(tfvisu_class);
}

void *tfvisu_new(t_symbol *s, int argc, t_atom *argv) {
  t_tfvisu *x = (t_tfvisu *)pd_new(tfvisu_class);
  if (!x)
    return nullptr;
  x->x_cleanup = nullptr;

  bool success = false;
  scope(exit) { if (!success) pd_free((t_pd *)x); };

  ///
  try {
    new (x) t_tfvisu;
    x->x_cleanup = [](t_visu *b) {
      static_cast<t_tfvisu *>(b)->~t_tfvisu(); };
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  ///
  try {
    if (!tfvisu_opts(x, argc, argv))
      return nullptr;
    visu_init(x, Visu_Transfer);
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  success = true;
  return x;
}

void tfvisu_free(t_tfvisu *x) {
  visu_free(x);
}

bool tfvisu_opts(t_tfvisu *x, int argc, t_atom *argv) {
  std::string title;
  unsigned channels = 2;
  switch (argc) {
    case 2: {
      channels = (int)atom_getfloat(&argv[1]);
      if ((int)channels <= 0 || channels > channelmax)
          return false;
    }
    case 1: {
      char buf[128];
      atom_string(&argv[0], buf, sizeof(buf));
      title.assign(buf);
    }
    case 0:
      break;
    default:
      return false;
  }
  x->x_title = std::move(title);
  x->x_channels = channels;
  return true;
}

void tfvisu_bang(t_tfvisu *x) {
  visu_bang(x);
}

void tfvisu_dsp(t_tfvisu *x, t_signal **sp) {
  visu_dsp(x, sp);
}
