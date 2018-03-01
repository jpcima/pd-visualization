#!/bin/bash -e
pkgname=jpcvisu
pkgver=$(git describe)

scriptdir=`dirname "$0"`
cd "$scriptdir/.."

if test "${pkgver:0:1}" = v; then
    pkgver="${pkgver:1}"
fi

rm -rf deken-tmp
mkdir -p "deken-tmp/$pkgname"
cp -va *.pd *.pd_linux visu~-gui "deken-tmp/$pkgname/"
cp -va *.md src cmake CMakeLists.txt "deken-tmp/$pkgname/"

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
deken package -v "$pkgver" "$pkgname"
