#include "ogvisu~.h"
#include "util/scope_guard.h"

PD_LAYOUT_CHECK(t_ogvisu);

static t_class *ogvisu_class;

void ogvisu_tilde_setup() {
  ogvisu_class = class_new(
      gensym("ogvisu~"),
      (t_newmethod)&ogvisu_new, (t_method)&ogvisu_free, sizeof(t_ogvisu),
      CLASS_DEFAULT, A_GIMME, A_NULL);
  CLASS_MAINSIGNALIN(
      ogvisu_class, t_ogvisu, x_signalin);
  class_addbang(
      ogvisu_class, &ogvisu_bang);
  class_addmethod(
      ogvisu_class, (t_method)&ogvisu_dsp, gensym("dsp"), A_CANT, A_NULL);
}

void *ogvisu_new(t_symbol *s, int argc, t_atom *argv) {
  t_ogvisu *x = (t_ogvisu *)pd_new(ogvisu_class);
  if (!x)
    return nullptr;
  x->x_cleanup = nullptr;

  bool success = false;
  scope(exit) { if (!success) pd_free((t_pd *)x); };

  ///
  try {
    new (x) t_ogvisu;
    x->x_cleanup = [](t_visu *b) {
      static_cast<t_ogvisu *>(b)->~t_ogvisu(); };
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  ///
  try {
    if (!ogvisu_opts(x, argc, argv))
      return nullptr;
    visu_init(x, Visu_Oscillogram);
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  success = true;
  return x;
}

void ogvisu_free(t_ogvisu *x) {
  visu_free(x);
}

bool ogvisu_opts(t_ogvisu *x, int argc, t_atom *argv) {
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

void ogvisu_bang(t_ogvisu *x) {
  visu_bang(x);
}

void ogvisu_dsp(t_ogvisu *x, t_signal **sp) {
  visu_dsp(x, sp);
}
