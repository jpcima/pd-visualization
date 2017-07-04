
ifeq ($(shell uname -s),Linux)
include mk/host-linux.mk
else ifeq ($(shell uname -s),Darwin)
include mk/host-darwin.mk
else ifneq ($(findstring MINGW,$(shell uname -s)),)
include mk/host-windows.mk
endif
