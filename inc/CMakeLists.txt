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

message(STATUS "Detecting astyle program")
find_program(ASTYLE astyle)
if(NOT EXISTS ${ASTYLE})
	message(STATUS "Detecting astyle program - missing")
	set(ASTYLE "true")
else()
	message(STATUS "Detecting astyle program - done")
endif()

add_custom_target(hdr DEPENDS hpack.h)

add_custom_command(
	OUTPUT	hpack.h
	COMMAND	sh -c "cpp ${CMAKE_CURRENT_SOURCE_DIR}/hpack.h.in >hpack.h"
	COMMAND	sed -i "/^#/d"  hpack.h
	COMMAND	${ASTYLE} -q -t hpack.h
	DEPENDS	hpack.h.in)

install (
	FILES		${CMAKE_CURRENT_BINARY_DIR}/hpack.h
	DESTINATION	${INCLUDE_INSTALL_DIR})
