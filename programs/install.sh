#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..
NDNOB_BUILDDIR=build.programs

OSREL=$(lsb_release -sc)
if [[ $OSREL != 'focal' ]]; then
  echo 'This script only works on Ubuntu 20.04' >/dev/stderr
  exit 1
fi

sudo apt-get update
sudo apt-get -y -qq install --no-install-recommends build-essential ca-certificates curl libboost-dev libmbedtls-dev ninja-build python3-distutils
if ! which meson >/dev/null; then
  curl -fsLS https://bootstrap.pypa.io/get-pip.py | sudo python3
  sudo pip install -U meson
fi

if ! [[ -f ${NDNOB_BUILDDIR}/build.ninja ]]; then
  meson ${NDNOB_BUILDDIR}
fi
ninja -C ${NDNOB_BUILDDIR} -j1
find ./${NDNOB_BUILDDIR}/programs -maxdepth 1 -type f | sudo xargs install -t /usr/local/bin
