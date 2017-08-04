#pragma once
#include "w_ts_visu.h"
#include <memory>

class W_TsOscillogram : public W_TsVisu {
 public:
  W_TsOscillogram(int x, int y, int w, int h);
  ~W_TsOscillogram();

  void update_ts_data(
      float sr, const float *smps, unsigned len) override;
  void reset_data() override;

  void resize(int x, int y, int w, int h) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};
