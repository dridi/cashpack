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

cmake_minimum_required(VERSION 3.0)

project(
	cashpack
	VERSION		0.1
	LANGUAGES	C)

enable_testing()

set(PROJECT_LIB_ABI	0)
set(PROJECT_C_FLAGS	"-O2 -Wall -Wextra -Werror")

set(CMAKE_C_FLAGS "${PROJECT_C_FLAGS} ${CMAKE_C_FLAGS}")

set(LIB_INSTALL_DIR lib CACHE PATH "Where to install the library.")
set(INCLUDE_INSTALL_DIR include CACHE PATH "Where to install the header.")

include_directories(
	"${CMAKE_SOURCE_DIR}/inc"
	"${CMAKE_BINARY_DIR}/gen")

add_subdirectory(inc)
add_subdirectory(gen)
add_subdirectory(lib)
add_subdirectory(tst)
