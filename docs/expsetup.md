# Experiment System Setup

NDN onboarding experiments are conducted on a Raspberry Pi 3B.
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

## Operating System Setup

The RPi is running Ubuntu 20.04 Server, 32-bit ARMv7.
Installed software include:

* APT packages: `dnsmasq hostapd jq pi-bluetooth socat tshark`
* Docker

Changed configuration includes:

```bash
# enable Bluetooth
echo 'include btcfg.txt' | sudo tee -a /boot/firmware/usrcfg.txt

# allow non-root to capture packets with 'dumpcap'
sudo dpkg-configure wireshark-common
#  "Should non-superusers be able to capture packets?" <Yes>
sudo usermod -aG wireshark $(whoami)
```

## Network Interfaces

Two USB WiFi adapters are attached for experiment use.
The experiment network interfaces are renamed to `auth` and `ap` via [systemd.link](https://www.freedesktop.org/software/systemd/man/systemd.link.html) files.

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

* [`/etc/netplan/01-netcfg.yml`](netplan.yml)
* [`/etc/hostapd/hostapd.conf`](hostapd.conf)
* [`/etc/dnsmasq.conf`](dnsmasq.conf)

Enable and start the services:

```bash
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd

sudo systemctl edit dnsmasq
#  [Unit]
#  Requires=network-online.target
#  After=network-online.target
sudo systemctl restart dnsmasq
```

### BLE Bridge

The built-in Bluetooth hardware is used by the authenticator, when the device uses Bluetooth Low Energy as direct connection transport.
It is controlled by the experiment script, connecting on-demand.

A [BLE-to-UDP bridge](https://github.com/yoursunny/esp8266ndn/tree/main/extras/BLE) script is required.
It can be installed with the following commands:

```bash
sudo apt install python3-dev python3-venv libglib2.0-dev
sudo pip install -U pipenv

mkdir -p ~/code/BLE
curl -fsLS https://github.com/yoursunny/esp8266ndn/archive/main.tar.gz |\
  tar -C ~/code/BLE -xz --strip-components=3 --wildcards 'esp8266ndn-*/extras/BLE'
cd ~/code/BLE
pipenv install
```

## NFD Docker

NFD is required to accept packets on the `ap` interface and forward them between the device and the certificate authority.
It's recommended to install NFD as a Docker container.

With this repository cloned at `~/code/PION`, build and start NFD Docker container:

```bash
cd ~/code/PION/extras/nfd
docker build -t nfd .

docker rm -f nfd
docker run -d --name nfd --init \
  --network host --cap-add=NET_ADMIN \
  -v /run/ndn:/run/ndn \
  --restart unless-stopped \
  nfd
```
