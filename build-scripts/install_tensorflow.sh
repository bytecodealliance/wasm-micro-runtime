CURR_PATH=$(cd $(dirname $0) && pwd -P)
WAMR_ROOT_DIR=${CURR_PATH}/..

cd ${WAMR_ROOT_DIR}/core/deps

if [ ! -d "tensorflow-src" ] 
then
    git clone https://github.com/tensorflow/tensorflow.git tensorflow-src \
        --branch v2.10.0
fi
