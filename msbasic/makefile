##########################################################
# 
# Copyright (c) 2024 Simon Southwell. All rights reserved.
#
# Date: 15th February 2024
#
# Linux makefile for cpu6502 MS Basic
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

SYSTEM        = cpu6502

TARGET        = $(SYSTEM).bin
SRC           = msbasic.s

OBJDIR        = obj

THISDIR       = $(PWD)

MODELDIR      = ../wozmon
MODELEXE      = $(THISDIR)/main.exe


# --------------------------------------------------------
# Build rules
# --------------------------------------------------------

all: $(TARGET) model

$(TARGET): $(OBJDIR)
	@ca65 -D$(SYSTEM) $(SRC) -o $(OBJDIR)/$(SYSTEM).o
	@ld65 -C $(SYSTEM).cfg $(OBJDIR)/$(SYSTEM).o -o $(TARGET) -Ln $(OBJDIR)/$(SYSTEM).lbl

$(OBJDIR):
	@mkdir -p $(OBJDIR)

model:
	@make --no-print-directory -C $(MODELDIR) OPDIR=$(THISDIR) $(MODELEXE)

# --------------------------------------------------------
# Clean rules
# --------------------------------------------------------

clean:
	@make --no-print-directory -C $(MODELDIR) OPDIR=$(THISDIR) clean
	@rm -rf $(TARGET) $(OBJDIR)
    
# --------------------------------------------------------
# Execution rules
# --------------------------------------------------------

run: all
	@./main.exe -n -tBIN -l 0x8000 -r 0x8000

