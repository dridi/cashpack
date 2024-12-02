#if 0
/*-
 * Copyright (c) 2016-2017 Dridi Boukelmoune
 * All rights reserved.
 *
 * Author: Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#endif

#ifdef HPR
#ifndef HPR_ERRORS_ONLY
HPR(NAM, 3, "name-only match",
	"\tAn index search only matched a field name. This is not an\n"
	"\terror.\n\n")

HPR(FLD, 2, "decoded field",
	"\tThe expected result when decoding fields only. This is not an\n"
	"\terror.\n\n")

HPR(BLK, 1, "incomplete block",
	"\tThe expected result during partial decoding. This is not an\n"
	"\terror.\n\n")

HPR(OK, 0, "success",
	"\tThe operation succeeded. This is not an error.\n\n")
#endif

HPR(ARG, -1, "invalid argument",
	"\tThis may include unexpected ``NULL`` pointers, values out of\n"
	"\trange, usage of a defunct codec... This error reflects a misuse\n"
	"\tof the library.\n\n")

HPR(BUF, -2, "buffer overflow",
	"\tThe decoding buffer ends before the decoded HPACK block, reading\n"
	"\tfurther would result in a buffer overflow. This error is turned\n"
	"\tinto a BLK result when decoding is partial.\n\n")

HPR(INT, -3, "integer overflow",
	"\tDecoding of an integer gives a value too large.\n\n")

HPR(IDX, -4, "invalid index",
	"\tThe decoded or specified index is out of range. In the case of\n"
	"\tan index search, no match was found.\n\n")

HPR(LEN, -5, "invalid length",
	"\tAn invalid length may refer to header fields with an empty name\n"
	"\tor a table size that exceeds the maximum. Anything that doesn't\n"
	"\tmeet a length requirement.\n\n")

HPR(HUF, -6, "invalid Huffman code",
	"\tA decoder decoded an invalid Huffman sequence.\n\n")

HPR(CHR, -7, "invalid character",
	"\tAn invalid header name or value character was coded.\n\n")

HPR(UPD, -8, "spurious update",
	"\tA table update occurred at a wrong time or with a wrong size.\n\n")

HPR(RSZ, -9, "missing resize update",
	"\tA table update was expected after a table resize but didn't\n"
	"\toccur.\n\n")

HPR(OOM, -10, "out of memory",
	"\tA reallocation failed during a table update.\n\n")

HPR(BSY, -11, "codec busy",
	"\tSome operations such as listing the contents of the dynamic\n"
	"\ttable can't be performed while a block is being decoded.\n\n")

HPR(SKP, -12, "message skipped",
	"\tThe request or response being decoded doesn't fit in the output\n"
	"\tbuffer. This error can be recovered from by skipping the large\n"
	"\tmessage.\n\n")

HPR(HDR, -13, "undefined pseudo-header",
	"\tA header list contains an undefined pseudo-header.\n\n")

HPR(REA, -14, "missing realloc function",
	"\tThe codec was initialized without a realloc function but used in\n"
	"\ta way that required a realloc operation.\n\n")

HPR(BIG, -15, "header too big",
	"\tThe attempt at skipping a message too big failed. There was a\n"
	"\tsingle field too large for the whole decoding buffer.\n\n")
#endif /* HPR */

#ifdef HPE
HPE(FIELD, 0, "new field",
	"\tA decoder sends a FIELD event to notify that a new field is\n"
	"\tbeing decoded. It will be followed by at least one NAME event\n"
	"\tand one VALUE event. The *buf* argument is always ``NULL`` and\n"
	"\t*len* represents the index of the field in the dynamic table or\n"
	"\tzero for non indexed fields.\n\n"

	"\tAn encoder sends a FIELD event before its processing. This gives\n"
	"\tan opportunity to change the field before it is consumed by the\n"
	"\tencoder. For instance, by the time its turn comes, and index may\n"
	"\tneed to be incremented or its reference could have been evicted.\n"
	"\tThe *buf* argument is always ``NULL`` and *len* always zero.\n\n"

	"\tWhen the contents of the dynamic table are listed, a FIELD event\n"
	"\tis sent for every field, followed by exactly one NAME plus one\n"
	"\tVALUE events. The *buf* argument is always ``NULL`` and *len* is\n"
	"\tthe size (including overhead) of the dynamic table entry.\n\n")

HPE(NEVER, 1, "the field is never indexed",
	"\tA decoder sends a NEVER event to inform that the field being\n"
	"\tdecoded and previously signalled by a FIELD event should never\n"
	"\tbe indexed by intermediate proxies. Both *buf* and *len* are\n"
	"\tunused and respectively ``NULL`` and zero.\n\n")

HPE(INDEX, 2, "a field was indexed",
	"\tA decoder sends an INDEX event to notify that the field decoded\n"
	"\tand previously signalled by a FIELD event was successfully\n"
	"\tinserted in the dynamic table.\n\n"

	"\tAn encoder sends an INDEX event to notify that a field was\n"
	"\tinserted in the dynamic table during encoding\n\n"

	"\tThe role of this event is to inform that the dynamic table\n"
	"\tchanged during an operation. *buf* is unused and always\n"
	"\t``NULL``. *len* is the size of the entry in the dynamic table,\n"
	"\toverhead included.\n\n")

HPE(NAME,  3, "field name",
	"\tA decoder sends a NAME event when the current field's name has\n"
	"\tbeen or will be decoded. When *buf* is not ``NULL``, it points\n"
	"\tto the *len* characters of the name string. The string is NOT\n"
	"\tnull-terminated. A ``NULL`` *buf* means that there are *len*\n"
	"\tHuffman octets to decode, or that the HPACK block expects a\n"
	"\tcontinuation in the middle of this string. In the worst case,\n"
	"\tthe decoded string length is ``1.6 * len`` and can be used as a\n"
	"\tbaseline for a preallocation. When *buf* is ``NULL`` the decoded\n"
	"\tcontents will be notified by at least one DATA event.\n\n"

	"\tWhen the contents of the dynamic table are listed, exactly one\n"
	"\tNAME event follows a FIELD event. The *buf* argument points to\n"
	"\ta null-terminated string of *len* characters.\n\n")

HPE(VALUE, 4, "field Value",
	"\tThe VALUE event is identical to the NAME event, and always comes\n"
	"\tafter the NAME event of a field. Instead of referring to the\n"
	"\tfield's name, it signals its value.\n\n")

HPE(DATA,  5, "raw data",
	"\tAn encoder sends DATA events when the encoding buffer is full,\n"
	"\tor when the encoding process is over and there are remaining\n"
	"\toctets.\n\n"

	"\tIn both cases *buf* points to *len* octets of data.\n\n")

HPE(EVICT, 6, "fields were evicted",
	"\tA decoder or an encoder sends an EVICT event to notify that some\n"
	"\tfields were evicted from the dynamic table during the processing\n"
	"\tof a header list.\n\n"

	"\tThe role of this event is to inform that the dynamic table\n"
	"\tchanged during an operation. *buf* is unused and always ``NULL``\n"
	"\tand *len* is the number of evicted fields.\n\n")

HPE(TABLE, 7, "the table was updated",
	"\tA decoder or an encoder sends a TABLE event when a dynamic table\n"
	"\tupdate is decoded or encoded. The *buf* argument is always\n"
	"\t``NULL`` and *len* is the new table maximum size.\n\n")

HPE(PTR, 8, "header line pointer",
    "\tA decoder sends a PTR event containing the address pointer of the\n"
    "\tstart byte of a header line.\n"
    "\tThis event will be emitted only when ``HPACK_CFG_SEND_PTR`` is set.\n\n")

HPE(RECERR, 9, "recoverable error",
    "\tA decoder sends a RECERR event containing the address pointer of\n"
    "\tthe byte in error, which may be in the middle of a header line.\n"
    "\tThis event will be emitted only when ``HPACK_CFG_SEND_ERR`` is set.\n\n")

#endif /* HPE */

#ifdef HPF
HPF(TYP_IDX, 0x01,
	"\tThe field is indexed, the *idx* member MUST be a valid index.\n\n")

HPF(TYP_DYN, 0x02,
	"\tThe field will be inserted in the dynamic table, if it fits.\n"
	"\tThe members *nam* and *val* are expected to point to the name\n"
	"\tvalue strings.\n\n")

HPF(TYP_LIT, 0x04,
	"\tA literal field without indexing. The members *nam* and *val*\n"
	"\tare expected to point to the name value strings.\n\n")

HPF(TYP_NVR, 0x08,
	"\tA literal field that should never be indexed. The members *nam*\n"
	"\tand *val* are expected to point to the name value strings.\n\n")

HPF(TYP_MSK, 0x0f,
	"\tThe bit mask to extract the type-of-field flag. Not an actual\n"
	"\tflag.\n\n")

HPF(NAM_IDX, 0x10,
	"\tThe field name is indexed. The member *nam_idx* MUST be a valid\n"
	"\tindex. It supersedes the *nam* pointer, and can be used for any\n"
	"\ttype of field except ``TYP_IDX``.\n\n")

HPF(NAM_HUF, 0x20,
	"\tThe field name shall be Huffman-encoded. It can be used for any\n"
	"\ttype of field except ``TYP_IDX`` or fields with ``NAM_IDX``.\n\n")

HPF(VAL_HUF, 0x40,
	"\tThe field value shall be Huffman-encoded. It can be used for any\n"
	"\ttype of field except ``TYP_IDX``.\n\n")

HPF(AUT_IDX, 0x80,
	"\tAutomatically look for a best match in the index and use it to\n"
	"\treduce the size of the encoded block. When ``TYP_NVR`` is set,\n"
	"\tonly a name match is performed. It is ignored when used together\n"
	"\twith either ``TYP_IDX`` or ``NAM_IDX``. If an exact match (both\n"
	"\tname and value) is found, either ``TYP_DYN`` or ``TYP_LIT`` are\n"
	"\tturned into ``TYP_IDX``. If a match is found, *idx* or *nam_idx*\n"
	"\tis set to non-zero, zero otherwise.\n\n")

#endif /* HPF */

#ifdef HPC
/* configuration flags */
HPC(DEGRADED, 0x01,
    "\tDegraded mode tries to recover from missing indexes in the dynamic\n"
    "\ttable. A missing header name will be replaced by ``unknown_name``,\n"
    "\tand a missing value will be replaced by ``unknown_value``.\n"
    "\t\n\n")

HPC(SEND_PTR, 0x02,
    "\tSends a ``PTR`` event at the start of every header line.\n"
    "\tThe address of the header line in the header block will be placed\n"
    "\tin the ``const char *buf`` parameter of the callback.\n\n")

HPC(SEND_ERR, 0x04,
    "\tSends a ``RECERR`` event whenever a recoverable error occurs.\n"
    "\tThe address of the error in the header block will be placed\n"
    "\tin the ``const char *buf`` parameter of the callback.\n"
    "\tThis parameter can be used with the ``SEND_PTR`` parameter\n"
    "\tto be able to spot and/or extract errored header lines from\n"
    "\ta header block.\n"
    "\tNote: this is mostly useful in ``DEGRADED`` mode.\n\n")

#endif /* HPC */

#ifdef HPP
HPP(STR, 7, 0x00) /* Section 5.2 */
HPP(HUF, 7, 0x80) /* Section 5.2 */
HPP(IDX, 7, 0x80) /* Section 6.1 */
HPP(DYN, 6, 0x40) /* Section 6.2.1 */
HPP(LIT, 4, 0x00) /* Section 6.2.2 */
HPP(NVR, 4, 0x10) /* Section 6.2.3 */
HPP(UPD, 5, 0x20) /* Section 6.3 */
#endif
