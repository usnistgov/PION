package parser

import (
	"bytes"
	"net"
)

var (
	ipDeviceAP = net.ParseIP("192.168.4.1")
)

const (
	etherTypeNDN = 0x8624
	udpPortNDN   = 6363
	udpPortBLE   = 6362
)

// Matcher determines whether an address belongs to the device.
type Matcher interface {
	isDevice(mac net.HardwareAddr, ip net.IP, port uint16) bool
}

// BLEBridgeMatcher is a Matcher for device connected via BleBridge.
type BLEBridgeMatcher struct{}

func (BLEBridgeMatcher) isDevice(mac net.HardwareAddr, ip net.IP, port uint16) bool {
	return ip.IsLoopback() && port == udpPortBLE
}

// APMatcher is a Matcher for device in WiFi AP mode.
type APMatcher struct{}

func (APMatcher) isDevice(mac net.HardwareAddr, ip net.IP, port uint16) bool {
	return ipDeviceAP.Equal(ip) && port == udpPortNDN
}

// STAMatcher is a Matcher for device in WiFi STA mode.
type STAMatcher struct {
	AP net.HardwareAddr
}

func (m STAMatcher) isDevice(mac net.HardwareAddr, ip net.IP, port uint16) bool {
	return (ip == nil || port == udpPortNDN) && !bytes.Equal([]byte(m.AP), []byte(mac))
}
