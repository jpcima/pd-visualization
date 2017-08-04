#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_Valuator.H>

#define VALUE_CALLBACK(v, r, f)                    \
  (v)->callback([](Fl_Widget *w, void *p) {        \
      static_cast<decltype((r))>(p)->f(            \
          static_cast<Fl_Valuator *>(w)->value()); \
    }, (r));
