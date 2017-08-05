#include "fl_widgets_ex.h"
#include <FL/Fl.H>

int Fl_KnobEx::handle(int event) {
  if (event == FL_MOUSEWHEEL) {
    int wx = this->x();
    int wy = this->y();
    int ww = this->w();
    int wh = this->h();
    int mx = Fl::event_x();
    int my = Fl::event_y();
    if (mx >= wx && mx < wx + ww && my >= wy && my < wy + wh) {
      int dx = Fl::event_dx();
      int dy = Fl::event_dy();
      double val = this->value();
      double range = this->maximum() - this->minimum();
      val += range * this->wheelstep * (dx - dy);
      if (this->value(this->clamp(val))) {
        this->set_changed();
        this->do_callback();
      }
      return true;
    }
  }
  return Fl_Knob::handle(event);
}
