#include "w_ts_oscillogram.h"
#include "fl_util.h"
#include "s_math.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <Fl_Knob/Fl_Knob.H>
#include <FL/fl_draw.H>
#include <vector>

struct W_TsOscillogram::Impl {
  W_TsOscillogram *Q = nullptr;

  int mx = -1, my = -1;

  float samplerate = 44100;
  float period = 100e-3;
  float scale = 1;
  float yoff = 0;

  static constexpr float periodmin = 1e-3;
  static constexpr float periodmax = 1;

  static constexpr float scalemin = 0.4;
  static constexpr float scalemax = 4;

  std::vector<float> timedata;

  Fl_Group *grpctl = nullptr;
  int grpw = 0;
  int grph = 0;

  void create_controls();
  void reposition_controls();
  void changed_tb(float val);
  void changed_sc(float val);
  void changed_yoff(float val);

  class Screen;
  Screen *screen = nullptr;
};

class W_TsOscillogram::Impl::Screen : public Fl_Widget {
 public:
  Screen(Impl *P, int x, int y, int w, int h)
      : Fl_Widget(x, y, w, h), P(P) {}
  void draw() override;
  int handle(int event) override;
 private:
  void draw_back();
  void draw_data();
  void draw_pointer(int x, int y);
  Impl *P = nullptr;
};

W_TsOscillogram::W_TsOscillogram(int x, int y, int w, int h)
    : W_TsVisu(x, y, w, h),
      P(new Impl) {
  P->Q = this;

  int sx = x;
  int sy = y;
  int sw = w;
  int sh = h;

  P->screen = new Impl::Screen(P.get(), sx, sy, sw, sh);
  this->resizable(P->screen);

  P->create_controls();
  P->reposition_controls();

  this->end();
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

int W_TsOscillogram::Impl::Screen::handle(int event) {
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

  return Fl_Widget::handle(event);
}

void W_TsOscillogram::Impl::Screen::draw() {
  int sx = P->screen->x();
  int sy = P->screen->y();
  int sw = P->screen->w();
  int sh = P->screen->h();
  if (sw > 0 && sh > 0) {
    fl_push_clip(sx, sy, sw, sh);
    //
    this->draw_back();
    this->draw_data();
    //
    int mx = P->mx;
    int my = P->my;
    if (mx >= sx && mx < sx+sw && my >= sy && my < sy+sh)
      this->draw_pointer(mx, my);
    fl_pop_clip();
  }
}

void W_TsOscillogram::Impl::Screen::draw_back() {
  int sx = this->x();
  int sy = this->y();
  int sw = this->w();
  int sh = this->h();

  float tb = P->period;
  float sc = P->scale;
  float yoff = P->yoff;

  fl_color(0, 0, 0);
  fl_rectf(sx, sy, sw, sh);

  for (unsigned i = 0; ; ++i) {
    const float vinterval = 0.25;

    float dv = vinterval * i;
    int yv = sy + (sh-1) * (- yoff - dv * sc + 1) / 2;

    if (i == 0) {
      fl_color(100, 100, 100);
      fl_line(sx, yv, sx+sw-1, yv);
    } else {
      float dv2 = -dv;
      int yv2 = sy + (sh-1) * (- yoff - dv2 * sc + 1) / 2;
      if ((std::abs(yv) < 0 || std::abs(yv) >= sh) &&
          (std::abs(yv2) < 0 || std::abs(yv2) >= sh))
        break;
      fl_color(50, 50, 50);
      fl_line(sx, yv, sx+sw-1, yv);
      fl_line(sx, yv2, sx+sw-1, yv2);
    }
  }

  constexpr unsigned xdivs = 20;
  for (unsigned i = 1; i < xdivs; ++i) {
    int xi = sx + sw * i / float(xdivs);
    fl_color(50, 50, 50);
    fl_line(xi, sy, xi, sy+sh-1);
  }

  fl_font(FL_COURIER, 12);
  fl_color(200, 200, 200);

  char textbuf[64];
  if (tb < 1)
    snprintf(textbuf, sizeof(textbuf), "X = %g ms", tb * 1e3);
  else
    snprintf(textbuf, sizeof(textbuf), "X = %g s", tb);
  textbuf[sizeof(textbuf)-1] = 0;
  fl_draw(textbuf, sx+2, sy+2, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_TOP, nullptr, 0);

  snprintf(textbuf, sizeof(textbuf), "Y = %g V", 2 / sc);
  textbuf[sizeof(textbuf)-1] = 0;
  fl_draw(textbuf, sx+2, sy+16, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_TOP, nullptr, 0);
}

void W_TsOscillogram::Impl::Screen::draw_data() {
  int sx = this->x();
  int sy = this->y();
  int sw = this->w();
  int sh = this->h();

  const float *p = P->timedata.data();
  unsigned n = P->timedata.size();

  if (n == 0)
    return;

  fl_color(255, 0, 0);

  float sr = P->samplerate;
  float tb = P->period;
  float sc = P->scale;
  float yoff = P->yoff;

  int xi {}, yi {};

  for (int i = 0; i < sw; ++i) {
    float r = float(sw-1-i) / (sw-1);
    float idf = (n - 1) - r * tb * sr;

    int id0 = idf;
    float mu = idf - id0;

    constexpr int nsmp = 4;
    float ismp[nsmp];
    for (int i = 0; i < nsmp; ++i)
      ismp[i] = (id0 + i < 0 || unsigned(id0 + i) >= n) ? 0 : p[id0 + i];

    // float smp = interp_linear(ismp, mu);
    float smp = interp_catmull(ismp, mu);

    int oldxi = xi;
    int oldyi = yi;
    xi = sx + i;
    yi = sy + (sh-1) * (- yoff - smp * sc + 1) / 2;

    if (i > 0)
      fl_line(oldxi, oldyi, xi, yi);
  }
}

void W_TsOscillogram::Impl::Screen::draw_pointer(int mx, int my) {
  int sx = this->x();
  int sy = this->y();
  int sw = this->w();
  int sh = this->h();

  float tb = P->period;
  float sc = P->scale;
  float yoff = P->yoff;

  float t = tb * (mx-sx) / (sw-1);
  float v = (1 - yoff - 2*float(my-sy)/(sh-1)) / sc;

  fl_color(150, 150, 150);
  fl_line(mx, sy, mx, sy+sh-1);
  fl_line(sx, my, sx+sw-1, my);

  char textbuf[64];
  if (t < 1)
    snprintf(textbuf, sizeof(textbuf), "%.2f ms %.2f V", t * 1e3f, v);
  else
    snprintf(textbuf, sizeof(textbuf), "%.2f s %.2f V", t, v);
  textbuf[sizeof(textbuf)-1] = 0;

  fl_font(FL_COURIER, 12);
  int tw = 8+std::ceil(fl_width("000.00 ms -0.00 V"));
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

void W_TsOscillogram::resize(int x, int y, int w, int h) {
  W_TsVisu::resize(x, y, w, h);
  P->reposition_controls();
}

void W_TsOscillogram::Impl::create_controls() {
  int mh = 60;

  Fl_Group *grpctl = this->grpctl = new Fl_Group(0, 0, 1, mh);
  grpctl->begin();
  grpctl->resizable(nullptr);
  grpctl->box(FL_ENGRAVED_BOX);
  grpctl->color(246);

  int interx = 8;
  int curx = interx;

  Fl_Knob *valtb = new Fl_Knob(curx, 15, 40, 40, "T. base");
  valtb->labelsize(10);
  valtb->align(FL_ALIGN_TOP);
  valtb->value(this->period / this->periodmax);
  VALUE_CALLBACK(valtb, this, changed_tb);
  curx += valtb->w() + interx;

  Fl_Knob *valsc = new Fl_Knob(curx, 15, 40, 40, "Y. scale");
  valsc->labelsize(10);
  valsc->align(FL_ALIGN_TOP);
  valsc->value(this->scale / this->scalemax);
  VALUE_CALLBACK(valsc, this, changed_sc);
  curx += valsc->w() + interx;

  Fl_Knob *valyoff = new Fl_Knob(curx, 15, 40, 40, "Y. offset");
  valyoff->labelsize(10);
  valyoff->align(FL_ALIGN_TOP);
  valyoff->range(-1, +1);
  valyoff->value(this->yoff);
  VALUE_CALLBACK(valyoff, this, changed_yoff);
  curx += valyoff->w() + interx;

  int grpw = this->grpw = curx;
  int grph = this->grph = mh;
  grpctl->size(grpw, grph);

  grpctl->end();
}

void W_TsOscillogram::Impl::reposition_controls() {
  int x = Q->x();
  int y = Q->y();
  int h = Q->h();
  Fl_Group *grpctl = this->grpctl;
  grpctl->resize(x, y+h-this->grph, this->grpw, this->grph);
}

void W_TsOscillogram::Impl::changed_tb(float val) {
  float tb = clamp(val * this->periodmax, this->periodmin, this->periodmax);
  this->period = tb;
  Q->redraw();
}

void W_TsOscillogram::Impl::changed_sc(float val) {
  float sc = clamp(val * this->scalemax, this->scalemin, this->scalemax);
  this->scale = sc;
  Q->redraw();
}

void W_TsOscillogram::Impl::changed_yoff(float val) {
  this->yoff = val;
  Q->redraw();
}
