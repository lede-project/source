#!/bin/sh

### gather all build system output for publishing

IMAGE_DIR="bin/targets/ramips/mt76x8"
PKG_DIR="bin/packages/mipsel_24kc"
BASE="bin/output"

# create directory structure
[ -d $BASE ] && rm -rf $BASE
mkdir -p $BASE/images/
mkdir -p $BASE/packages/

# copy images
cp $IMAGE_DIR/omega2*.bin $BASE/images/

# copy packages
cp -r $IMAGE_DIR/packages/ $BASE/packages/core/
cp -r $PKG_DIR/* $BASE/packages/

