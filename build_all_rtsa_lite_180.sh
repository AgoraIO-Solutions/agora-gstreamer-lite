#! /bin/sh

BUILD_DIR=$PWD

#build libagora_rtsac
echo "building and installing libagorac ..."
cd agora/libagora_rtsac/ && ./install_lite.sh $BUILD_DIR/agora/sdk/agora_rtsa_sdk

echo "building and installing agora plugin ..."
sudo rm -rf $BUILD_DIR/gst-agora/build || true
cd $BUILD_DIR/gst-agora && meson build && ./install

