#pragma once
#include "w_visu.h"

class W_TsVisu : public W_Visu {
 public:
  W_TsVisu(int x, int y, int w, int h)
      : W_Visu(x, y, w, h) {}
  virtual ~W_TsVisu() {}

  virtual void update_ts_data(
      float sr, const float *smps, unsigned len) = 0;
};
