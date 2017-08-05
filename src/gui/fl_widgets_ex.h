#pragma once
#include <Fl_Knob/Fl_Knob.H>

class Fl_KnobEx : public Fl_Knob {
 public:
  Fl_KnobEx(int x, int y, int w, int h, const char *l = nullptr)
      : Fl_Knob(x, y, w, h, l) {}
  int handle(int event) override;
 private:
  double wheelstep = 1.0 / 20;
};
