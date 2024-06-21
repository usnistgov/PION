// Command pion-pcapparse extracts information from PION packet trace.
package main

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"time"

	"github.com/gopacket/gopacket/pcapgo"
	"github.com/urfave/cli/v2"
	"github.com/usnistgov/PION/extras/pcapparse/parser"
)

func main() {
	var ble, ap bool
	var sta string
	var matcher parser.Matcher
	app := &cli.App{
		Flags: []cli.Flag{
			&cli.BoolFlag{
				Name:        "ble",
				Destination: &ble,
			},
			&cli.BoolFlag{
				Name:        "ap",
				Destination: &ap,
			},
			&cli.StringFlag{
				Name:        "sta",
				Destination: &sta,
			},
		},
		Before: func(c *cli.Context) (e error) {
			count := 0
			if ble {
				matcher = parser.BLEBridgeMatcher{}
				count++
			}
			if ap {
				matcher = parser.APMatcher{}
				count++
			}
			if sta != "" {
				m := parser.STAMatcher{}
				if m.AP, e = net.ParseMAC(sta); e != nil {
					return cli.Exit(e, 1)
				}
				matcher = m
				count++
			}
			if count != 1 {
				return cli.Exit("exactly one of --ble --ap --sta is required", 1)
			}
			return nil
		},
		Action: func(c *cli.Context) error {
			reader, e := pcapgo.NewReader(os.Stdin)
			if e != nil {
				return cli.Exit(e, 1)
			}

			p := parser.New(matcher)
			for {
				packetData, ci, e := reader.ReadPacketData()
				if errors.Is(e, io.EOF) {
					return nil
				} else if e != nil {
					return cli.Exit(e, 1)
				}

				dir, length := p.Parse(packetData)
				if dir != parser.DirNone {
					fmt.Println(ci.Timestamp.UnixNano()/int64(time.Millisecond), dir, length)
				}
			}
		},
	}
	app.Run(os.Args)
}
