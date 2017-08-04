// generated by Fast Light User Interface Designer (fluid) version 1.0010

#include "Test.h"
#include <math.h>

int main(int ac,char **av) {
  make_window();
Fl::run();
}

Fl_Knob *v1=(Fl_Knob *)0;

static void cb_v1(Fl_Knob*, void*) {
  o1->value(pow(10,v1->value()));
}

Fl_Value_Output *o1=(Fl_Value_Output *)0;

Fl_Knob *v2=(Fl_Knob *)0;

static void cb_v2(Fl_Knob*, void*) {
  o2->value(pow(10.0,v2->value()));
}

Fl_Knob *v3=(Fl_Knob *)0;

static void cb_v3(Fl_Knob*, void*) {
  o3->value(pow(10.0,v3->value()));
}

Fl_Value_Output *o2=(Fl_Value_Output *)0;

Fl_Value_Output *o3=(Fl_Value_Output *)0;

Fl_Knob *v4=(Fl_Knob *)0;

static void cb_v4(Fl_Knob*, void*) {
  o4->value(v4->value());
v5->scaleticks((int)(v4->value()));
}

Fl_Value_Output *o4=(Fl_Value_Output *)0;

Fl_Knob *v5=(Fl_Knob *)0;

static void cb_v5(Fl_Knob*, void*) {
  o5->value(v5->value());
v4->cursor(v5->value());
v6->cursor(v5->value());
}

Fl_Knob *v6=(Fl_Knob *)0;

static void cb_v6(Fl_Knob*, void*) {
  o6->value(pow(10.0,v6->value()));
}

Fl_Value_Output *o5=(Fl_Value_Output *)0;

Fl_Value_Output *o6=(Fl_Value_Output *)0;

Fl_Window* make_window() {
  Fl_Window* w;
  { Fl_Window* o = new Fl_Window(274, 211);
    w = o;
    o->labelsize(10);
    { Fl_Knob* o = v1 = new Fl_Knob(20, 12, 55, 58, "Fl_Knob::LINELOG_1");
      o->color(10);
      o->selection_color(1);
      o->labelsize(9);
      o->step(0.001);
      o->callback((Fl_Callback*)cb_v1);
      o->type(Fl_Knob::LINELOG_1);
    }
    { Fl_Value_Output* o = o1 = new Fl_Value_Output(30, 85, 40, 15);
      o->minimum(1);
      o->maximum(10);
      o->step(0.01);
      o->value(1);
      o->textsize(9);
    }
    { Fl_Group* o = new Fl_Group(95, 0, 175, 105);
      o->box(FL_FLAT_BOX);
      o->color(147);
      { Fl_Knob* o = v2 = new Fl_Knob(110, 15, 55, 56, "Fl_Knob::LINELOG_2");
        o->labelsize(9);
        o->labelcolor(7);
        o->maximum(2);
        o->step(0.001);
        o->callback((Fl_Callback*)cb_v2);
        o->align(FL_ALIGN_TOP);
        o->type(Fl_Knob::LINELOG_2);
      }
      { Fl_Knob* o = v3 = new Fl_Knob(195, 15, 55, 55, "Fl_Knob::DOTLOG_3");
        o->color(37);
        o->selection_color(7);
        o->labelsize(9);
        o->labelcolor(7);
        o->maximum(3);
        o->step(0.01);
        o->callback((Fl_Callback*)cb_v3);
        o->type(Fl_Knob::DOTLOG_3);
      }
      { Fl_Value_Output* o = o2 = new Fl_Value_Output(120, 85, 35, 15);
        o->minimum(1);
        o->maximum(100);
        o->step(0.05);
        o->value(1);
        o->textsize(9);
      }
      { Fl_Value_Output* o = o3 = new Fl_Value_Output(205, 85, 40, 15);
        o->minimum(1);
        o->maximum(1000);
        o->step(0.1);
        o->value(1);
        o->textsize(9);
      }
      o->end();
    }
    { Fl_Knob* o = v4 = new Fl_Knob(20, 116, 55, 59, "Num. of Ticks-->");
      o->color(230);
      o->selection_color(1);
      o->labelsize(9);
      o->maximum(31);
      o->step(1);
      o->callback((Fl_Callback*)cb_v4);
      o->type(Fl_Knob::LINELIN);
    }
    { Fl_Value_Output* o = o4 = new Fl_Value_Output(30, 190, 35, 15);
      o->maximum(100);
      o->step(0.01);
      o->textsize(9);
    }
    { Fl_Group* o = new Fl_Group(95, 110, 175, 100);
      o->box(FL_ENGRAVED_BOX);
      o->color(246);
      { Fl_Knob* o = v5 = new Fl_Knob(110, 120, 55, 55, "<--Cursor size-->");
        o->labelsize(9);
        o->maximum(100);
        o->step(1);
        o->value(20);
        o->callback((Fl_Callback*)cb_v5);
        o->type(Fl_Knob::DOTLIN);
      }
      { Fl_Knob* o = v6 = new Fl_Knob(195, 120, 55, 55, "Knob::DOTLOG_3");
        o->labelsize(9);
        o->maximum(3);
        o->step(0.01);
        o->callback((Fl_Callback*)cb_v6);
        o->type(Fl_Knob::DOTLOG_3);
      }
      { Fl_Value_Output* o = o5 = new Fl_Value_Output(120, 190, 35, 15);
        o->maximum(10);
        o->step(0.1);
        o->textsize(9);
      }
      { Fl_Value_Output* o = o6 = new Fl_Value_Output(210, 190, 35, 15);
        o->minimum(1);
        o->maximum(1000);
        o->step(0.1);
        o->value(1);
        o->textsize(9);
      }
      o->end();
    }
    o->show();
    o->end();
  }
  return w;
}
