#pragma once
#include "visu~-common.h"
#include "util/unix.h"
#include <m_pd.h>
#include <thread>
#include <atomic>
#include <string>
#include <memory>

///
class RemoteVisu;

///
struct t_visu {
  t_visu();
  ~t_visu();
  t_object x_obj;
  float x_signalin = 0;
  VisuType x_visutype = Visu_Default;
  std::string x_title;
  unsigned x_channels = 2;
  std::unique_ptr<RemoteVisu> x_remote;
  std::thread x_commander;
  unix_sock x_comm[2];
  std::atomic_bool x_ready{false};
  t_outlet *x_out_ready = nullptr;
  void (*x_cleanup)(t_visu *) = nullptr;
  void commander_thread_routine();
};

void visu_init(t_visu *x, VisuType t);
void visu_free(t_visu *x);
void visu_bang(t_visu *x);
void visu_dsp(t_visu *x, t_signal **sp);
t_int *visu_perform(t_int *w);

///
void visu_position(t_visu *x, t_float xpos, t_float ypos);
void visu_size(t_visu *x, t_float w, t_float h);

///
template <class T>
void visu_setup_generic_methods(t_class *c)
{
  class_addmethod(
      c, (t_method)+[](t_visu *x, t_float xpos, t_float ypos) { visu_position(x, xpos, ypos); },
      gensym("position"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(
      c, (t_method)+[](t_visu *x, t_float w, t_float h) { visu_size(x, w, h); },
      gensym("size"), A_FLOAT, A_FLOAT, A_NULL);
}
