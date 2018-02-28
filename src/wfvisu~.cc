#include "wfvisu~.h"
#include "util/scope_guard.h"

PD_LAYOUT_CHECK(t_wfvisu);

static t_class *wfvisu_class;

void wfvisu_tilde_setup() {
  wfvisu_class = class_new(
      gensym("wfvisu~"),
      (t_newmethod)&wfvisu_new, (t_method)&wfvisu_free, sizeof(t_wfvisu),
      CLASS_DEFAULT, A_GIMME, A_NULL);
  CLASS_MAINSIGNALIN(
      wfvisu_class, t_wfvisu, x_signalin);
  class_addbang(
      wfvisu_class, &wfvisu_bang);
  class_addmethod(
      wfvisu_class, (t_method)&wfvisu_dsp, gensym("dsp"), A_CANT, A_NULL);
}

void *wfvisu_new(t_symbol *s, int argc, t_atom *argv) {
  t_wfvisu *x = (t_wfvisu *)pd_new(wfvisu_class);
  if (!x)
    return nullptr;
  x->x_cleanup = nullptr;

  bool success = false;
  scope(exit) { if (!success) pd_free((t_pd *)x); };

  ///
  try {
    new (x) t_wfvisu;
    x->x_cleanup = [](t_visu *b) {
      static_cast<t_wfvisu *>(b)->~t_wfvisu(); };
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  ///
  try {
    if (!wfvisu_opts(x, argc, argv))
      return nullptr;
    visu_init(x, Visu_Waterfall);
  } catch (std::exception &ex) {
    error("%s", ex.what());
    return nullptr;
  }

  success = true;
  return x;
}

void wfvisu_free(t_wfvisu *x) {
  visu_free(x);
}

bool wfvisu_opts(t_wfvisu *x, int argc, t_atom *argv) {
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

void wfvisu_bang(t_wfvisu *x) {
  visu_bang(x);
}

void wfvisu_dsp(t_wfvisu *x, t_signal **sp) {
  visu_dsp(x, sp);
}
