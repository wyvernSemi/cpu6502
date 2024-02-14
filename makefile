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
# $Id: makefile,v 1.6 2017/02/11 08:45:33 simon Exp $
# $Source: /home/simon/CVS/src/cpu/cpu6502/makefile,v $
# 
##########################################################

##########################################################
# Definitions
##########################################################

TARGET=cpu6502
TESTTGT=test.hex
TESTTGT2=test_65c02.hex

SRCDIR=./src
TESTDIR=./test
OBJDIR=./obj

SRCFILES=cpu6502.cpp read_ihx.cpp
TESTSRC=test.a65
TESTSRC2=test_65c02.a65

COMMINCL=read_ihx.h cpu6502.h cpu6502_api.h
TGTINCL=cpu6502.h

OBJECTS=${SRCFILES:%.cpp=%.o}

# Default user and C compile options, which can be
# overidden
USROPTS=
STDALONE=-DWY65_STANDALONE
COPTS=-Ofast

# Common C options (don't override)
COPTSCOMM=-I${SRCDIR}/ -Wno-write-strings ${STDALONE}

CC=g++
ASM=as65

# Test program start address
TSTADDR=0x400

# Conditionally set  options to enable coverge only if
# DOCOV defined
ifdef DOCOV
  COPTS=-g
  COVOPTS=-coverage

  # Test options to increase coverage
  TESTCOVOPTS=-f dummy -M dummy -l 0x000a -d 0xffff -i 0xffffffff -S 0x00100000 -E0x0010ffff
endif

# GCC in CYGWIN gives tedious warnings that all code is 
# relocatable, and so -fPIC not required. So shut it up, 
# and only add for Linux.
OSTYPE:=$(shell uname -o)
ifneq (${OSTYPE}, Cygwin)  
  COPTS += -fPIC
endif

COVEXCL=
LCOVINFO=${TARGET}.info
COVLOGFILE=cov.log
COVDIR=cov_html

# Default to no echoing of commands, but can be overridden
# on command line (VERBOSE=1)
ifndef VERBOSE
.SILENT:
endif

##########################################################
# Dependency definitions
##########################################################

# By default, build the standalone executable, the static
# and dynamic libraries 
all: ${TARGET} lib${TARGET}.a lib${TARGET}.so

${OBJDIR}/cpu6502.o:  ${COMMINCL:%=${SRCDIR}/%} ${TGTINCL:%=${SRCDIR}/%}
${OBJDIR}/read_ihx.o: ${COMMINCL:%=${SRCDIR}/%}

##########################################################
# Compilation rules
##########################################################

# Main target (standalone executable)
${TARGET}: ${OBJECTS:%=${OBJDIR}/%}
	${CC} ${COPTS} ${COPTSCOMM} ${USROPTS} ${COVOPTS} $^ -o $@

# Static library    
lib${TARGET}.a: ${OBJECTS:%=${OBJDIR}/%}
	rm -f $@
	ar rcs $@ ${OBJECTS:%=${OBJDIR}/%}

# Shared object    
lib${TARGET}.so: ${OBJECTS:%=${OBJDIR}/%}
	@$(CC) -shared -Wl,-soname,$@ -o $@ ${OBJECTS:%=${OBJDIR}/%}

# Relocatable objects    
${OBJDIR}/%.o : ${SRCDIR}/%.cpp
	mkdir -p ${OBJDIR}
	$(CC) ${ARCHOPT} ${COPTS} ${COPTSCOMM} ${USROPTS} ${COVOPTS} -c $< -o $@ 

##########################################################
# Test
#
# Requires Frank Kingswood's as65 assembler:
#   http://www.kingswood-consulting.co.uk/assemblers/as65_142.zip
#
##########################################################

ifndef DOCOV

${TESTDIR}/${TESTTGT} : ${TESTDIR}/${TESTSRC}
	cd ${TESTDIR} && ${ASM} -q -s2 ${TESTSRC}

${TESTDIR}/${TESTTGT2} : ${TESTDIR}/${TESTSRC2}
	cd ${TESTDIR} && ${ASM} -q -s2 -x ${TESTSRC2}

test: ${TARGET} ${TESTDIR}/${TESTTGT} ${TESTDIR}/${TESTTGT2}
	./${TARGET} -I ${TESTDIR}/${TESTTGT}  -s ${TSTADDR}
	./${TARGET} -I ${TESTDIR}/${TESTTGT2} -s ${TSTADDR} -c

else

# When DOCOV defined, run test three times for each of the
# input file formats

${TESTDIR}/${TESTTGT} : ${TESTDIR}/${TESTSRC}
	cd ${TESTDIR} && ${ASM} -q ${TESTSRC} && ${ASM} -q -s ${TESTSRC} && ${ASM} -q -s2 ${TESTSRC}

${TESTDIR}/${TESTTGT2} : ${TESTDIR}/${TESTSRC2}
	cd ${TESTDIR} && ${ASM} -qx ${TESTSRC2} && ${ASM} -qx -s ${TESTSRC2} && ${ASM} -qx -s2 ${TESTSRC2}

test: ${TARGET} ${TESTDIR}/${TESTTGT} ${TESTDIR}/${TESTTGT2}
	./${TARGET} ${TESTCOVOPTS} -I ${TESTDIR}/${TESTTGT}              -s ${TSTADDR}
	./${TARGET} ${TESTCOVOPTS} -M ${TESTDIR}/${TESTTGT:%.hex=%.s19}  -s ${TSTADDR}
	./${TARGET} ${TESTCOVOPTS} -f ${TESTDIR}/${TESTTGT:%.hex=%.bin}  -s ${TSTADDR}
	./${TARGET} ${TESTCOVOPTS} -I ${TESTDIR}/${TESTTGT2}             -s ${TSTADDR} -c
	./${TARGET} ${TESTCOVOPTS} -M ${TESTDIR}/${TESTTGT2:%.hex=%.s19} -s ${TSTADDR} -c
	./${TARGET} ${TESTCOVOPTS} -f ${TESTDIR}/${TESTTGT2:%.hex=%.bin} -s ${TSTADDR} -c
endif

##########################################################
# coverage
##########################################################

# Coverage target only valid if DOCOV defined
ifdef DOCOV

coverage: test
	lcov -c -d ${OBJDIR} -o ${LCOVINFO} > ${COVLOGFILE}
	lcov -r ${LCOVINFO} ${COVEXCL} -o ${LCOVINFO} >> ${COVLOGFILE}
	genhtml -o ${COVDIR} ${LCOVINFO} >> ${COVLOGFILE}

else

.PHONY: coverage
coverage:
	echo "Compile with DOCOV defined (e.g. 'make clean && make DOCOV=1 coverage')"

endif

##########################################################
# Clean up rules
##########################################################

clean:
	rm -rf ${TARGET} lib${TARGET}.a lib${TARGET}.so\
	       ${TESTDIR}/${TESTTGT} ${TESTDIR}/${TESTTGT:%.hex=%.bin} \
	       ${TESTDIR}/${TESTTGT:%.hex=%.s19} ${TESTDIR}/${TESTTGT:%.hex=%.lst} \
	       ${TESTDIR}/${TESTTGT2} ${TESTDIR}/${TESTTGT2:%.hex=%.bin} \
	       ${TESTDIR}/${TESTTGT2:%.hex=%.s19} ${TESTDIR}/${TESTTGT2:%.hex=%.lst} \
	       ${OBJDIR} ${COVDIR} ${LCOVINFO} ${COVLOGFILE} *.gc* ${TARGET}.log

