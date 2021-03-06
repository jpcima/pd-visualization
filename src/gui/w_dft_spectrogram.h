#pragma once
#include "w_dft_visu.h"
#include <memory>

class W_DftSpectrogram : public W_DftVisu {
 public:
  W_DftSpectrogram(int x, int y, int w, int h);
  ~W_DftSpectrogram();

  void update_dft_data(
      const std::complex<float> *spec[], unsigned n, float fs, unsigned nch) override;
  VisuDftResolution desired_resolution() const override;
  void reset_data() override;

  void draw() override;
  int handle(int event) override;
  void resize(int x, int y, int w, int h) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};
