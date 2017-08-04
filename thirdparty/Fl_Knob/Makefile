CC = gcc
CXX = g++
AR = ar
LD = g++
FLUID = fluid

TARGET = Test
INCFLAGS = -I.
CPPFLAGS = -O2 $(INCFLAGS)
CXXFLAGS = -O2 $(INCFLAGS)
ARFLAGS = -ru
OBJS = Fl_Knob/Fl_Knob.o Test.o

all : $(TARGET)

.SUFFIXES:	.cxx .H .o .fld

.fld.cxx:
	$(FLUID) -c $<
.cxx.o :
	$(CXX) $(CXXFLAGS) -c $< -o $(<:.cxx=.o)

$(TARGET) : $(OBJS)
	$(LD) -o $@ $(OBJS) -L/usr/X11/lib -L/usr/local/lib -lfltk -lX11 -lm

clean:
	-rm -f $(OBJS) $(TARGET)
