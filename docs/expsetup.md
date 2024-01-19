# Experiment System Setup

PION onboarding experiments are conducted on a Raspberry Pi 3B.
This page explains how the system was setup.

```text
  serial |--------------------|
   /-----|    Raspberry Pi    |
   |     |--------------------|
   |       \auth/ \ ap /  |
   |        \--/   \--/   |BLE
|--+--|      |      |     |
|ESP32|------+------+-----+
|-----| WiFi / BLE
```

> Certain commercial entities, equipment, or materials may be identified in this document in order to describe an experimental procedure or concept adequately.
> Such identification is not intended to imply recommendation or endorsement by the National Institute of Standards and Technology, nor is it intended to imply that the entities, materials, or equipment are necessarily the best available for the purpose.

## Operating System Setup

The RPi is running Ubuntu Server 22.04 (64-bit).
Installed software include:

* APT packages: `dnsmasq hostapd iw jq pi-bluetooth wireshark-common`
* Docker

Changed configuration includes:

```bash
# enable Bluetooth
echo 'include btcfg.txt' | sudo tee -a /boot/firmware/usrcfg.txt

# allow non-root to capture packets with 'dumpcap'
echo 'wireshark-common wireshark-common/install-setuid boolean true' | sudo debconf-set-selections
sudo dpkg-reconfigure --frontend=noninteractive wireshark-common
sudo adduser $(id -un) wireshark
```

## Network Interfaces

The built-in WiFi interface and a USB WiFi adapter are attached for experiment use.
They are renamed to `auth` and `ap` via [systemd.link](https://www.freedesktop.org/software/systemd/man/systemd.link.html) files.

### "auth" interface

The `auth` interface runs in WiFi station mode.
It is used by the authenticator, when the device uses WiFi as direct connection transport.
It is controlled by the experiment script, connecting on-demand.

Upload this config file:

* [`/etc/wpa_supplicant/wpa_supplicant-auth.conf`](wpa_supplicant-auth.conf)

Enable and start the services:

```bash
sudo systemctl enable wpa_supplicant@auth
sudo systemctl start wpa_supplicant@auth
```

### "ap" interface

The `ap` interface runs in WiFi AP mode.
It is attached to NFD, used by the certificate authority.
It is always powered on.

Upload these config files:

* [`/etc/netplan/10-ap.yaml`](netplan-ap.yaml)
* [`/etc/hostapd/hostapd.conf`](hostapd.conf)
* [`/etc/dnsmasq.conf`](dnsmasq.conf)

Enable and start the services:

```bash
sudo systemctl unmask hostapd
sudo systemctl enable --now hostapd

sudo SYSTEMD_EDITOR=tee systemctl edit dnsmasq <<EOT
[Unit]
Requires=network-online.target
After=network-online.target
EOT
sudo systemctl restart dnsmasq
```

### BLE Bridge

The built-in Bluetooth hardware is used by the authenticator, when the device uses Bluetooth Low Energy as direct connection transport.
It is controlled by the experiment script, connecting on-demand.

A [BLE-to-UDP bridge](https://github.com/yoursunny/esp8266ndn/tree/main/extras/BLE) script is required.
It can be installed with the following commands:

```bash
sudo apt install python3-dev python3-venv libglib2.0-dev
sudo pip3 install -U pipenv

mkdir -p ~/BLE
curl -fsLS https://github.com/yoursunny/esp8266ndn/archive/main.tar.gz |\
  tar -C ~/BLE -xz --strip-components=3 --wildcards 'esp8266ndn-*/extras/BLE'
cd ~/BLE
pipenv install
```

## NFD Docker

NFD is required to accept packets on the `ap` interface and forward them between the device and the certificate authority.
It's recommended to install NFD as a Docker container.

With this repository cloned at `~/PION`, build and start NFD Docker container:

```bash
cd ~/PION/extras/nfd
docker build --pull -t localhost/nfd .

docker rm -f nfd
docker run -d --name nfd --init \
  --network host --cap-add=NET_ADMIN \
  -v /run/ndn:/run/ndn \
  --restart unless-stopped \
  localhost/nfd
```
