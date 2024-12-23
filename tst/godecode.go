// License: BSD-2-Clause
// (c) 2016-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"syscall"
	"golang.org/x/net/http2/hpack"
)

func DecodeBlocks(dec *hpack.Decoder, spec string, blk []byte) error {
	var err error
	var len int

	for _, stp := range strings.Split(spec, ",") {
		if stp != "" {
			stp = strings.Replace(stp, ",", "", 1)
			if len, err = strconv.Atoi(stp[1:]); err != nil {
				log.Fatal(err)
			}
		}
		if stp == "" {
			break
		} else if stp[0] == 'a' {
			panic("abort")
		} else if stp[0] == 'd' {
			if _, err = dec.Write(blk[0:len]); err == nil {
				blk = blk[len:]
				err = dec.Close()
			}
		} else if stp[0] == 'p' {
			if _, err = dec.Write(blk[0:len]); err == nil {
				blk = blk[len:]
			}
		} else if stp[0] == 'r' {
			dec.SetAllowedMaxDynamicTableSize(uint32(len))
		} else {
			log.Fatal("unknown command: ", stp)
		}
		if err != nil {
			break
		}
	}
	if _, err = dec.Write(blk); err == nil {
		err = dec.Close()
	}
	return err
}

func main() {
	var spec string
	var exp string
	var tbl_sz int

	flag.StringVar(&spec, "decoding-spec", "", "An hdecode spec.")
	flag.StringVar(&exp, "expect-error", "OK", "An error (except BSY).")
	flag.IntVar(&tbl_sz, "table-size", 4096, "The dynamic table size.")
	flag.Parse()

	if exp == "BSY" || flag.NArg() != 1 {
		flag.Usage()
		os.Exit(1)
	}

	file, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Fatal("open: ", err)
	}

	st, err := file.Stat()
	if err != nil {
		log.Fatal("stat: ", err)
	}

	blk, err := syscall.Mmap(int(file.Fd()), 0, int(st.Size()),
		syscall.PROT_READ, syscall.MAP_PRIVATE)
	if err != nil {
		log.Fatal("mmap: ", err)
	}

	lst := make([]hpack.HeaderField, 0)
	dec := hpack.NewDecoder(uint32(tbl_sz), func(fld hpack.HeaderField) {
		lst = append(lst, fld)
	})

	err = DecodeBlocks(dec, spec, blk)
	if err != nil && exp == "OK" {
		log.Fatal("decode: ", err)
	}

	if err == nil {
		fmt.Print("Decoded header list:\n\n")
		for _, fld := range lst {
			fmt.Printf("%s: %s\n", fld.Name, fld.Value)
		}
		fmt.Println()
	}
}
