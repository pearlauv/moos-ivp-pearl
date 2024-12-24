# Author: Charlie Benjamin
# Note: If you have access to the 2680 student tree, you can copy the whole file over
# Note: If you are copying the files over from this existing folder which has been slightly modified for different unit measurements then you may want to start after the indicated line below
# Final Note: This setup is for a linux computer. For Mac, you will have to change "/usr/local/include"

cd /home/student2680
git clone https://github.com/bluerobotics/ping-cpp
cd ping-cpp

#setup from here if not using 2680

git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --parallel --config Debug

cp src/device/ping-device-ping1d.h /usr/local/include
cp src/device/ping-device.h /usr/local/include
cp src/message/ping-message-common.h /usr/local/include
cp src/message/ping-message.h /usr/local/include
cp src/message/ping-parser.h /usr/local/include
cp src/hal/link/ping-port.h /usr/local/include

mkdir /usr/local/include/abstract-link
cp src/hal/link/desktop/abstract-link.h /usr/local/include/abstract-link

cp build/src/device/libDEVICE.a /usr/local/lib
cp build/src/hal/libHAL.a /usr/local/lib
