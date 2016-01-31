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

macro(man_page SECTION PAGE)
	set(MAN ${PAGE}.${SECTION})
	set(RST ${MAN}.rst)
	set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${RST})
	set(RST2MAN_OPTS "--strip-comments")
	set(CMD "${RST2MAN} ${RST2MAN_OPTS} ${RST} ${MAN}")
	add_custom_command(
		OUTPUT	${MAN}.gz
		COMMAND cp ${SRC} ${RST}
		COMMAND	sh -c "${CMD}"
		COMMAND gzip -f ${MAN}
		DEPENDS	${RST})
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/${MAN}.gz
		DESTINATION ${MAN_INSTALL_DIR}/man${SECTION})
endmacro(man_page)

if(MANPAGES)
	file(GLOB RST_MANUALS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.rst)
	string(REPLACE .rst .gz MANUALS "${RST_MANUALS}")

	add_custom_target(man ALL DEPENDS ${MANUALS})

	man_page(3 cashpack)
endif()
