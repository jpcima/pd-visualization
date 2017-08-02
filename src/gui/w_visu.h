#pragma once
#include <FL/Fl_Group.H>

class W_Visu : public Fl_Group {
 public:
  W_Visu(int x, int y, int w, int h)
      : Fl_Group(x, y, w, h) {}
  virtual ~W_Visu() {}

  virtual void reset_data() = 0;
};
