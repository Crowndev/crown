#!/usr/bin/env bash

# This script is executed inside the builder image / or local compile

set -e

source ./ci/matrix.sh

mkdir -p $PWD/../binary
mkdir -p $PWD/../binary/win64
mkdir -p $PWD/../binary/linux64
mkdir -p $PWD/../binary/mac
mkdir -p $PWD/../binary/android

unset CC; unset CXX
unset DISPLAY

export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
export CCACHE_SIZE=${CCACHE_SIZE:-400M}

ccache --max-size=$CCACHE_SIZE

if [ -n "$USE_SHELL" ]; then
  export CONFIG_SHELL="$USE_SHELL"
fi

CROWN_CONFIG_ALL="--disable-tests --disable-bench --disable-man --disable-dependency-tracking --prefix=$BUILD_DIR/depends/$HOST --bindir=$OUT_DIR/bin --libdir=$OUT_DIR/lib"

test -n "$USE_SHELL" && eval '"$USE_SHELL" -c "./autogen.sh"' || ./autogen.sh

rm -rf build-ci
mkdir build-ci
cd build-ci

../configure --cache-file=config.cache $CROWN_CONFIG_ALL $CROWN_CONFIG || ( cat config.log && false)
# make distdir VERSION=$BUILD_TARGET
make VERSION=$BUILD_TARGET
if [ "$BUILD_TARGET" = "android" ]; then
cp -R ../src/qt/android src/qt/
cp -R ../src/qt/Makefile src/qt/
cd src/qt
make apk
cp android/build/outputs/apk/release/android-release.apk ../../../../binary/android
tar -czvf ../../../../binary/Crown-android.tar.gz ../../../../binary/android/*
rm -rf ../../../../binary/android
cd ../..
fi

if [ "$BUILD_TARGET" = "win64" ]; then
cd src
strip qt/crown-qt.exe
strip crownd.exe
strip crown-cli.exe
cp crown-cli.exe crownd.exe crown-tx.exe crown-util.exe crown-wallet.exe qt/crown-qt.exe ../../../binary/win64
tar -czvf ../../../binary/Crown-Win64.tar.gz ../../../binary/win64/*
rm -rf ../../../binary/win64
cd ..
fi

if [ "$BUILD_TARGET" = "linux64" ]; then
cd src
strip qt/crown-qt
strip crownd
strip crown-cli
cp crown-cli crownd crown-tx crown-util crown-wallet qt/crown-qt ../../../binary/linux64
tar -czvf ../../../binary/Crown-linux64.tar.gz ../../../binary/linux64/*
rm -rf ../../../binary/linux64
cd ..
fi

#cd crown-$BUILD_TARGET
#./configure --cache-file=../config.cache $CROWN_CONFIG_ALL $CROWN_CONFIG || ( cat config.log && false)

make -s $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && make $GOAL V=1 ; false )
