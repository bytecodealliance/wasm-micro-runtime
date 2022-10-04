if [ ! -d "tensorflow-src" ] 
then
    git clone https://github.com/tensorflow/tensorflow.git tensorflow-src \
        --branch v2.10.0
fi
