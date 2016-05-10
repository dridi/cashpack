/*-
 * Copyright (c) 2016 Dridi Boukelmoune
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

#ifdef HPR
#ifndef HPR_ERRORS_ONLY
HPR(OK,   0, "Success")
#endif
HPR(ARG,  -1, "Wrong argument")
HPR(BUF,  -2, "Buffer overflow")
HPR(INT,  -3, "Integer overflow")
HPR(IDX,  -4, "Invalid index")
HPR(LEN,  -5, "Invalid length")
HPR(HUF,  -6, "Invalid Huffman code")
HPR(CHR,  -7, "Invalid character")
HPR(UPD,  -8, "Spurious update")
HPR(RSZ,  -9, "Missing resize update")
HPR(OOM, -10, "Out of memory")
#ifndef HPR_ERRORS_ONLY
HPR(BLK, -11, "Incomplete block")
#endif
HPR(BSY, -12, "Decoder busy")
#endif

#ifdef HPE
HPE(FIELD, 0, "new field",
	"\tA decoder sends a FIELD event to notify that a new field is\n"
	"\tbeing decoded. It will be followed by at least one NAME event\n"
	"\tand one VALUE event. The *buf* argument is always ``NULL`` and\n"
	"\t*len* represents the index of the field in the dynamic table or\n"
	"\tzero for non indexed fields.\n\n"

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
	"\tchanged during an operation. Both *buf* and *len* are\n"
	"\tunused and respectively ``NULL`` and zero.\n\n")

HPE(NAME,  3, "field name",
	"\tA decoder sends a NAME event when the current field's name has\n"
	"\tbeen or will be decoded. When *buf* is not ``NULL``, it points\n"
	"\tto the *len* characters of the name string. The string is NOT\n"
	"\tnull-terminated. A ``NULL`` *buf* means that there are *len*\n"
	"\tHuffman octets to decode. In the worst case, the decoded string\n"
	"\tlength is ``1.6 * len`` and can be used as a baseline for a\n"
	"\tpreallocation. The decoded contents will be notified by at least\n"
	"\tone DATA event.\n\n"

	"\tWhen the contents of the dynamic table are listed, exactly one\n"
	"\tNAME event follows a FIELD event. The *buf* argument points to\n"
	"\ta null-terminated string of *len* characters.\n\n")

HPE(VALUE, 4, "field Value",
	"\tThe VALUE event is identical to the NAME event, and always comes\n"
	"\tafter the NAME event of a field. Instead of referring to the\n"
	"\tfield's name, it signals its value.\n\n")

HPE(DATA,  5, "raw data",
	"\tA decoder sends DATA events for every chunks of Huffman octets\n"
	"\tsuccessfully decoded.\n\n"

	"\tAn encoder sends DATA events when the encoding buffer is full,\n"
	"\tor when the encoding process is over and there are remaining\n"
	"\toctets\n\n"

	"\tIn both cases *buf* points to *len* octets of data.\n\n")

HPE(EVICT, 6, "a field was evicted",
	"\tA decoder or an encoder sends an EVICT event to notify that a\n"
	"\tfield was evicted from the dynamic table during the processing\n"
	"\tof a header list.\n\n"

	"\tThe role of this event is to inform that the dynamic table\n"
	"\tchanged during an operation. Both *buf* and *len* are\n"
	"\tunused and respectively ``NULL`` and zero.\n\n")

HPE(TABLE, 7, "the table was updated",
	"\tA decoder or an encoder sends a TABLE event when a dynamic table\n"
	"\tupdate is decoded or encoded. The *buf* argument is always\n"
	"\t``NULL`` and *len* is the new table maximum size.\n\n")
#endif
