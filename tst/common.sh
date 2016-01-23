# Copyright (c) 2016 Dridi Boukelmoune
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
set -o pipefail

TEST_NAM="$(basename "$0")"
TEST_DIR="$(dirname "$0")"
TEST_TMP="$(mktemp -d cashpack.XXXXXXXX)"

trap "rm -fr $TEST_TMP" EXIT

MEMCHECK_CMD="valgrind		\
	--tool=memcheck		\
	--leak-check=yes	\
	--show-reachable=yes	\
	--track-fds=yes		\
	--error-exitcode=99	\
	--log-file=memcheck-${TEST_NAM}-%p.log"

[ "$MEMCHECK" = ON ] && rm -f memcheck-${TEST_NAM}-*.log

memcheck() {
	if [ "$MEMCHECK" = ON ]
	then
		local rc=0
		$MEMCHECK_CMD "$@" || rc=$?
		[ $rc -eq 99 ] && echo >&2 memcheck: "$@"
		return $rc
	else
		"$@"
	fi
}

cmd_check() {
	for cmd
	do
		type "$cmd" >/dev/null
	done
}

rm_comments() {
	sed -e '/^#/d'
}

rm_blanks() {
	sed -e '/^$/d'
}

mk_hex() {
	rm_comments >"$TEST_TMP/hex"
}

mk_bin() {
	cut -d '|' -f 1 |
	while read bin
	do
		# XXX: is this portable?
		printf "%02x" $((2#$bin))
	done |
	mk_hex
}

mk_msg() {
	rm_comments | rm_blanks >"$TEST_TMP/msg"
	echo >> "$TEST_TMP/msg"
}

mk_tbl() {
	msg="Dynamic Table (after decoding):"
	cat >"$TEST_TMP/tbl_tmp"

	if [ -s "$TEST_TMP/tbl_tmp" ]
	then
		printf "%s\n\n" "$msg" |
		cat - "$TEST_TMP/tbl_tmp" >"$TEST_TMP/tbl"
	else
		printf >"$TEST_TMP/tbl" "%s empty.\n" "$msg"
	fi

	rm  "$TEST_TMP/tbl_tmp"
}

mk_enc() {
	false # XXX: create the encoding sequence
}

tst_decode() {
	"$TEST_DIR/hex_decode"  <"$TEST_TMP/hex" >"$TEST_TMP/bin"
	memcheck ./hdecode $@ "$TEST_TMP/bin" >"$TEST_TMP/dec"

	printf "Decoded header list:\n\n"    >"$TEST_TMP/tst"
	cat "$TEST_TMP/msg" "$TEST_TMP/tbl" >>"$TEST_TMP/tst"

	diff -u "$TEST_TMP/tst" "$TEST_TMP/dec"
}

tst_encode() {
	false # XXX: check the encoding process
}
