#include "w_dft_waterfall.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <memory>
#include <cmath>
#include <stdint.h>
#include <string.h>

struct W_DftWaterfall::Impl {
  float fsref = 44100;

  int x {}, y {}, w {}, h {};

  int mx = -1, my = -1;

  unsigned pixsize {};
  unsigned stride {};

  Fl_Box *rulertop = nullptr;
  Fl_Box *rulerbtm = nullptr;

  std::unique_ptr<uint8_t[]> imagebuf;
  unsigned imagerow = 0;

  float dbmin = -200;
  float dbmax = +0;

  // data
  void draw_rulers();
  void draw_data();
  void draw_pointer(int x, int y);

  void screen_dims(int *px, int *py, int *pw, int *ph);

  uint8_t *row(unsigned i);
  uint8_t *current_top_row();
  uint8_t *current_btm_row(bool mirror = false);
  void advance_row(unsigned n);
};

// margins
static constexpr int mw = 20;
static constexpr int mh = 20;

W_DftWaterfall::W_DftWaterfall(int x, int y, int w, int h)
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

  int dw, dh;
  P->screen_dims(nullptr, nullptr, &dw, &dh);

  P->pixsize = 4;
  P->stride = dw * P->pixsize;
  P->imagebuf.reset(new uint8_t[dh*2*P->stride]());
}

W_DftWaterfall::~W_DftWaterfall() {
}

void W_DftWaterfall::update_dft_data(
    const std::complex<float> *spec, unsigned n, float fs) {
  float nyq = fs/2;
  float dbmin = P->dbmin;
  float dbmax = P->dbmax;

  int w, h;
  P->screen_dims(0, 0, &w, &h);

  unsigned pixsize = P->pixsize;

  P->advance_row(1);
  uint8_t *row = P->current_btm_row();
  for (int i = 0; i < w; ++i) {
    float rx = i / float(w-1);
    int speci = std::lrint(rx * (n-1));

    // no interpolation
    float a = std::abs(spec[speci]);

    float g = (a > 0.0f) ? (20 * std::log10(a)) : dbmin;

    // value display
    float dv = (g - dbmin) / (dbmax - dbmin);
    dv = std::min(dv, 1.0f);
    dv = std::max(dv, 0.0f);

    const float logt = 50;
    float logdv = (std::pow(logt, dv) - 1) / (logt - 1);

    color::hsl<float> hcol {170, 50, logdv * 100};
    color::rgb<uint8_t> col(hcol);

    uint8_t *pix = &row[i*pixsize];
    pix[0] = color::get::red(col);
    pix[1] = color::get::green(col);
    pix[2] = color::get::blue(col);
    pix[3] = 0;
  }
  memcpy(P->current_btm_row(true), row, P->stride);
}

void W_DftWaterfall::reset_data() {
  int h;
  P->screen_dims(nullptr, nullptr, nullptr, &h);
  memset(P->imagebuf.get(), 0, h*2*P->stride);
}

void W_DftWaterfall::draw() {
  Fl_Group::draw();
  //
  P->draw_rulers();
  //
  int x, y, w, h;
  P->screen_dims(&x, &y, &w, &h);
  fl_push_clip(x, y, w, h);
  P->draw_data();
  //
  int mx = P->mx;
  int my = P->my;
  if (mx >= x && mx < x+w && my >= y && my < y+h)
    P->draw_pointer(mx, my);
  fl_pop_clip();
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

void W_DftWaterfall::Impl::draw_rulers() {
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

void W_DftWaterfall::Impl::draw_data() {
  int x, y, w, h;
  screen_dims(&x, &y, &w, &h);

  unsigned pixsize = this->pixsize;
  unsigned stride = this->stride;

  uint8_t *top = this->current_top_row();
  fl_draw_image(top, x, y, w, h, pixsize, stride);
}

void W_DftWaterfall::Impl::draw_pointer(int mx, int my) {
  int x, y, w, h;
  this->screen_dims(&x, &y, &w, &h);

  float f_nyq = this->fsref / 2;
  float f = mx * f_nyq / (w-1);

  fl_color(150, 150, 150);
  fl_line(mx, y, mx, y+h-1);

  char textbuf[32];
  snprintf(textbuf, sizeof(textbuf), "%.2f Hz", f);
  textbuf[sizeof(textbuf)-1] = 0;

  fl_font(FL_COURIER, 12);
  int tw = 8+std::ceil(fl_width("00000.00 Hz"));
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

void W_DftWaterfall::Impl::screen_dims(int *px, int *py, int *pw, int *ph) {
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

uint8_t *W_DftWaterfall::Impl::row(unsigned i) {
  return &this->imagebuf[i*this->stride];
}

uint8_t *W_DftWaterfall::Impl::current_top_row() {
  return this->row(this->imagerow);
}

uint8_t *W_DftWaterfall::Impl::current_btm_row(bool mirror) {
  int dh;
  this->screen_dims(nullptr, nullptr, nullptr, &dh);
  int rowindex = this->imagerow+dh-1;
  if (mirror)
    rowindex = (rowindex + dh) % (2 * dh);
  return this->row(rowindex);
}

void W_DftWaterfall::Impl::advance_row(unsigned n) {
  int dh;
  this->screen_dims(nullptr, nullptr, nullptr, &dh);
  this->imagerow = (this->imagerow + n) % dh;
}
