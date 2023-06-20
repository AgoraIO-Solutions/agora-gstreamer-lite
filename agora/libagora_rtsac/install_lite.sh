#! /bin/sh

INPUT_DIR=$1;
export AGORA_SDK_DIR=$INPUT_DIR
echo "agora sdk dir"
echo $AGORA_SDK_DIR
echo "MAKE"
make
echo "MAKE INSTALL"
sudo make install
sudo cp  $INPUT_DIR/agora_sdk/lib/x86_64/libagora-rtc-sdk.so  /usr/local/lib/lite/
#sudo cp agorac.h /usr/local/include
#sudo cp agoraconfig.h /usr/local/include
sudo ldconfig
