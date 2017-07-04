#!/bin/sh
set -e

t=i686-w64-mingw32
pd=thirdparty/pd-msw

exec make \
  PKG_CONFIG="$t-pkg-config" CC="$t-gcc" CXX="$t-g++" \
  system=Windows pdbinpath="$pd"/bin pdincludepath="$pd"/src \
  -f mk/host-windows.mk "$@"
