mkdir -p /root/src/tflite
cd /root/src/tflite

git clone https://github.com/tensorflow/tensorflow.git tensorflow_src \
    --branch v2.8.2 

mkdir -p build
cd build

cmake ../tensorflow_src/tensorflow/lite 

cmake --build . -j $(grep -c ^processor /proc/cpuinfo)

mkdir /usr/local/lib/tflite; find . | grep -E "\.a$" | xargs cp -t /usr/local/lib/tflite

cp -r flatbuffers/include/flatbuffers /usr/local/include

cp -r ../tensorflow_src/tensorflow /usr/local/include
