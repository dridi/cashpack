#!/bin/sh
#
# Copyright (c) 2017 Dridi Boukelmoune
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

set -e

TMP_C_FILE=$(mktemp tmp.XXXXXXXXXX.c)
TMP_H_FILE=$(mktemp tmp.XXXXXXXXXX.h)
SRC_C_FILE=$(mktemp src.XXXXXXXXXX.c)
SRC_H_FILE=$(mktemp src.XXXXXXXXXX.h)

SRC_DIR=$(dirname "$1")

trap 'rm -f "$TMP_C_FILE" "$TMP_H_FILE" "$SRC_C_FILE" "$SRC_H_FILE"' EXIT

# Usage: ./$0 source.h.in target.h
test $# = 2
test -f "$1"

# Create a dummy C file to detect potential junk
cat >"$TMP_C_FILE" <<EOF
const int END_OF_HEADER = 0;
EOF

# Append the dummy C code to the source header
cat "$1" "$TMP_C_FILE" >"$SRC_C_FILE"

# Process the headers as C files
cpp -C -P "$TMP_C_FILE" "$TMP_H_FILE"
cpp -C -P -I"$SRC_DIR" "$SRC_C_FILE" "$SRC_H_FILE"

# Remove protections
sed -i -e /REMOVE_ME/d "$SRC_H_FILE"

# Measure prepended junkness
JUNK=$(sed -e /END_OF_HEADER/q <"$TMP_H_FILE" | wc -l | awk '{print $1}')

# Remove potential junk prepended by the preprocessor.
# The temp header is no longer needed as is, reuse it.
# The temp C source contains a single line of code to
# help the `-n` option cope with a lack of junk.
tail -n "+$JUNK" <"$SRC_H_FILE" >"$TMP_H_FILE"

# Remove the dummy C code
sed -i -e /END_OF_HEADER/d "$TMP_H_FILE"

# Work around weird macro expansion on OSX
sed -i -e s/##// "$TMP_H_FILE"

# Make it look less bad
if command -v uncrustify >/dev/null
then
	CFG=$SRC_DIR/uncrustify.cfg
	uncrustify -c "$CFG" -q -l C --no-backup "$TMP_H_FILE"
fi

# And voilà
cp -f "$TMP_H_FILE" "$2"
