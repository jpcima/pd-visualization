#include "w_dft_spectrogram.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <memory>
#include <cmath>
#include <stdint.h>
#include <string.h>

struct W_DftSpectrogram::Impl {
  float fsref = 44100;

  int x {}, y {}, w {}, h {};
  int mx = -1, my = -1;

  std::unique_ptr<float[]> dispvalues;

  Fl_Box *rulertop = nullptr;
  Fl_Box *rulerbtm = nullptr;

  float dbmin = -140;
  float dbmax = +0;

  // data
  void draw_rulers();
  void draw_back();
  void draw_data();
  void draw_pointer(int x, int y);

  void screen_dims(int *px, int *py, int *pw, int *ph);
};

// margins
static constexpr int mw = 20;
static constexpr int mh = 20;

W_DftSpectrogram::W_DftSpectrogram(int x, int y, int w, int h)
    : W_DftVisu(x, y, w, h),
      P(new Impl) {
  P->x = x;
  P->y = y;
  P->w = w;
  P->h = h;

  bool b_rulertop = true;
  bool b_rulerbtm = true;

  this->begin();

  if (b_rulertop) {
    P->rulertop = new Fl_Box(0, 0, w, mh, "");
    P->rulertop->box(FL_UP_BOX);
  }
  if (b_rulerbtm) {
    P->rulerbtm = new Fl_Box(0, h-mh, w, mh, "");
    P->rulerbtm->box(FL_UP_BOX);
  }

  this->end();

  int dw;
  P->screen_dims(nullptr, nullptr, &dw, nullptr);

  P->dispvalues.reset(new float[dw]());
}

W_DftSpectrogram::~W_DftSpectrogram() {
}

void W_DftSpectrogram::update_data(
    const std::complex<float> *spec, unsigned n, float fs) {
  float nyq = fs/2;
  float dbmin = P->dbmin;
  float dbmax = P->dbmax;

  int w, h;
  P->screen_dims(0, 0, &w, &h);

  float *dispvalues = P->dispvalues.get();

  for (int i = 0; i < w; ++i) {
    float rx = i / float(w-1);
    int speci = std::lrint(rx * (n-1));

    // no interpolation
    float a = std::abs(spec[speci]);

    float g = (a > 0.0f) ? (20 * std::log10(a)) : dbmin;

    // value display
    float dv = (g - dbmin) / (dbmax - dbmin);

    dispvalues[i] = dv;
  }
}

void W_DftSpectrogram::reset_data() {
  int dw;
  P->screen_dims(nullptr, nullptr, &dw, nullptr);
  memset(P->dispvalues.get(), 0, dw * sizeof(P->dispvalues[0]));
}

void W_DftSpectrogram::draw() {
  Fl_Group::draw();
  //
  P->draw_rulers();
  //
  int x, y, w, h;
  P->screen_dims(&x, &y, &w, &h);
  fl_push_clip(x, y, w, h);
  P->draw_back();
  P->draw_data();
  //
  int mx = P->mx;
  int my = P->my;
  if (mx >= x && mx < x+w && my >= y && my < y+h)
    P->draw_pointer(mx, my);
  fl_pop_clip();
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
  int x = this->x;
  int y = this->y;
  int w = this->w;
  int h = this->h;

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
    snprintf(textbuf, sizeof(textbuf), "%g", f);
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
  int x, y, w, h;
  screen_dims(&x, &y, &w, &h);

  fl_color(0, 0, 0);
  fl_rectf(x, y, w, h);

  float f_nyq = this->fsref / 2;
  float f_interval = 1000;

  int lastx = 0;
  for (unsigned i = 1; ; ++i) {
    float f = f_interval * i;
    if (f > f_nyq)
      break;
    int xf = x + (w-1) * f / f_nyq;
    fl_color(50, 50, 50);
    fl_line(xf, y, xf, y+h-1);
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
    int yg = y + (1-dv) * (h-1);
    fl_color(50, 50, 50);
    fl_line(x, yg, x+w-1, yg);

    char textbuf[16];
    snprintf(textbuf, sizeof(textbuf), "%g dB", g);
    textbuf[sizeof(textbuf)-1] = 0;

    fl_font(FL_COURIER, 10);
    fl_color(200, 200, 200);
    fl_draw(textbuf, lastx-2, yg, 0, 0, FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM, nullptr, 0);
    fl_draw(textbuf, x+2, yg, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_BOTTOM, nullptr, 0);
  }
}

void W_DftSpectrogram::Impl::draw_data() {
  int x, y, w, h;
  screen_dims(&x, &y, &w, &h);
  float *dispvalues = this->dispvalues.get();

  fl_push_clip(x, y, w, h);
  fl_color(51, 204, 179);

  int lasty {};
  for (int i = 0; i < w; ++i) {
    float v = dispvalues[i];
    int newy = (1-v)*(h-1);
    if (i > 0)
      fl_line(i-1, lasty, i, newy);
    lasty = newy;
  }

  fl_pop_clip();
}

void W_DftSpectrogram::Impl::draw_pointer(int mx, int my) {
  int x, y, w, h;
  this->screen_dims(&x, &y, &w, &h);

  float f_nyq = this->fsref / 2;
  float f = mx * f_nyq / (w-1);

  float dbmin = this->dbmin;
  float dbmax = this->dbmax;
  float dv = 1-float(my-y)/(h-1);
  float g = dbmin + dv * (dbmax - dbmin);

  fl_color(150, 150, 150);
  fl_line(mx, y, mx, y+h-1);
  fl_line(x, my, x+w-1, my);

  char textbuf[64];
  snprintf(textbuf, sizeof(textbuf), "%.2f Hz %.2f dB", f, g);
  textbuf[sizeof(textbuf)-1] = 0;

  fl_font(FL_COURIER, 12);
  int tw = 8+std::ceil(fl_width("00000.00 Hz -000.00 dB"));
  int th = 16;
  int tx = x+w-1-tw-4;
  int ty = y+4;
  int tal = FL_ALIGN_LEFT|FL_ALIGN_CENTER;
  fl_color(50, 50, 50);
  fl_rectf(tx, ty, tw, th);
  fl_color(200, 200, 200);
  fl_rect(tx, ty, tw, th);
  fl_draw(textbuf, tx+4, ty, tw, th, tal, nullptr, 0);
}

void W_DftSpectrogram::Impl::screen_dims(int *px, int *py, int *pw, int *ph) {
  int x = this->x;
  int y = this->y;
  int w = this->w;
  int h = this->h;
  // remove top margin
  if (this->rulertop) {
    y += mh;
    h -= mh;
  }
  // remove bottom margin
  if (this->rulerbtm) {
    h -= mh;
  }
  if (px) *px = x;
  if (py) *py = y;
  if (pw) *pw = w;
  if (ph) *ph = h;
}
