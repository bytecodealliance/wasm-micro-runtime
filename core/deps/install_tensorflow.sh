#!/bin/sh

DEPS_ROOT=$(cd "$(dirname "$0")/" && pwd)
cd ${DEPS_ROOT}

echo "Downloading tensorflow in ${PWD}..."

git clone https://github.com/tensorflow/tensorflow.git tensorflow-src \
    --branch v2.12.0

# NOTE: fixes this https://github.com/tensorflow/tensorflow/issues/59631
cd tensorflow-src
git cherry-pick 5115fa96d7c5b41451674892317be43e30b7c389

exit 0
