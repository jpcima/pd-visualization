#pragma once
#include <memory>

class RemoteVisu {
 public:
  RemoteVisu();
  ~RemoteVisu();

  bool is_running() const;

  void start(const char *pgm, const char *title = nullptr);
  void stop();

  bool toggle_visibility();
  bool send_samples(float fs, const float *smp, unsigned n);

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};
