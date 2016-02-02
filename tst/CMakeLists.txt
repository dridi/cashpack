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

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

macro(tst_cmd CMD)
	add_executable(${CMD} ${CMD}.c)
	target_link_libraries(${CMD} hpack)
	add_dependencies(check ${CMD})
endmacro(tst_cmd)

macro(tst_add TST)
	add_test(${TST} ${CMAKE_CURRENT_SOURCE_DIR}/${TST})
	set_property(TEST ${TST} PROPERTY ENVIRONMENT "MEMCHECK=${MEMCHECK}")
endmacro(tst_add)

tst_cmd(hdecode)

message(STATUS "Check for working hexdumps")

execute_process(
	COMMAND		${CMAKE_CURRENT_SOURCE_DIR}/hexcheck
	RESULT_VARIABLE	HEXCHECK_RESULT
	ERROR_VARIABLE	HEXCHECK_ERROR)

execute_process(
	COMMAND		${CMAKE_CURRENT_SOURCE_DIR}/bincheck
	RESULT_VARIABLE	BINCHECK_RESULT
	ERROR_VARIABLE	BINCHECK_ERROR)

if(${HEXCHECK_RESULT} EQUAL 0 AND ${BINCHECK_RESULT} EQUAL 0)
	message(STATUS "Check for working hexdumps - works")

	# RFC examples
	tst_add(rfc7541_c_2_1)
	tst_add(rfc7541_c_2_2)
	tst_add(rfc7541_c_2_3)
	tst_add(rfc7541_c_2_4)
	tst_add(rfc7541_c_3_1)
	tst_add(rfc7541_c_3_2)
	tst_add(rfc7541_c_3_3)
	tst_add(rfc7541_c_4_1)
	tst_add(rfc7541_c_4_2)
	tst_add(rfc7541_c_4_3)
	tst_add(rfc7541_c_5_1)
	tst_add(rfc7541_c_5_2)
	tst_add(rfc7541_c_5_3)
	tst_add(rfc7541_c_6_1)
	tst_add(rfc7541_c_6_2)
	tst_add(rfc7541_c_6_3)

	# additional coverage
	tst_add(hpack_cov)
	tst_add(hpack_huf)
	tst_add(hpack_int)
	tst_add(hpack_tbl)

	# regressions
	tst_add(afl_fuzz)
else()
	message(STATUS	"Check for working hexdumps - broken")
	message(FATAL_ERROR
		"The test suite can't be run:"
		"\nhexcheck: ${HEXCHECK_ERROR}"
		"\nbincheck: ${BINCHECK_ERROR}")
endif()

if(COVERAGE)
	message(STATUS "Enabling lcov support")

	add_custom_target(lcov-init COMMAND ${LCOV} -z -d .)
	add_custom_target(
		lcov
		COMMAND ${LCOV} -c -o tst.info -d .
		COMMAND ${LCOV} -c -o lib.info -d ../lib
		COMMAND ${LCOV} -a tst.info -a lib.info -o raw.info
		COMMAND ${LCOV} -r raw.info '/usr/*' 'inc/*' -o cashpack.info
		COMMAND ${GENHTML} -o lcov cashpack.info)

	add_dependencies(check lcov-init)
	add_dependencies(lcov check)

	message(STATUS "Enabling lcov support - done")
endif ()
