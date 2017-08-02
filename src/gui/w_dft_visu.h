#pragma once
#include "w_visu.h"
#include <complex>

class W_DftVisu : public W_Visu {
 public:
  W_DftVisu(int x, int y, int w, int h)
      : W_Visu(x, y, w, h) {}
  virtual ~W_DftVisu() {}

  virtual void update_dft_data(
      const std::complex<float> *spec, unsigned n, float fs) = 0;
};
