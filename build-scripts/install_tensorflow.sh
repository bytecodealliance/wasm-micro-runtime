mkdir -p /root/src/tflite
cd /root/src/tflite

if [ ! -d "tensorflow_src" ] 
then
    git clone https://github.com/tensorflow/tensorflow.git tensorflow_src \
        --branch v2.8.2 
fi
