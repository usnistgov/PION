#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..
PION_BUILDDIR=build.programs

OSREL=$(lsb_release -sc)
if [[ $OSREL != jammy ]]; then
  echo 'This script only works on Ubuntu 22.04' >/dev/stderr
  exit 1
fi

sudo apt-get update
sudo apt-get -y -qq install --no-install-recommends build-essential libboost-dev libmbedtls-dev ninja-build python3-pip
sudo pip3 install meson

if ! [[ -f ${PION_BUILDDIR}/compile_commands.json ]]; then
  meson setup ${PION_BUILDDIR}
fi
meson compile -C ${PION_BUILDDIR} -j1
find ./${PION_BUILDDIR}/programs ./${PION_BUILDDIR}/subprojects/NDNph/programs -maxdepth 1 -type f |
  sudo xargs install -t /usr/local/bin
