#!/bin/sh

DEPS_ROOT=$(cd "$(dirname "$0")/" && pwd)
cd ${DEPS_ROOT}

echo "Downloading tensorflow in ${PWD}..."

git clone https://github.com/tensorflow/tensorflow.git tensorflow-src \
    --branch v2.9.2

exit 0
