#pragma once
#include "visu~-common.h"
#include "util/unix.h"
#include <m_pd.h>
#include <thread>
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
  void (*x_cleanup)(t_visu *) = nullptr;
  void commander_thread_routine();
};

void visu_init(t_visu *x, VisuType t);
void visu_free(t_visu *x);
void visu_bang(t_visu *x);
void visu_dsp(t_visu *x, t_signal **sp);
t_int *visu_perform(t_int *w);
