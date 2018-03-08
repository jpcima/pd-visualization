#!/bin/bash -e
pkgname=jpcvisu
dekformat=1 # 0=old (.zip/.tar.gz) 1=new (.dek)

case "$#" in
    1) pkgos=$1; pkgver=$(git describe) ;;
    2) pkgos=$1; pkgver=$2 ;;
    *) exit 1 ;;
esac

scriptdir=`dirname "$0"`
cd "$scriptdir/.."

if test "${pkgver:0:1}" = v; then
    pkgver="${pkgver:1}"
fi

mkdir -p deken-pkg

rm -rf deken-tmp
mkdir -p "deken-tmp/$pkgname"
cp -va *.md src cmake CMakeLists.txt "deken-tmp/$pkgname/"
cp -va *.pd "deken-tmp/$pkgname/"

case "$pkgos" in
    linux) cp -va *.pd_linux visu~-gui "deken-tmp/$pkgname/" ;;
    windows) cp -va *.dll visu~-gui.exe "deken-tmp/$pkgname/" ;;
    mac) cp -va *.pd_darwin visu~-gui "deken-tmp/$pkgname/" ;;
esac

mkdir -p "deken-tmp/$pkgname/thirdparty"
find thirdparty -type f | while read f; do
    case "$f" in
        thirdparty/color/.git*) ;;
        thirdparty/color/doc/*) ;;
        thirdparty/color/tmp/*) ;;
        thirdparty/color/example/*) ;;
        thirdparty/color/favicon.*) ;;
        thirdparty/color/src/color/*/*) ;;
        thirdparty/color/src/color/color.body.hpp) ;;
        thirdparty/Fl_Knob/Makefile) ;;
        thirdparty/Fl_Knob/Test.*) ;;
        *) install -v -D -m 644 "$f" "deken-tmp/$pkgname/$f" ;;
    esac
done

cd deken-tmp
deken package --dekformat "$dekformat" --version "$pkgver" "$pkgname"
if test "$dekformat" -lt 1; then
    case "$pkgos" in
        windows) mv -f *.zip *.zip.* ../deken-pkg/ ;;
        *) mv -f *.tar.gz *.tar.gz.* ../deken-pkg/ ;;
    esac
else
    mv -f *.dek *.dek.* ../deken-pkg/
fi
cp -f ../objects.txt "../deken-pkg/$pkgname-v$pkgver-objects.txt"
