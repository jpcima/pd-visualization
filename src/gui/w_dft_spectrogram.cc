#include "w_dft_spectrogram.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <algorithm>
#include <vector>
#include <cmath>
#include <stdint.h>
#include <string.h>

struct W_DftSpectrogram::Impl {
  W_DftSpectrogram *Q = nullptr;

  float fsref = 44100;
  float fs = 44100;

  int mx = -1, my = -1;

  std::vector<std::complex<float>> spec;
  unsigned channels = 0;

  Fl_Box *rulertop = nullptr;
  Fl_Box *rulerbtm = nullptr;
  Fl_Box *screen = nullptr;

  float dbmin = -140;
  float dbmax = +0;

  // data
  void draw_rulers();
  void draw_back();
  void draw_data();
  void draw_pointer(int x, int y);
};

// margins
static constexpr int /*mw = 20,*/ mh = 20;

W_DftSpectrogram::W_DftSpectrogram(int x, int y, int w, int h)
    : W_DftVisu(x, y, w, h),
      P(new Impl) {
  P->Q = this;

  int sx = x;
  int sy = y;
  int sw = w;
  int sh = h;

  bool b_rulertop = true;
  bool b_rulerbtm = true;

  if (b_rulertop) {
    P->rulertop = new Fl_Box(x, y, w, mh, "");
    P->rulertop->box(FL_UP_BOX);
    sy += mh;
    sh -= std::min(sh, mh);
  }
  if (b_rulerbtm) {
    P->rulerbtm = new Fl_Box(x, y+h-mh, w, mh, "");
    P->rulerbtm->box(FL_UP_BOX);
    sh -= std::min(sh, mh);
  }

  P->screen = new Fl_Box(sx, sy, sw, sh);
  this->resizable(P->screen);

  this->end();
}

W_DftSpectrogram::~W_DftSpectrogram() {
}

void W_DftSpectrogram::update_dft_data(
    const std::complex<float> *spec[], unsigned n, float fs, unsigned nch) {
  P->spec.clear();
  P->spec.reserve(nch * n);
  for (unsigned c = 0; c < nch; ++c)
      P->spec.insert(P->spec.end(), spec[c], spec[c] + n);
  P->fs = fs;
  P->channels = nch;
}

void W_DftSpectrogram::reset_data() {
  P->spec.clear();
  P->channels = 0;
}

void W_DftSpectrogram::draw() {
  Fl_Group::draw();
  //
  P->draw_rulers();
  //
  int sx = P->screen->x();
  int sy = P->screen->y();
  int sw = P->screen->w();
  int sh = P->screen->h();
  if (sw > 0 && sh > 0) {
    fl_push_clip(sx, sy, sw, sh);
    P->draw_back();
    P->draw_data();
    //
    int mx = P->mx;
    int my = P->my;
    if (mx >= sx && mx < sx+sw && my >= sy && my < sy+sh)
      P->draw_pointer(mx, my);
    fl_pop_clip();
  }
}

int W_DftSpectrogram::handle(int event) {
  if (event == FL_ENTER) {
    fl_cursor(FL_CURSOR_CROSS);
    return 1;
  }

  if (event == FL_LEAVE) {
    P->mx = -1;
    P->my = -1;
    fl_cursor(FL_CURSOR_DEFAULT);
    return 1;
  }

  if (event == FL_MOVE) {
    P->mx = Fl::event_x();
    P->my = Fl::event_y();
    return 1;
  }

  return Fl_Group::handle(event);
}

void W_DftSpectrogram::Impl::draw_rulers() {
  int x = Q->x();
  int y = Q->y();
  int w = Q->w();
  int h = Q->h();

  fl_color(0, 0, 0);

  float f_nyq = this->fsref / 2;
  float f_interval = 250;
  for (unsigned i = 0; ; ++i) {
    float f = f_interval * i;
    if (f > f_nyq)
      break;

    int xf = x + (w-1) * f / f_nyq;

    const int gradh[] = {16, 6, 8, 6};
    int l = gradh[i%4];
    if (this->rulertop)
      fl_line(xf, y+mh-l, xf, y+mh-1);
    if (this->rulerbtm)
      fl_line(xf, y+h-mh, xf, y+h-mh+l);

    char textbuf[32];
    // snprintf(textbuf, sizeof(textbuf), "%g", f);
    snprintf(textbuf, sizeof(textbuf), "%gk", f/1000);
    textbuf[sizeof(textbuf)-1] = 0;

    fl_font(FL_COURIER, 10);
    if (i%4 == 0) {
      if (this->rulertop)
        fl_draw(textbuf, xf+4, y+2, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_TOP, nullptr, 0);
      if (this->rulerbtm)
        fl_draw(textbuf, xf+4, y+h-1, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_BOTTOM, nullptr, 0);
    }
  }
}

void W_DftSpectrogram::Impl::draw_back() {
  int sx = this->screen->x();
  int sy = this->screen->y();
  int sw = this->screen->w();
  int sh = this->screen->h();

  fl_color(0, 0, 0);
  fl_rectf(sx, sy, sw, sh);

  float f_nyq = this->fsref / 2;
  float f_interval = 1000;

  int lastx = 0;
  for (unsigned i = 1; ; ++i) {
    float f = f_interval * i;
    if (f > f_nyq)
      break;
    int xf = sx + (sw-1) * f / f_nyq;
    fl_color(50, 50, 50);
    fl_line(xf, sy, xf, sy+sh-1);
    lastx = xf;
  }

  float dbmin = this->dbmin;
  float dbmax = this->dbmax;
  float ginterval = 20;

  for (unsigned i = 0; ; ++i) {
    float g = dbmin + ginterval * i;
    if (g > dbmax)
      break;
    float dv = (g - dbmin) / (dbmax - dbmin);
    int yg = sy + (1-dv) * (sh-1);
    fl_color(50, 50, 50);
    fl_line(sx, yg, sx+sw-1, yg);

    char textbuf[16];
    snprintf(textbuf, sizeof(textbuf), "%g dB", g);
    textbuf[sizeof(textbuf)-1] = 0;

    fl_font(FL_COURIER, 10);
    fl_color(200, 200, 200);
    fl_draw(textbuf, lastx-2, yg, 0, 0, FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM, nullptr, 0);
    fl_draw(textbuf, sx+2, yg, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_BOTTOM, nullptr, 0);
  }
}

void W_DftSpectrogram::Impl::draw_data() {
  const unsigned channels = this->channels;
  if (channels == 0)
    return;

  const unsigned specn = this->spec.size() / channels;
  if (specn == 0)
    return;

  const float fs = this->fs;
  const float fsref = this->fsref;

  const int sx = this->screen->x();
  const int sy = this->screen->y();
  const int sw = this->screen->w();
  const int sh = this->screen->h();

  for (unsigned c = channels; c-- > 0;) {
    const std::complex<float> *spec = &this->spec[c * specn];

    const unsigned hue = (170 + c * 130) % 360;
    color::hsv<float> col_hsv{(float)hue, 75, 80};
    color::rgb<uint8_t> col_rgb(col_hsv);

    fl_color(
      color::get::red(col_rgb),
      color::get::green(col_rgb),
      color::get::blue(col_rgb));

    int lasty {};
    for (int i = 0; i < sw; ++i) {
      float rx = i / float(sw-1);

      // no interpolation
      int speci = std::lrint(rx * (specn-1) * (fsref / fs));
      float a = std::abs(spec[speci]);

      float g = (a > 0.0f) ? (20 * std::log10(a)) : dbmin;
      float dv = (g - dbmin) / (dbmax - dbmin);

      int newy = sy+(1-dv)*(sh-1);
      if (i > 0)
          fl_line(sx+i-1, lasty, sx+i, newy);
      lasty = newy;
    }
  }
}

void W_DftSpectrogram::Impl::draw_pointer(int mx, int my) {
  int sx = this->screen->x();
  int sy = this->screen->y();
  int sw = this->screen->w();
  int sh = this->screen->h();

  float f_nyq = this->fsref / 2;
  float f = (mx-sx) * f_nyq / (sw-1);

  float dbmin = this->dbmin;
  float dbmax = this->dbmax;
  float dv = 1-float(my-sy)/(sh-1);
  float g = dbmin + dv * (dbmax - dbmin);

  fl_color(150, 150, 150);
  fl_line(mx, sy, mx, sy+sh-1);
  fl_line(sx, my, sx+sw-1, my);

  char textbuf[64];
  snprintf(textbuf, sizeof(textbuf), "%.2f Hz %.2f dB", f, g);
  textbuf[sizeof(textbuf)-1] = 0;

  fl_font(FL_COURIER, 12);
  int tw = 8+std::ceil(fl_width("00000.00 Hz -000.00 dB"));
  int th = 16;
  int tx = sx+sw-1-tw-4;
  int ty = sy+4;
  int tal = FL_ALIGN_LEFT|FL_ALIGN_CENTER;
  fl_color(50, 50, 50);
  fl_rectf(tx, ty, tw, th);
  fl_color(200, 200, 200);
  fl_rect(tx, ty, tw, th);
  fl_draw(textbuf, tx+4, ty, tw, th, tal, nullptr, 0);
}
