#pragma once
#include "visu~-common.h"
#include <memory>

class RemoteVisu {
 public:
  RemoteVisu();
  ~RemoteVisu();

  bool is_running() const;

  void start(
      const char *pgm, VisuType type = Visu_Default,
      const char *title = nullptr);
  void stop();

  bool toggle_visibility();
  bool set_position(float x, float y);
  bool set_size(float w, float h);
  bool send_frames(
      float fs, const float *data[], unsigned nframes, unsigned nchannels);

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};
