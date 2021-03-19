#!/bin/bash
set -eo pipefail

if ! ndnsec get-default &>/dev/null; then
  ndnsec key-gen /localhost/operator >/dev/null
fi

mkdir -p /etc/ndn/certs
ndnsec cert-dump -i $(ndnsec get-default) > /etc/ndn/certs/localhost.ndncert

cp /nfd.conf.sample /etc/ndn/nfd.conf
infoedit -f /etc/ndn/nfd.conf -s general.user -v ndn
infoedit -f /etc/ndn/nfd.conf -s general.group -v ndn
infoedit -f /etc/ndn/nfd.conf -s tables.cs_max_packets -v 4096
infoedit -f /etc/ndn/nfd.conf -s face_system.unix.path -v /run/ndn/nfd.sock
infoedit -f /etc/ndn/nfd.conf -d face_system.udp.whitelist
infoedit -f /etc/ndn/nfd.conf -p face_system.udp.whitelist.ifname -v ap
infoedit -f /etc/ndn/nfd.conf -d face_system.ether.whitelist
infoedit -f /etc/ndn/nfd.conf -p face_system.ether.whitelist.ifname -v ap

echo 'transport=unix:///run/ndn/nfd.sock' > /etc/ndn/client.conf

chown -R ndn:ndn /var/lib/ndn/nfd
/connect.sh &
exec /usr/bin/nfd
