#!/usr/bin/env bash

# exit on first error
set -o errexit
# exit when undefined var is used
set -o nounset
# exit with error code of right-most command in a pipe
set -o pipefail

# This script opens a bash session in docker for the Dockerfile in this folder.

readonly image_name=rust_qt_binding_generator_dev
readonly docker_file_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
readonly src_dir=$(dirname "$docker_file_dir")
readonly docker_home_dir="$src_dir/docker_home"

# create the docker image
docker build -t "$image_name" "$docker_file_dir"

# create a working directory
if [ ! -a "$docker_home_dir" ]; then
    mkdir -p "$docker_home_dir"/rust_qt_binding_generator
fi

# give the docker application access to X
xhost +si:localuser:"$USER"

# start a docker session with the source code and a dedicated home directory
docker run --rm -v "$docker_home_dir":/home/neon \
  -v "$src_dir":/home/neon/rust_qt_binding_generator \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -e DISPLAY="$DISPLAY" \
  -e XDG_CURRENT_DESKTOP="$XDG_CURRENT_DESKTOP" \
  -ti "$image_name" bash

#  --network none \
