# Copyright (c) 2016-2017 Dridi Boukelmoune
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
set -u

# Test environment

readonly TEST_NAM=$(basename "$0")
readonly TEST_DIR=$(dirname "$0")
readonly TEST_TMP=$(mktemp -d "cashpack.${TEST_NAM}.XXXXXXXX")

keep_tmp() {
	[ "${KEEP_TMP:-no}" = yes ] &&
	mv "$TEST_TMP" "failed-${TEST_TMP}"
	echo >&2 "Temp directory kept as failed-${TEST_TMP}"
}

# The ERR trap is an extension allowed by
# POSIX. It is not supported by dash(1)...
trap 'keep_tmp' ERR || :
trap 'rm -fr "$TEST_TMP"' EXIT

# Test conditionals

HDECODE="hdecode fdecode"
HIGNORE=
NOTABLE=godecode

for xx in ng go
do
	test -x "./${xx}decode" && HDECODE="$HDECODE ${xx}decode"
done

readonly HDECODE
readonly NOTABLE

# Valgrind setup

MEMCHECK=${MEMCHECK:-no}
MEMCHECK_CMD="libtool --mode=execute	\
	valgrind			\
	--tool=memcheck			\
	--leak-check=full		\
	--show-leak-kinds=all		\
	--errors-for-leak-kinds=all	\
	--track-fds=yes			\
	--error-exitcode=99		\
	--log-file=memcheck-${TEST_NAM}-%p.log"

readonly MEMCHECK
readonly MEMCHECK_CMD

[ "$MEMCHECK" = yes ] && HIGNORE="godecode"

# ZSH quirks

setopt shwordsplit >/dev/null 2>&1 || :

# Internal functions

memcheck() {
	if [ "$MEMCHECK" = yes ]
	then
		local rc=0
		$MEMCHECK_CMD "$@" || rc=$?
		[ $rc -eq 99 ] && echo >&2 memcheck: "$@"
		return $rc
	else
		"$@"
	fi
}

rm_comments() {
	sed -e '/^#/d'
}

rm_blanks() {
	sed -e '/^$/d'
}

skip_diff() {
	for opt
	do
		[ "$opt" = --expect-error ] && return
	done
	return 1
}

skip_cmd() {
	for cmd in $HIGNORE
	do
		[ "$cmd" = "$1" ] && return
	done
	return 1
}

skip_tbl() {
	for cmd in $NOTABLE
	do
		[ "$cmd" = "$1" ] && return
	done
	return 1
}

# Test fixtures

cmd_check() {
	missing=

	for cmd
	do
		command -v "$cmd" >/dev/null 2>&1 ||
		missing="$missing $cmd"
	done

	if [ -n "$missing" ]
	then
		echo "program not found:$missing" >&2
		return 1
	fi
}

mk_hex() {
	"$TEST_DIR/hex_decode" |
	"$TEST_DIR/hex_encode" >"$TEST_TMP/hex"
	"$TEST_DIR/hex_decode" <"$TEST_TMP/hex" >"$TEST_TMP/bin"
}

mk_bin() {
	printf '%s\n' 'mk_bin ---' >&2
	while read -r bin pipe comment
	do
		dec=$(printf 'ibase=2; %s\n' "$bin" | bc)
		printf '%02x' "$dec"
		printf '%02x: %s %s %s\n' "$dec" "$bin" "$pipe" "$comment" >&2
	done |
	mk_hex
	printf '%s\n' --- >&2
}

mk_msg() {
	rm_comments | rm_blanks >"$TEST_TMP/msg"
	echo >> "$TEST_TMP/msg"
}

mk_tbl() {
	msg="Dynamic Table (after decoding):"
	rm_comments >"$TEST_TMP/tbl_tmp"

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
	rm_comments | rm_blanks >"$TEST_TMP/enc"
}

mk_chars() {
	fmt=$(printf %s "$2" | tr ' ' '\t')
	# shellcheck disable=SC2059
	printf "$fmt" ' ' |
	tr '\t ' " $1"
}

hpack_decode() {
	printf "hpack_decode: %s\n" "$*" >&2
	memcheck "$@" "$TEST_TMP/bin" >"$TEST_TMP/dec_out"
}

hpack_encode() {
	printf "hpack_encode: %s\n" "$*" >&2
	memcheck "$@" <"$TEST_TMP/enc" \
		1>"$TEST_TMP/enc_bin" \
		3>"$TEST_TMP/enc_tbl"
}

tst_ignore() (
	set -e
	HIGNORE="$HIGNORE $1"
	shift
	"$@"
)

tst_solely() (
	set -e
	for cmd in $HDECODE
	do
		[ "$cmd" != "$1" ] && HIGNORE="$HIGNORE $cmd"
	done
	shift
	"$@"
)

tst_decode() {
	for dec in $HDECODE
	do
		skip_cmd "$dec" && continue

		hpack_decode "./$dec" "$@"

		skip_diff "$@" && continue

		printf "Decoded header list:\n\n" |
		cat - "$TEST_TMP/msg" >"$TEST_TMP/out"

		skip_tbl "$dec" || cat "$TEST_TMP/tbl" >>"$TEST_TMP/out"

		diff -u "$TEST_TMP/out" "$TEST_TMP/dec_out"
	done
}

tst_encode() {
	hpack_encode ./hencode "$@"

	skip_diff "$@" && return

	"$TEST_DIR/hex_encode" <"$TEST_TMP/enc_bin" >"$TEST_TMP/enc_hex"

	diff -u "$TEST_TMP/hex" "$TEST_TMP/enc_hex"
	diff -u "$TEST_TMP/tbl" "$TEST_TMP/enc_tbl"
}

err_decode() {
	for dec in $HDECODE
	do
		skip_cmd "$dec" && continue
		printf "err_decode: %s\n" "$*" >&2
		! "./$dec" "$@" "$TEST_TMP/bin" >"$TEST_TMP/dec_out"
	done
}

err_encode() {
	printf "err_encode: %s\n" "$*" >&2
	! ./hencode "$@" <"$TEST_TMP/enc" \
		1>"$TEST_TMP/enc_bin" \
		3>"$TEST_TMP/enc_tbl"
}

tst_repeat() {
	i=1
	j="$1"
	shift
	while [ "$i" -lt "$j" ]
	do
		"$@" $i
		i=$((i + 1))
	done
}

_() {
	if expr "$1" : '^-' >/dev/null
	then
		echo "------$*"
	else
		echo "TEST: $*"
	fi
}
