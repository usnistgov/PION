#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..
PION_BUILDDIR=build.programs

OSREL=$(lsb_release -sc)
if [[ $OSREL != focal ]]; then
  echo 'This script only works on Ubuntu 20.04' >/dev/stderr
  exit 1
fi

sudo apt-get update
sudo apt-get -y -qq install --no-install-recommends build-essential ca-certificates curl libboost-dev libmbedtls-dev ninja-build python3-distutils
curl -fsLS https://bootstrap.pypa.io/get-pip.py | sudo python3
sudo pip install -U meson

if ! [[ -f ${PION_BUILDDIR}/build.ninja ]]; then
  meson ${PION_BUILDDIR}
fi
ninja -C ${PION_BUILDDIR} -j1
find ./${PION_BUILDDIR}/programs ./${PION_BUILDDIR}/subprojects/NDNph/programs -maxdepth 1 -type f | sudo xargs install -t /usr/local/bin
