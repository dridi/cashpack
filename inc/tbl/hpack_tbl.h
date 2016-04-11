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
HPR(ARG, -1, "Wrong argument")
HPR(BUF, -2, "Buffer overflow")
HPR(INT, -3, "Integer overflow")
HPR(IDX, -4, "Invalid index")
HPR(LEN, -5, "Invalid length")
HPR(HUF, -6, "Invalid Huffman code")
HPR(CHR, -7, "Invalid character")
#endif

#ifdef HPE
HPE(FIELD, 0, "New field")
HPE(NEVER, 1, "Field is never indexed")
HPE(INDEX, 2, "Field was indexed")
HPE(NAME,  3, "New name string")
HPE(VALUE, 4, "New Value string")
HPE(DATA,  5, "String data")
HPE(EVICT, 6, "A field was evicted")
HPE(TABLE, 7, "Table update")
#endif
