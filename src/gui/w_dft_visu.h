#pragma once
#include <FL/Fl_Group.H>
#include <complex>

class W_DftVisu : public Fl_Group {
 public:
  W_DftVisu(int x, int y, int w, int h)
      : Fl_Group(x, y, w, h) {}
  virtual ~W_DftVisu() {}

  virtual void update_data(
      const std::complex<float> *spec, unsigned n, float fs) = 0;
  virtual void reset_data() = 0;
};
