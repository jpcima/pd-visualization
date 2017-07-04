# -*- mode: makefile; -*-
include mk/makefile-top.mk
sys_ldlibs = -lm -ldl
include mk/makefile-bottom.mk
include Makefile.pdlibbuilder
