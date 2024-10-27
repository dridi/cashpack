// Copyright (c) 2016-2019 Dridi Boukelmoune
// All rights reserved.
//
// Author: Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

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
