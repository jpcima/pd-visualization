#include "w_dft_waterfall.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <algorithm>
#include <memory>
#include <cmath>
#include <stdint.h>
#include <string.h>

struct W_DftWaterfall::Impl {
  W_DftWaterfall *Q = nullptr;

  float fsref = 44100;

  int mx = -1, my = -1;

  static constexpr unsigned pixsize = 4;
  unsigned stride {};

  Fl_Box *rulertop = nullptr;
  Fl_Box *rulerbtm = nullptr;
  Fl_Box *screen = nullptr;

  std::unique_ptr<uint8_t[]> imagebuf;
  unsigned imagew = 0;
  unsigned imageh = 0;
  unsigned imagerow = 0;

  float dbmin = -200;
  float dbmax = +0;

  // data
  void draw_rulers();
  void draw_data();
  void draw_pointer(int x, int y);

  uint8_t *row(unsigned i);
  uint8_t *current_top_row();
  uint8_t *current_btm_row(bool mirror = false);
  void advance_row(unsigned n);

  bool imagebuf_valid() const;
  void adapt_imagebuf();
};

// margins
static constexpr int /*mw = 20,*/ mh = 20;

W_DftWaterfall::W_DftWaterfall(int x, int y, int w, int h)
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

  P->adapt_imagebuf();
}

W_DftWaterfall::~W_DftWaterfall() {
}

void W_DftWaterfall::update_dft_data(
    const std::complex<float> *spec[], unsigned n, float fs, unsigned nch) {
  if (n == 0 || nch == 0)
    return;

  const unsigned sw = P->screen->w();
  const unsigned sh = P->screen->h();
  if (sw == 0 || sh == 0)
    return;

  if (!P->imagebuf_valid())
    return;

  const float dbmin = P->dbmin;
  const float dbmax = P->dbmax;
  const float fsref = P->fsref;

  const unsigned pixsize = P->pixsize;

  P->advance_row(1);
  uint8_t *row = P->current_btm_row();

  for (unsigned i = 0; i < sw; ++i) {
    unsigned red = 0;
    unsigned green = 0;
    unsigned blue = 0;

    for (unsigned c = 0; c < nch; ++c) {
      float rx = i / float(sw-1);
      int speci = std::lrint(rx * (n-1) * (fsref / fs));

      // no interpolation
      float a = std::abs(spec[c][speci]);
      float g = (a > 0.0f) ? (20 * std::log10(a)) : dbmin;

      // value display
      float dv = (g - dbmin) / (dbmax - dbmin);
      dv = std::min(dv, 1.0f);
      dv = std::max(dv, 0.0f);

      const float logt = 50;
      float logdv = (std::pow(logt, dv) - 1) / (logt - 1);

      const unsigned hue = (170 + c * 130) % 360;
      color::hsl<float> hcol{(float)hue, 50, logdv * 100};
      color::rgb<uint8_t> col(hcol);

      red += color::get::red(col);
      green += color::get::green(col);
      blue += color::get::blue(col);
    }

    uint8_t *pix = &row[i*pixsize];
    pix[0] = std::min(255u, red);
    pix[1] = std::min(255u, green);
    pix[2] = std::min(255u, blue);
    pix[3] = 0;
  }
  memcpy(P->current_btm_row(true), row, P->stride);
}

void W_DftWaterfall::reset_data() {
  if (!P->imagebuf_valid())
    return;

  unsigned sh = P->screen->h();
  memset(P->imagebuf.get(), 0, sh*2*P->stride);
}

void W_DftWaterfall::draw() {
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
    P->draw_data();
    //
    int mx = P->mx;
    int my = P->my;
    if (mx >= sx && mx < sx+sw && my >= sy && my < sy+sh)
      P->draw_pointer(mx, my);
    fl_pop_clip();
  }
}

int W_DftWaterfall::handle(int event) {
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

void W_DftWaterfall::resize(int x, int y, int w, int h) {
  W_DftVisu::resize(x, y, w, h);
  P->adapt_imagebuf();
}

void W_DftWaterfall::Impl::draw_rulers() {
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

void W_DftWaterfall::Impl::draw_data() {
  int sx = this->screen->x();
  int sy = this->screen->y();
  int sw = this->screen->w();
  int sh = this->screen->h();

  if (!this->imagebuf_valid()) {
    fl_color(0, 0, 0);
    fl_rectf(sx, sy, sw, sh);
    return;
  }

  unsigned pixsize = this->pixsize;
  unsigned stride = this->stride;

  uint8_t *top = this->current_top_row();
  fl_draw_image(top, sx, sy, sw, sh, pixsize, stride);
}

void W_DftWaterfall::Impl::draw_pointer(int mx, int my) {
  int sx = this->screen->x();
  int sy = this->screen->y();
  int sw = this->screen->w();
  int sh = this->screen->h();

  float f_nyq = this->fsref / 2;
  float f = (mx-sx) * f_nyq / (sw-1);

  fl_color(150, 150, 150);
  fl_line(mx, sy, mx, sy+sh-1);

  char textbuf[32];
  snprintf(textbuf, sizeof(textbuf), "%.2f Hz", f);
  textbuf[sizeof(textbuf)-1] = 0;

  fl_font(FL_COURIER, 12);
  int tw = 8+std::ceil(fl_width("00000.00 Hz"));
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

uint8_t *W_DftWaterfall::Impl::row(unsigned i) {
  return &this->imagebuf[i*this->stride];
}

uint8_t *W_DftWaterfall::Impl::current_top_row() {
  return this->row(this->imagerow);
}

uint8_t *W_DftWaterfall::Impl::current_btm_row(bool mirror) {
  unsigned sh = this->screen->h();
  unsigned rowindex = this->imagerow+sh-1;
  if (mirror) {
    rowindex += sh;
    if (rowindex >= 2 * sh)
      rowindex -= 2 * sh;
  }
  return this->row(rowindex);
}

void W_DftWaterfall::Impl::advance_row(unsigned n) {
  unsigned sh = this->screen->h();
  unsigned imagerow = this->imagerow + n;
  if (imagerow >= sh)
    imagerow -= sh;
  this->imagerow = imagerow;
}

bool W_DftWaterfall::Impl::imagebuf_valid() const {
  unsigned sw = this->screen->w();
  unsigned sh = this->screen->h();
  return this->imagew == sw && this->imageh == sh;
}

void W_DftWaterfall::Impl::adapt_imagebuf() {
  if (this->imagebuf_valid())
    return;
  unsigned sw = this->screen->w();
  unsigned sh = this->screen->h();
  unsigned stride = sw * this->pixsize;
  this->imagebuf.reset(new uint8_t[sh * 2 * stride]());
  this->stride = stride;
  this->imagew = sw;
  this->imageh = sh;
  this->imagerow = 0;
}
