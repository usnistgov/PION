#!/bin/bash
set -euo pipefail
export HOME=/var/lib/ndn/nfd

if ! ndnsec get-default &>/dev/null; then
  ndnsec key-gen /localhost/operator >/dev/null
fi

mkdir -p /etc/ndn/certs
ndnsec cert-dump -i $(ndnsec get-default) >/etc/ndn/certs/localhost.ndncert

cp /etc/ndn/nfd.conf.sample /etc/ndn/nfd.conf
nfdconfedit() {
  infoedit -f /etc/ndn/nfd.conf "$@"
}

nfdconfedit -s general.user -v ndn
nfdconfedit -s general.group -v ndn
nfdconfedit -s tables.cs_max_packets -v 4096
nfdconfedit -s face_system.unix.path -v /run/ndn/nfd.sock
nfdconfedit -s face_system.udp.unicast_mtu -v 1452
nfdconfedit -d face_system.udp.whitelist
nfdconfedit -p face_system.udp.whitelist.ifname -v ap
nfdconfedit -d face_system.ether.whitelist
nfdconfedit -p face_system.ether.whitelist.ifname -v ap
nfdconfedit -d rib.auto_prefix_propagate

chown -R ndn:ndn /var/lib/ndn/nfd
exec /usr/bin/nfd
