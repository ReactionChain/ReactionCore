#!/usr/bin/env bash

export LC_ALL=C

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/.. || exit

DOCKER_IMAGE=${DOCKER_IMAGE:-reaction/reactiond-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/reactiond docker/bin/
cp $BUILD_DIR/src/reaction-cli docker/bin/
cp $BUILD_DIR/src/reaction-tx docker/bin/
strip docker/bin/reactiond
strip docker/bin/reaction-cli
strip docker/bin/reaction-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
