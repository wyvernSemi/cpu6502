##########################################################
# 
# Copyright (c) 2017 Simon Southwell. All rights reserved.
#
# Date: 12th January 2017
#
# Linux makefile for 'cpu6502' instruction set simulator
# 
# This file is part of the cpu6502 instruction set simulator.
#
# cpu6502 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpu6502 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpu6502. If not, see <http://www.gnu.org/licenses/>.
#
# $Id: makefile,v 1.1 2017/01/13 09:05:13 simon Exp $
# $Source: /home/simon/CVS/src/cpu/cpu6502/makefile,v $
# 
##########################################################

##########################################################
# Definitions
##########################################################

TARGET=cpu6502

SRCDIR=./src
TESTDIR=./test

SRCFILES=cpu6502.cpp read_ihx.cpp
TESTSRC=test.a65
TESTTGT=test.hex

COMMINCL=read_ihx.h cpu6502.h

USROPTS=-DWY65_STANDALONE
COPTS=-Ofast -I${SRCDIR}/ -Wno-write-strings

CC=g++

##########################################################
# Dependency definitions
##########################################################

all: ${TARGET}

${SRCDIR}/cpu6502.cpp  : ${COMMINCL:%=${SRCDIR}/%} ${SRCDIR}/cpu6502.h
${SRCDIR}/read_ihx.cpp : ${COMMINCL:%=${SRCDIR}/%}

##########################################################
# Compilation rules
##########################################################

${TARGET}: ${SRCFILES:%=${SRCDIR}/%}
	@${CC} ${COPTS} ${USROPTS} $^ -o $@

##########################################################
# Test
#
# Requires Frank Kingswood's as65 assembler
#
# http://www.kingswood-consulting.co.uk/assemblers/as65_142.zip
#
##########################################################

${TESTDIR}/${TESTTGT} : ${TESTDIR}/${TESTSRC}
	@cd ${TESTDIR} && as65 -s2 ${TESTSRC}

test: ${TARGET} ${TESTDIR}/${TESTTGT}
	@${TARGET} -I ${TESTDIR}/test.hex

##########################################################
# Clean up rules
##########################################################

clean:
	@rm -rf ${TARGET} ${TESTDIR}/${TESTTGT}

