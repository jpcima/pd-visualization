#include "w_dft_spectrogram.h"
#include "gui/fl_util.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>
#include <color/color.hpp>
#include <algorithm>
#include <vector>
#include <complex>
#include <cmath>
#include <stdint.h>
#include <string.h>

typedef std::complex<float> cfloat;

struct W_DftSpectrogram::Impl {
  W_DftSpectrogram *Q = nullptr;

  static constexpr float fsref = 44100;
  float fs = 44100;

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
  int fdomain = Domain_Logarithmic;

  int mx = -1, my = -1;

  std::vector<std::complex<float>> spec;
  unsigned channels = 0;

  Fl_Box *rulertop = nullptr;
  Fl_Box *rulerbtm = nullptr;
  Fl_Box *screen = nullptr;

  float dbmin = -140;
  float dbmax = +20;

  // data
  void draw_rulers();
  void draw_back();
  void draw_data();
  void draw_pointer(int x, int y);

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

  P->create_controls(false);
  P->reposition_controls();

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

VisuDftResolution W_DftSpectrogram::desired_resolution() const {
  return (P->fdomain == Impl::Domain_Logarithmic) ?
    VisuDftResolution::High : VisuDftResolution::Medium;
}

void W_DftSpectrogram::reset_data() {
  P->spec.clear();
  P->channels = 0;
}

void W_DftSpectrogram::draw() {
  //
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
    P->draw_back();
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

void W_DftSpectrogram::resize(int x, int y, int w, int h) {
  W_DftVisu::resize(x, y, w, h);
  P->reposition_controls();
}

float W_DftSpectrogram::Impl::frequency_of_x(float x) const {
  return frequency_of_r((x - Q->x()) / Q->w());
}

float W_DftSpectrogram::Impl::frequency_of_r(float r) const {
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

float W_DftSpectrogram::Impl::r_of_frequency(float f) const {
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

float W_DftSpectrogram::Impl::x_of_frequency(float f) const {
  return Q->x() + r_of_frequency(f) * Q->w();
}

float W_DftSpectrogram::Impl::nth_frequency_mark(unsigned i) const
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

int W_DftSpectrogram::Impl::height_of_mark(unsigned i) const {
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

void W_DftSpectrogram::Impl::draw_rulers() {
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

void W_DftSpectrogram::Impl::draw_back() {
  int sx = this->screen->x();
  int sy = this->screen->y();
  int sw = this->screen->w();
  int sh = this->screen->h();

  fl_color(0, 0, 0);
  fl_rectf(sx, sy, sw, sh);

  float f_nyq = this->fsref / 2;
  //float f_interval = 1000;

  for (unsigned i = 1; ; ++i) {
    // float f = f_interval * i;
    float f = nth_frequency_mark(i);
    unsigned g = (fdomain == Domain_Logarithmic) ? 1 : 4;
    if (f > f_nyq)
      break;

    if (i % g == 0) {
      int xf = x_of_frequency(f);
      fl_color(50, 50, 50);
      fl_line(xf, sy, xf, sy+sh-1);
    }
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
    fl_draw(textbuf, sx+2, yg, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_BOTTOM, nullptr, 0);
    fl_draw(textbuf, sx+sw-2, yg, 0, 0, FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM, nullptr, 0);
  }
}

void W_DftSpectrogram::Impl::draw_data() {
  const unsigned channels = this->channels;
  if (channels == 0)
    return;

  const unsigned specn = this->spec.size() / channels;
  if (specn == 0)
    return;
  const unsigned dftsize = (specn - 1) * 2;

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

      float f = frequency_of_x(sx + i);
      float binnum = f * dftsize / fs;
      unsigned binidx = (unsigned)binnum;
      float mu = binnum - binidx;

      constexpr unsigned itp = 4;
      float a[itp] = {};

      for (unsigned j = 0; j < itp; ++j) {
        cfloat bin = 0.0f;
        if (binidx + j < specn)
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

  float f = frequency_of_x(mx);

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

void W_DftSpectrogram::Impl::create_controls(bool expanded) {
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

void W_DftSpectrogram::Impl::reposition_controls() {
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

void W_DftSpectrogram::Impl::on_expand_controls() {
  Q->begin();
  this->create_controls(true);
  this->reposition_controls();
  Q->end();
}

void W_DftSpectrogram::Impl::on_unexpand_controls() {
  Q->begin();
  this->create_controls(false);
  this->reposition_controls();
  Q->end();
}

void W_DftSpectrogram::Impl::changed_fdomain(int val) {
  this->fdomain = val;
  Q->redraw();
}
