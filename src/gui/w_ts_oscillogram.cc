#include "w_ts_oscillogram.h"
#include <FL/fl_draw.H>
#include <array>
#include <vector>

struct W_TsOscillogram::Impl {
  int x {}, y {}, w {}, h {};

  float samplerate = 44100;
  std::vector<float> timedata;

  void draw_back();
  void draw_data();

  void screen_dims(int *px, int *py, int *pw, int *ph);
};

W_TsOscillogram::W_TsOscillogram(int x, int y, int w, int h)
    : W_TsVisu(x, y, w, h),
      P(new Impl) {
  P->x = x;
  P->y = y;
  P->w = w;
  P->h = h;
}

W_TsOscillogram::~W_TsOscillogram() {
}

void W_TsOscillogram::update_ts_data(
    float sr, const float *smps, unsigned len) {
  P->samplerate = sr;
  P->timedata.assign(smps, smps + len);
}

void W_TsOscillogram::reset_data() {
  P->timedata.clear();
}

void W_TsOscillogram::draw() {
  Fl_Group::draw();
  //
  int x, y, w, h;
  P->screen_dims(&x, &y, &w, &h);
  fl_push_clip(x, y, w, h);
  //
  P->draw_back();
  P->draw_data();
  fl_pop_clip();
}

void W_TsOscillogram::Impl::draw_back() {
  int x, y, w, h;
  this->screen_dims(&x, &y, &w, &h);

  fl_color(0, 0, 0);
  fl_rectf(x, y, w, h);

  
}

void W_TsOscillogram::Impl::draw_data() {
  int x, y, w, h;
  this->screen_dims(&x, &y, &w, &h);

  const float *p = this->timedata.data();
  unsigned n = this->timedata.size();

  if (n == 0)
    return;

  fl_color(255, 0, 0);

  float sr = this->samplerate;
  float tb = 100e-3;

  int xi {}, yi {};

  for (int i = 0; i < w; ++i) {
    // float r = float(i) / (w-1);
    float r = float(w-1-i) / (w-1);
    // float idf = r * tb * sr;
    float idf = (n - 1) - r * tb * sr;

    std::array<int, 2> idn {int(idf), int(idf) + 1};
    float mu = idf - idn[0];

    std::array<float, idn.size()> ismp;
    for (unsigned i = 0; i < ismp.size(); ++i) {
      unsigned idx = idn[0];
      ismp[i] = (int(idx) < 0) ? 0 : (idx >= n) ? 0 : p[idx];
    }
    float smp = ismp[0] * (1-mu) + ismp[1] * mu;

    int oldxi = xi;
    int oldyi = yi;
    xi = x + i;
    yi = y + h * (-smp + 1) / 2;

    if (i > 0)
      fl_line(oldxi, oldyi, xi, yi);
  }
}

int W_TsOscillogram::handle(int event) {
  
  return Fl_Group::handle(event);
}

void W_TsOscillogram::Impl::screen_dims(int *px, int *py, int *pw, int *ph) {
  int x = this->x;
  int y = this->y;
  int w = this->w;
  int h = this->h;
  if (px) *px = x;
  if (py) *py = y;
  if (pw) *pw = w;
  if (ph) *ph = h;
}
