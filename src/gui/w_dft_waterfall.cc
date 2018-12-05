#include "w_dft_waterfall.h"
#include "gui/fl_util.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <algorithm>
#include <memory>
#include <cmath>
#include <stdint.h>
#include <string.h>

typedef std::complex<float> cfloat;

struct W_DftWaterfall::Impl {
  W_DftWaterfall *Q = nullptr;

  float fsref = 44100;

  float frequency_of_x(float x) const;
  float frequency_of_r(float r) const;
  float r_of_frequency(float f) const;
  float x_of_frequency(float f) const;

  float nth_frequency_mark(unsigned i) const;
  int height_of_mark(unsigned i) const;

  enum {
    Domain_Linear,
    Domain_Logarithmic,
  };
  int fdomain = Domain_Linear;

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

  // controls
  Fl_Group *grpctl = nullptr;
  Fl_Button *btnexpand = nullptr;
  int grpw = 0;
  int grph = 0;

  void create_controls(bool expanded);
  void reposition_controls();
  void on_expand_controls();
  void on_unexpand_controls();

  void changed_fdomain(int val);
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

  P->create_controls(false);
  P->reposition_controls();

  this->end();

  P->adapt_imagebuf();
}

W_DftWaterfall::~W_DftWaterfall() {
}

void W_DftWaterfall::update_dft_data(
    const cfloat *allspec[], unsigned n, float fs, unsigned nch) {
  if (n == 0 || nch == 0)
    return;

  const unsigned dftsize = (n - 1) * 2;

  const unsigned sx = P->screen->x();
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
      const cfloat *spec = allspec[c];

      float f = P->frequency_of_x(sx + i);
      float binnum = f * dftsize / fs;
      unsigned binidx = (unsigned)binnum;
      float mu = binnum - binidx;

      constexpr unsigned itp = 2;
      float a[itp] = {};

      for (unsigned j = 0; j < itp; ++j) {
        cfloat bin = 0.0f;
        if (binidx + j < n)
          bin = spec[binidx + j];
        a[j] = std::abs(bin);
      }

      float g = dbmin;
      bool gvalid = true;
      for (unsigned j = 0; gvalid && j < itp; ++j)
        gvalid = a[j] > 0;
      if (gvalid) {
        float y[itp];
        for (unsigned j = 0; j < itp; ++j)
          y[j] = 20 * std::log10(a[j]);

        switch (itp) {
        case 1:
          g = y[0];
          break;
        case 2:
          g = y[0] * (1 - mu) + y[1] * mu;
          break;
        case 4: {
          float c[4];
          if (0) {  // Cubic
            c[0] = y[3] - y[2] - y[0] + y[1];
            c[1] = y[0] - y[1] - c[0];
            c[2] = y[2] - y[0];
            c[3] = y[1];
          }
          else {  // Hermite
            c[0] = -0.5f * y[0] + 1.5f * y[1] - 1.5f * y[2] + 0.5f * y[3];
            c[1] = y[0] - 2.5f * y[1] + 2 * y[2] - 0.5f * y[3];
            c[2] = -0.5f * y[0] + 0.5f * y[2];
            c[3] = y[1];
          }
          g = c[0] * (mu * mu * mu) + c[1] * (mu * mu) + c[2] * mu + c[3];
          break;
        }
        }
      }

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

VisuDftResolution W_DftWaterfall::desired_resolution() const {
  return VisuDftResolution::Medium;
}

void W_DftWaterfall::reset_data() {
  if (!P->imagebuf_valid())
    return;

  unsigned sh = P->screen->h();
  memset(P->imagebuf.get(), 0, sh*2*P->stride);
}

void W_DftWaterfall::draw() {
  draw_child(*P->rulertop);
  draw_child(*P->rulerbtm);
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

  //
  draw_child(*P->grpctl);
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
  P->reposition_controls();
}

float W_DftWaterfall::Impl::frequency_of_x(float x) const {
  return frequency_of_r((x - Q->x()) / Q->w());
}

float W_DftWaterfall::Impl::frequency_of_r(float r) const {
  float fmax = 0.5f * fsref;
  switch (fdomain) {
  default:
  case Domain_Linear:
    return r * fmax;
  case Domain_Logarithmic:
    float fmin = 10;
    float lfmin = std::log10(fmin);
    float lfmax = std::log10(fmax);
    float lf = lfmin + (lfmax - lfmin) * r;
    return std::pow(10.0f, lf);
  }
}

float W_DftWaterfall::Impl::r_of_frequency(float f) const {
  float fmax = 0.5f * fsref;
  switch (fdomain) {
  default:
  case Domain_Linear:
    return f / fmax;
  case Domain_Logarithmic:
    float lf = std::log10(f);
    float fmin = 10;
    float lfmin = std::log10(fmin);
    float lfmax = std::log10(fmax);
    return (lf - lfmin) / (lfmax - lfmin);
  }
}

float W_DftWaterfall::Impl::x_of_frequency(float f) const {
  return Q->x() + r_of_frequency(f) * Q->w();
}

float W_DftWaterfall::Impl::nth_frequency_mark(unsigned i) const
{
    switch (fdomain) {
    default:
    case Domain_Linear:
      return 250 * i;
    case Domain_Logarithmic: {
      const float m[] = {1.0, 2.5, 5.0};
      const unsigned n = 3;
      return m[i % n] * std::pow<float>(10, (i + n) / n);
    }
    }
}

int W_DftWaterfall::Impl::height_of_mark(unsigned i) const {
    switch (fdomain) {
    default:
    case Domain_Linear: {
      const int gradh[] = {16, 6, 8, 6};
      return gradh[i%4];
    }
    case Domain_Logarithmic: {
      const int gradh[] = {16, 6, 8};
      return gradh[i%3];
    }
    }
}

void W_DftWaterfall::Impl::draw_rulers() {
  int x = Q->x();
  int y = Q->y();
  int w = Q->w();
  int h = Q->h();

  fl_color(0, 0, 0);

  float f_nyq = 0.5f * this->fsref;
  for (unsigned i = 0; ; ++i) {
    float f = nth_frequency_mark(i);
    unsigned g = (fdomain == Domain_Logarithmic) ? 1 : 4;
    if (f > f_nyq)
      break;

    int xf = x_of_frequency(f);

    int l = height_of_mark(i);
    if (this->rulertop)
      fl_line(xf, y+mh-l, xf, y+mh-1);
    if (this->rulerbtm)
      fl_line(xf, y+h-mh, xf, y+h-mh+l);

    char textbuf[32];
    if (f < 1000)
      snprintf(textbuf, sizeof(textbuf), "%g", f);
    else
      snprintf(textbuf, sizeof(textbuf), "%gk", f/1000);
    textbuf[sizeof(textbuf)-1] = 0;

    fl_font(FL_COURIER, 10);
    if (i%g == 0) {
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

  float f = frequency_of_x(mx);

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

void W_DftWaterfall::Impl::create_controls(bool expanded) {
  int mh = 60;
  int bh = 20;

  bool btnfocus = this->btnexpand == Fl::focus();

  delete this->grpctl;
  this->grpctl = nullptr;
  this->btnexpand = nullptr;

  if (!expanded) {
    int grpw = this->grpw = bh;
    int grph = this->grph = bh;
    Fl_Group *grpctl = this->grpctl = new Fl_Group(0, 0, grpw, grph);
    grpctl->begin();
    grpctl->resizable(nullptr);
    Fl_Button *btn = this->btnexpand = new Fl_Button(0, 0, grpw, grph, "+");
    if (btnfocus) btn->take_focus();
    btn->labelfont(FL_COURIER|FL_BOLD);
    btn->labelsize(16);
    TRIGGER_CALLBACK(btn, this, on_expand_controls);
    grpctl->end();
    return;
  }

  Fl_Group *grpctl = this->grpctl = new Fl_Group(0, 0, 1, mh);
  grpctl->begin();
  grpctl->resizable(nullptr);

  int interx = 8;
  int curx = 0;

  Fl_Group *box {};
  Fl_Button *btn {};
  Fl_Choice *choice {};

  int knobw = 42;
  int knobh = 42;

  box = new Fl_Group(curx, 0, 1, mh);
  box->begin();
  box->resizable(nullptr);
  box->color(fl_rgb_color(191, 218, 255));
  box->box(FL_ENGRAVED_BOX);
  curx += interx;

  choice = new Fl_Choice(curx, 15, 120, knobh, "F. domain");
  choice->labelsize(10);
  choice->align(FL_ALIGN_TOP);
  choice->textfont(FL_COURIER);
  choice->textsize(12);
  choice->add("Linear");
  choice->add("Logarithmic");
  choice->value(this->fdomain);
  VALUE_CALLBACK(choice, this, changed_fdomain);
  curx += choice->w() + interx;

  box->size(curx - box->x(), mh);
  box->end();

  box = new Fl_Group(curx, 0, mh, mh);
  box->begin();
  // box->color(fl_rgb_color(191, 218, 255));
  box->box(FL_ENGRAVED_BOX);
  btn = this->btnexpand = new Fl_Button(curx, mh-bh, bh, bh, "-");
  if (btnfocus) btn->take_focus();
  btn->labelfont(FL_COURIER|FL_BOLD);
  btn->labelsize(16);
  TRIGGER_CALLBACK(btn, this, on_unexpand_controls);
  curx += btn->w();
  box->end();

  int grpw = this->grpw = curx;
  int grph = this->grph = mh;
  grpctl->size(grpw, grph);

  grpctl->end();
}

void W_DftWaterfall::Impl::reposition_controls() {
  int x = Q->x();
  int y = Q->y();
  int w = Q->w();
  int h = Q->h();
  Fl_Group *grpctl = this->grpctl;
  grpctl->resize(x+w-this->grpw, y+h-this->grph, this->grpw, this->grph);

  int wrbtm = this->rulertop->w() - this->grpw;
  if (this->rulerbtm->w() != wrbtm) {
    this->rulerbtm->size(wrbtm, this->rulerbtm->h());
    Q->redraw();
  }
}

void W_DftWaterfall::Impl::on_expand_controls() {
  Q->begin();
  this->create_controls(true);
  this->reposition_controls();
  Q->end();
}

void W_DftWaterfall::Impl::on_unexpand_controls() {
  Q->begin();
  this->create_controls(false);
  this->reposition_controls();
  Q->end();
}

void W_DftWaterfall::Impl::changed_fdomain(int val) {
  this->fdomain = val;
  Q->redraw();
}
