// Package parser provides a parser for PION packet trace.
package parser

import (
	"github.com/gopacket/gopacket"
	"github.com/gopacket/gopacket/layers"
)

// Direction represents a packet direction.
type Direction uint8

const (
	DirNone Direction = iota
	DirFromDevice
	DirToDevice
)

func (dir Direction) String() string {
	switch dir {
	case DirFromDevice:
		return "<"
	case DirToDevice:
		return ">"
	}
	return "?"
}

// Parser parses a captured packet in PION experiment.
type Parser struct {
	matcher Matcher
	dlp     *gopacket.DecodingLayerParser

	decoded []gopacket.LayerType
	eth     layers.Ethernet
	ip      layers.IPv4
	udp     layers.UDP
}

// Parse parses a packet.
func (p *Parser) Parse(input []byte) (dir Direction, length int) {
	p.dlp.DecodeLayers(input, &p.decoded)
	for _, typ := range p.decoded {
		switch typ {
		case layers.LayerTypeEthernet:
			if p.eth.EthernetType == etherTypeNDN {
				switch {
				case p.matcher.isDevice(p.eth.SrcMAC, nil, 0):
					return DirFromDevice, len(p.eth.Payload)
				case p.matcher.isDevice(p.eth.DstMAC, nil, 0):
					return DirToDevice, len(p.eth.Payload)
				}
			}
		case layers.LayerTypeUDP:
			switch {
			case p.matcher.isDevice(p.eth.SrcMAC, p.ip.SrcIP, uint16(p.udp.SrcPort)):
				return DirFromDevice, len(p.udp.Payload)
			case p.matcher.isDevice(p.eth.DstMAC, p.ip.DstIP, uint16(p.udp.DstPort)):
				return DirToDevice, len(p.udp.Payload)
			}
		}
	}
	return DirNone, 0
}

// New creates a Parser.
func New(matcher Matcher) (p *Parser) {
	p = &Parser{
		matcher: matcher,
	}
	p.dlp = gopacket.NewDecodingLayerParser(layers.LayerTypeEthernet, &p.eth, &p.ip, &p.udp)
	return p
}
