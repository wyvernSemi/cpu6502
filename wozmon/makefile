##########################################################
# 
# Copyright (c) 2024 Simon Southwell. All rights reserved.
#
# Date: 13th February 2024
#
# Linux makefile for 'cpu6502' model for running wozmon
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
##########################################################

# Get OS type
OSTYPE:=$(shell uname)

OPDIR           = .

WOZROOTNAME     = wozmon
WOZSRC          = $(WOZROOTNAME).asm

TARGETNAME      = cpu6502

MODELSRC        = main.cpp
MODELEXE        = $(OPDIR)/$(MODELSRC:%.cpp=%.exe)
MODELOPTS       = -g -DWOZMON -Wno-write-strings -I../src
MODELLIBS       = -L. -lcpu6502

ifeq (${OSTYPE}, Linux)
MODELLIBS       += -lncurses
endif

CPULIB          = libcpu6502.a
CPULIBOPTS      = STDALONE=""

ASMEXE          = vasm
ASMOPTS         = -dotdir -wdc02 -L $(WOZROOTNAME).lst

C++             = g++

# --------------------------------------------------------
# Build rules
# --------------------------------------------------------

all: $(MODELEXE) ihex

# Targets for different assembler output formats
bin:  $(TARGETNAME).bin
ihex: $(TARGETNAME).ihex
srec: $(TARGETNAME).srec

# Rule to compile cpu6502 CPU library
$(CPULIB):
	@make --no-print-directory -C .. $(CPULIBOPTS) $@
	@mv ../$(CPULIB) .

# Executable model
$(MODELEXE): $(MODELSRC) $(CPULIB)
	@$(C++) $(MODELOPTS) $^ $(MODELLIBS) -o $@

# Rule to assemble binary output
$(TARGETNAME).bin: $(WOZSRC)
	@$(ASMEXE) $(ASMOPTS) -Fbin $(WOZSRC) -o $@

# Rule to assemble Intel hex output
$(TARGETNAME).ihex: $(WOZSRC)
	@$(ASMEXE) $(ASMOPTS) -Fihex $(WOZSRC) -o $@

# Rule to assemble Motorola S-Record output
$(TARGETNAME).srec: $(WOZSRC)
	@$(ASMEXE) $(ASMOPTS) -Fsrec $(WOZSRC) -o $@

# --------------------------------------------------------
# Clean rules
# --------------------------------------------------------

clean:
	@make --no-print-directory -C .. clean
	@rm -rf $(TARGETNAME).bin $(TARGETNAME).ihex $(TARGETNAME).srec $(WOZROOTNAME).lst $(CPULIB) $(MODELEXE)
    
# --------------------------------------------------------
# Execution rules
# --------------------------------------------------------

run: all
	@./$(MODELEXE)