#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..
NDNPH_VERSION=${NDNPH_VERSION:-develop}
NDNPH_BUILDDIR=build.ndnob
NDNOB_BUILDDIR=build.programs

OSREL=$(lsb_release -sc)
if [[ $OSREL != 'focal' ]]; then
  echo 'This script only works on Ubuntu 20.04' >/dev/stderr
  exit 1
fi

if ! [[ -f spake2-mbed/spake2.hpp ]]; then
  echo 'Pesa/spake2-mbed checkout is missing' >/dev/stderr
  exit 1
fi

sudo apt-get update
sudo apt-get -y -qq install --no-install-recommends build-essential ca-certificates curl libboost-dev libmbedtls-dev ninja-build python3-distutils
if ! which meson >/dev/null; then
  curl -sfL https://bootstrap.pypa.io/get-pip.py | sudo python3
  sudo pip install -U meson
fi

if ! [[ -f /usr/local/include/NDNph.h ]]; then
  if ! [[ -f ../NDNph/meson.build ]]; then
    mkdir ../NDNph
    curl -sfL https://github.com/yoursunny/NDNph/archive/${NDNPH_VERSION}.tar.gz | tar -C ../NDNph -xz --strip-components=1
  fi
  (
    cd ../NDNph
    meson ${NDNPH_BUILDDIR} -Dunittest=disabled -Dprograms=enabled
    ninja -C ${NDNPH_BUILDDIR} -j1
    sudo ninja -C ${NDNPH_BUILDDIR} install
    find ./${NDNPH_BUILDDIR}/programs -maxdepth 1 -type f | sudo xargs install -t /usr/local/bin
  )
fi

mk/update-vendor.sh
if ! [[ -f ${NDNOB_BUILDDIR}/build.ninja ]]; then
  meson ${NDNOB_BUILDDIR}
fi
ninja -C ${NDNOB_BUILDDIR} -j1
find ./${NDNOB_BUILDDIR}/programs -maxdepth 1 -type f | sudo xargs install -t /usr/local/bin
