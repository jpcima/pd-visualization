
cflags = $(all_cflags) $(sys_cflags)
ldlibs = $(all_ldlibs) $(sys_ldlibs)
ldflags = $(all_ldflags) $(sys_ldflags)

gui.objects = $(filter %.o,$(patsubst %.cc,%.o,$(gui.sources)))

$(gui.objects): %.o: %.cc
	$(CXX) -O2 -c -o $@ $(all_cflags) $(fltk_cflags) $(fftwf_cflags) $(sys_cflags) $<

visu~-gui$(exe_ext): $(gui.objects)
	$(CXX) -o $@ $(sys_ldflags) $^ $(fltk_ldlibs) $(fftwf_ldlibs) $(sys_ldlibs)

clean-gui:
	rm -f visu~-gui$(exe_ext) $(gui.objects)
.PHONY: clean-gui

all: visu~-gui$(exe_ext)
clean: clean-gui
