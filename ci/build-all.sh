#!/bin/bash
export BUILD_TARGET="win64"
ci/build_src.sh
export BUILD_TARGET="linux64"
ci/build_src.sh
export BUILD_TARGET="android"
ci/build_src.sh
