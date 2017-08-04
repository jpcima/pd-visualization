#pragma once
#include <FL/Fl_Widget.H>

#define VALUE_CALLBACK(v, r, f)                    \
  (v)->callback([](Fl_Widget *w, void *p) {        \
      static_cast<decltype(r)>(p)->f(              \
          static_cast<decltype(v)>(w)->value());   \
    }, (r));
