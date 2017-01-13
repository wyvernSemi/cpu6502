//=============================================================
// 
// Copyright (c) 2016 Simon Southwell. All rights reserved.
//
// Date: 18th December 2016
//
// This file is part of the cpu6502 instruction set simulator.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
// $Id: cpu6502.cpp,v 1.3 2017/01/12 20:18:07 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/cpu6502.cpp,v $
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "cpu6502.h"
#include "read_ihx.h"

// -------------------------------------------------------------------------
// cpu6502
//
// Class constructor
//
// -------------------------------------------------------------------------

cpu6502::cpu6502()
{
    // Reset internal class state
    ext_wr_mem        = NULL;
    ext_rd_mem        = NULL;
    fp                = NULL;
    nextPc            = INVALID_NEXT_PC;

    // Initialise instruction table
    int idx           = 0;

    set_tbl_entry(instr_tbl[idx++], "BRK", &cpu6502::BRK, 7, NON  /* 0x00 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 6, IDX  /* 0x01 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x02 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x03 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x04 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 3, ZPG  /* 0x05 */);
    set_tbl_entry(instr_tbl[idx++], "ASL", &cpu6502::ASL, 5, ZPG  /* 0x06 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 5, NON  /* 0x07 */);
    set_tbl_entry(instr_tbl[idx++], "PHP", &cpu6502::PHP, 3, NON  /* 0x08 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 2, IMM  /* 0x09 */);
    set_tbl_entry(instr_tbl[idx++], "ASL", &cpu6502::ASL, 2, ACC  /* 0x0A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x0B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x0C */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 4, ABS  /* 0x0D */);
    set_tbl_entry(instr_tbl[idx++], "ASL", &cpu6502::ASL, 6, ABS  /* 0x0E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x0F */);
    set_tbl_entry(instr_tbl[idx++], "BPL", &cpu6502::BPL, 2, REL  /* 0x10 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 5, IDY  /* 0x11 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x12 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x13 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x14 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 4, ZPX  /* 0x15 */);
    set_tbl_entry(instr_tbl[idx++], "ASL", &cpu6502::ASL, 6, ZPX  /* 0x16 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x17 */);
    set_tbl_entry(instr_tbl[idx++], "CLC", &cpu6502::CLC, 2, NON  /* 0x18 */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 4, ABY  /* 0x19 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0x1A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x1B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x1C */);
    set_tbl_entry(instr_tbl[idx++], "ORA", &cpu6502::ORA, 4, ABX  /* 0x1D */);
    set_tbl_entry(instr_tbl[idx++], "ASL", &cpu6502::ASL, 7, ABX  /* 0x1E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x1F */);
    set_tbl_entry(instr_tbl[idx++], "JSR", &cpu6502::JSR, 6, ABS  /* 0x20 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 6, IDX  /* 0x21 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x22 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x23 */);
    set_tbl_entry(instr_tbl[idx++], "BIT", &cpu6502::BIT, 3, ZPG  /* 0x24 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 3, ZPG  /* 0x25 */);
    set_tbl_entry(instr_tbl[idx++], "ROL", &cpu6502::ROL, 5, ZPG  /* 0x26 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x27 */);
    set_tbl_entry(instr_tbl[idx++], "PLP", &cpu6502::PLP, 4, NON  /* 0x28 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 2, IMM  /* 0x29 */);
    set_tbl_entry(instr_tbl[idx++], "ROL", &cpu6502::ROL, 2, ACC  /* 0x2A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x2B */);
    set_tbl_entry(instr_tbl[idx++], "BIT", &cpu6502::BIT, 4, ABS  /* 0x2C */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 4, ABS  /* 0x2D */);
    set_tbl_entry(instr_tbl[idx++], "ROL", &cpu6502::ROL, 6, ABS  /* 0x2E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x2F */);
    set_tbl_entry(instr_tbl[idx++], "BMI", &cpu6502::BMI, 2, REL  /* 0x30 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 5, IDY  /* 0x31 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x32 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x33 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x34 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 4, ZPX  /* 0x35 */);
    set_tbl_entry(instr_tbl[idx++], "ROL", &cpu6502::ROL, 6, ZPX  /* 0x36 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x37 */);
    set_tbl_entry(instr_tbl[idx++], "SEC", &cpu6502::SEC, 2, NON  /* 0x38 */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 4, ABY  /* 0x39 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0x3A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x3B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x3C */);
    set_tbl_entry(instr_tbl[idx++], "AND", &cpu6502::AND, 4, ABX  /* 0x3D */);
    set_tbl_entry(instr_tbl[idx++], "ROL", &cpu6502::ROL, 7, ABX  /* 0x3E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x3F */);
    set_tbl_entry(instr_tbl[idx++], "RTI", &cpu6502::RTI, 6, NON  /* 0x40 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 6, IDX  /* 0x41 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x42 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x43 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x44 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 3, ZPG  /* 0x45 */);
    set_tbl_entry(instr_tbl[idx++], "LSR", &cpu6502::LSR, 5, ZPG  /* 0x46 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x47 */);
    set_tbl_entry(instr_tbl[idx++], "PHA", &cpu6502::PHA, 3, NON  /* 0x48 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 2, IMM  /* 0x49 */);
    set_tbl_entry(instr_tbl[idx++], "LSR", &cpu6502::LSR, 2, ACC  /* 0x4A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 2, IMM  /* 0x4B */);
    set_tbl_entry(instr_tbl[idx++], "JMP", &cpu6502::JMP, 3, ABS  /* 0x4C */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 4, ABS  /* 0x4D */);
    set_tbl_entry(instr_tbl[idx++], "LSR", &cpu6502::LSR, 6, ABS  /* 0x4E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x4F */);
    set_tbl_entry(instr_tbl[idx++], "BVC", &cpu6502::BVC, 2, REL  /* 0x50 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 5, IDY  /* 0x51 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x52 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x53 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x54 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 4, ZPX  /* 0x55 */);
    set_tbl_entry(instr_tbl[idx++], "LSR", &cpu6502::LSR, 6, ZPX  /* 0x56 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x57 */);
    set_tbl_entry(instr_tbl[idx++], "CLI", &cpu6502::CLI, 2, NON  /* 0x58 */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 4, ABY  /* 0x59 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0x5A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x5B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x5C */);
    set_tbl_entry(instr_tbl[idx++], "EOR", &cpu6502::EOR, 4, ABX  /* 0x5D */);
    set_tbl_entry(instr_tbl[idx++], "LSR", &cpu6502::LSR, 7, ABX  /* 0x5E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x5F */);
    set_tbl_entry(instr_tbl[idx++], "RTS", &cpu6502::RTS, 6, NON  /* 0x60 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 6, IDX  /* 0x61 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x62 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x63 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x64 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 3, ZPG  /* 0x65 */);
    set_tbl_entry(instr_tbl[idx++], "ROR", &cpu6502::ROR, 5, ZPG  /* 0x66 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x67 */);
    set_tbl_entry(instr_tbl[idx++], "PLA", &cpu6502::PLA, 4, NON  /* 0x68 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 2, IMM  /* 0x69 */);
    set_tbl_entry(instr_tbl[idx++], "ROR", &cpu6502::ROR, 2, ACC  /* 0x6A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x6B */);
    set_tbl_entry(instr_tbl[idx++], "JMP", &cpu6502::JMP, 5, IND  /* 0x6C */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 4, ABS  /* 0x6D */);
    set_tbl_entry(instr_tbl[idx++], "ROR", &cpu6502::ROR, 6, ABS  /* 0x6E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x6F */);
    set_tbl_entry(instr_tbl[idx++], "BVS", &cpu6502::BVS, 2, REL  /* 0x70 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 5, IDY  /* 0x71 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x72 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x73 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x74 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 4, ZPX  /* 0x75 */);
    set_tbl_entry(instr_tbl[idx++], "ROR", &cpu6502::ROR, 6, ZPX  /* 0x76 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x77 */);
    set_tbl_entry(instr_tbl[idx++], "SEI", &cpu6502::SEI, 2, NON  /* 0x78 */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 4, ABY  /* 0x79 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0x7A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x7B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x7C */);
    set_tbl_entry(instr_tbl[idx++], "ADC", &cpu6502::ADC, 4, ABX  /* 0x7D */);
    set_tbl_entry(instr_tbl[idx++], "ROR", &cpu6502::ROR, 7, ABX  /* 0x7E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x7F */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 2, NON  /* 0x80 */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 6, IDX  /* 0x81 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x82 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x83 */);
    set_tbl_entry(instr_tbl[idx++], "STY", &cpu6502::STY, 3, ZPG  /* 0x84 */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 3, ZPG  /* 0x85 */);
    set_tbl_entry(instr_tbl[idx++], "STX", &cpu6502::STX, 3, ZPG  /* 0x86 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0x87 */);
    set_tbl_entry(instr_tbl[idx++], "DEY", &cpu6502::DEY, 2, NON  /* 0x88 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x89 */);
    set_tbl_entry(instr_tbl[idx++], "TXA", &cpu6502::TXA, 2, NON  /* 0x8A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x8B */);
    set_tbl_entry(instr_tbl[idx++], "STY", &cpu6502::STY, 4, ABS  /* 0x8C */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 4, ABS  /* 0x8D */);
    set_tbl_entry(instr_tbl[idx++], "STX", &cpu6502::STX, 4, ABS  /* 0x8E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x8F */);
    set_tbl_entry(instr_tbl[idx++], "BCC", &cpu6502::BCC, 2, REL  /* 0x90 */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 6, IDY  /* 0x91 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x92 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x93 */);
    set_tbl_entry(instr_tbl[idx++], "STY", &cpu6502::STY, 4, ZPX  /* 0x94 */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 4, ZPX  /* 0x95 */);
    set_tbl_entry(instr_tbl[idx++], "STX", &cpu6502::STX, 4, ZPY  /* 0x96 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x97 */);
    set_tbl_entry(instr_tbl[idx++], "TYA", &cpu6502::TYA, 2, NON  /* 0x98 */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 5, ABY  /* 0x99 */);
    set_tbl_entry(instr_tbl[idx++], "TXS", &cpu6502::TXS, 2, NON  /* 0x9A */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0x9B */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x9C */);
    set_tbl_entry(instr_tbl[idx++], "STA", &cpu6502::STA, 5, ABX  /* 0x9D */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x9E */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0x9F */);
    set_tbl_entry(instr_tbl[idx++], "LDY", &cpu6502::LDY, 2, IMM  /* 0xA0 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 6, IDX  /* 0xA1 */);
    set_tbl_entry(instr_tbl[idx++], "LDX", &cpu6502::LDX, 2, IMM  /* 0xA2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xA3 */);
    set_tbl_entry(instr_tbl[idx++], "LDY", &cpu6502::LDY, 3, ZPG  /* 0xA4 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 3, ZPG  /* 0xA5 */);
    set_tbl_entry(instr_tbl[idx++], "LDX", &cpu6502::LDX, 3, ZPG  /* 0xA6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xA7 */);
    set_tbl_entry(instr_tbl[idx++], "TAY", &cpu6502::TAY, 2, NON  /* 0xA8 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 2, IMM  /* 0xA9 */);
    set_tbl_entry(instr_tbl[idx++], "TAX", &cpu6502::TAX, 2, NON  /* 0xAA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xAB */);
    set_tbl_entry(instr_tbl[idx++], "LDY", &cpu6502::LDY, 4, ABS  /* 0xAC */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 4, ABS  /* 0xAD */);
    set_tbl_entry(instr_tbl[idx++], "LDX", &cpu6502::LDX, 4, ABS  /* 0xAE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xAF */);
    set_tbl_entry(instr_tbl[idx++], "BCS", &cpu6502::BCS, 2, REL  /* 0xB0 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 5, IDY  /* 0xB1 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xB2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xB3 */);
    set_tbl_entry(instr_tbl[idx++], "LDY", &cpu6502::LDY, 4, ZPX  /* 0xB4 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 4, ZPX  /* 0xB5 */);
    set_tbl_entry(instr_tbl[idx++], "LDX", &cpu6502::LDX, 4, ZPY  /* 0xB6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xB7 */);
    set_tbl_entry(instr_tbl[idx++], "CLV", &cpu6502::CLV, 2, NON  /* 0xB8 */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 4, ABY  /* 0xB9 */);
    set_tbl_entry(instr_tbl[idx++], "TSX", &cpu6502::TSX, 2, NON  /* 0xBA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xBB */);
    set_tbl_entry(instr_tbl[idx++], "LDY", &cpu6502::LDY, 4, ABX  /* 0xBC */);
    set_tbl_entry(instr_tbl[idx++], "LDA", &cpu6502::LDA, 4, ABX  /* 0xBD */);
    set_tbl_entry(instr_tbl[idx++], "LDX", &cpu6502::LDX, 4, ABY  /* 0xBE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xBF */);
    set_tbl_entry(instr_tbl[idx++], "CPY", &cpu6502::CPY, 2, IMM  /* 0xC0 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 6, IDX  /* 0xC1 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xC2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xC3 */);
    set_tbl_entry(instr_tbl[idx++], "CPY", &cpu6502::CPY, 3, ZPG  /* 0xC4 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 3, ZPG  /* 0xC5 */);
    set_tbl_entry(instr_tbl[idx++], "DEC", &cpu6502::DEC, 5, ZPG  /* 0xC6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xC7 */);
    set_tbl_entry(instr_tbl[idx++], "INY", &cpu6502::INY, 2, NON  /* 0xC8 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 2, IMM  /* 0xC9 */);
    set_tbl_entry(instr_tbl[idx++], "DEX", &cpu6502::DEX, 2, NON  /* 0xCA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xCB */);
    set_tbl_entry(instr_tbl[idx++], "CPY", &cpu6502::CPY, 4, ABS  /* 0xCC */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 4, ABS  /* 0xCD */);
    set_tbl_entry(instr_tbl[idx++], "DEC", &cpu6502::DEC, 6, ABS  /* 0xCE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xCF */);
    set_tbl_entry(instr_tbl[idx++], "BNE", &cpu6502::BNE, 2, REL  /* 0xD0 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 5, IDY  /* 0xD1 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xD2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xD3 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xD4 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 4, ZPX  /* 0xD5 */);
    set_tbl_entry(instr_tbl[idx++], "DEC", &cpu6502::DEC, 6, ZPX  /* 0xD6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xD7 */);
    set_tbl_entry(instr_tbl[idx++], "CLD", &cpu6502::CLD, 2, NON  /* 0xD8 */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 4, ABY  /* 0xD9 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0xDA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xDB */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xDC */);
    set_tbl_entry(instr_tbl[idx++], "CMP", &cpu6502::CMP, 4, ABX  /* 0xDD */);
    set_tbl_entry(instr_tbl[idx++], "DEC", &cpu6502::DEC, 7, ABX  /* 0xDE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xDF */);
    set_tbl_entry(instr_tbl[idx++], "CPX", &cpu6502::CPX, 2, IMM  /* 0xE0 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 6, IDX  /* 0xE1 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xE2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xE3 */);
    set_tbl_entry(instr_tbl[idx++], "CPX", &cpu6502::CPX, 3, ZPG  /* 0xE4 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 3, ZPG  /* 0xE5 */);
    set_tbl_entry(instr_tbl[idx++], "INC", &cpu6502::INC, 5, ZPG  /* 0xE6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xE7 */);
    set_tbl_entry(instr_tbl[idx++], "INX", &cpu6502::INX, 2, NON  /* 0xE8 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 2, IMM  /* 0xE9 */);
    set_tbl_entry(instr_tbl[idx++], "NOP", &cpu6502::NOP, 2, NON  /* 0xEA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xEB */);
    set_tbl_entry(instr_tbl[idx++], "CPX", &cpu6502::CPX, 4, ABS  /* 0xEC */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 4, ABS  /* 0xED */);
    set_tbl_entry(instr_tbl[idx++], "INC", &cpu6502::INC, 6, ABS  /* 0xEE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xEF */);
    set_tbl_entry(instr_tbl[idx++], "BEQ", &cpu6502::BEQ, 2, REL  /* 0xF0 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 5, IDY  /* 0xF1 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xF2 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xF3 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xF4 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 4, ZPX  /* 0xF5 */);
    set_tbl_entry(instr_tbl[idx++], "INC", &cpu6502::INC, 6, ZPX  /* 0xF6 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xF7 */);
    set_tbl_entry(instr_tbl[idx++], "SED", &cpu6502::SED, 2, NON  /* 0xF8 */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 4, ABY  /* 0xF9 */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, NON  /* 0xFA */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, IMM  /* 0xFB */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xFC */);
    set_tbl_entry(instr_tbl[idx++], "SBC", &cpu6502::SBC, 4, ABX  /* 0xFD */);
    set_tbl_entry(instr_tbl[idx++], "INC", &cpu6502::INC, 7, ABX  /* 0xFE */);
    set_tbl_entry(instr_tbl[idx++], "XXX", &cpu6502::NOP, 3, ABS  /* 0xFF */);
}

// -------------------------------------------------------------------------
// calc_address()
//
// Calculate and return the operand address, based on instruction addressing 
// mode. If a page is crossed (for relevant instructions), return status
//  in supplied variable (pg_crossed).
//
// NB: On entry p_regs->pc should be pointing to memory location after 
// opcode.
//
// -------------------------------------------------------------------------

uint32_t cpu6502::calc_addr(const addr_mode_e mode, wy65_reg_t* p_regs, bool &pg_crossed)
{
    uint32_t addr      = INVALID_ADDR;
    uint32_t tmp_addr;

    // Default to no page crossing
    pg_crossed         = false;

    switch(mode)
    {
    case IND:
        tmp_addr      = rd_mem(p_regs->pc);
#ifndef WY65_INDIRECT_FIX
        tmp_addr     |= rd_mem(((p_regs->pc+1) & 0xff) | (p_regs->pc & 0xff00)) << 8;
#else
        tmp_addr     |= rd_mem(p_regs->pc+1) << 8;
#endif
        addr          = rd_mem(tmp_addr);
        addr         |= rd_mem(tmp_addr+1) << 8;
        
        p_regs->pc   += 2;
        break;       
                     
    case IDX:        
        tmp_addr      = (rd_mem(p_regs->pc) + p_regs->x) & 0xff;
        addr          = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
        p_regs->pc   += 1;
        break;

    case IDY:
        tmp_addr      = rd_mem(p_regs->pc);
        tmp_addr      = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
        addr          = tmp_addr + p_regs->y;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 1;
        break;

    case ABS:
        addr          = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        p_regs->pc   += 2;
        break;

    case ABX:
        tmp_addr      = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        addr          = tmp_addr + p_regs->x;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 2;
        break;

    case ABY:
        tmp_addr      = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        addr          = tmp_addr + p_regs->y;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 2;
        break;

    case ZPG:
        addr          = rd_mem(p_regs->pc);
        p_regs->pc   += 1;
        break;

    case ZPX:
        addr          = (rd_mem(p_regs->pc) + p_regs->x) & 0xff;
        p_regs->pc   += 1;
        break;

    case ZPY:
        addr          = (rd_mem(p_regs->pc) + p_regs->y) & 0xff;
        p_regs->pc   += 1;
        break;

    case REL:
        tmp_addr      = p_regs->pc + 1;                             // Location of next instruction in cpu_memory

        addr          = tmp_addr + (int8_t)rd_mem(p_regs->pc);
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc    = tmp_addr;                                   // Default PC to next instruction
        break;

    case IMM:
        addr          = p_regs->pc;
        p_regs->pc   += 1;
        break;

    // No address required
    case ACC:
    case NON:
        break;
    }

    return addr;
}

// -------------------------------------------------------------------------
// Instruction methods. All take an op_t pointer as input (opcode,
// addressing mode and minimum execution cycles. The final execution
// count is returned on exit (with extras for page crossing, branching
// etc., as relevant, added in).
// -------------------------------------------------------------------------

int cpu6502::ADC (const op_t* p_op) 
{
    bool page_crossed;

    bool bcd          = (state.regs.flags & BCD_MASK) ? true : false;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    // Fetch the operand from memory
    uint8_t mem_val   = rd_mem(addr);

    // In BCD mode, convert accumulator and operand value back to a straight binary value
    if (bcd)
    {
        state.regs.a  = (state.regs.a  & 0xf) + (state.regs.a  >> 4) * 10;
        mem_val       = (mem_val & 0xf) + (mem_val >> 4) * 10;
    }

    // Do addition into extended result
    int8_t a          = (int8_t)state.regs.a;
    int8_t m          = (int8_t)mem_val;

    int32_t result    = a  + m + ((state.regs.flags & CARRY_MASK) ? 1 : 0);
    uint32_t res_uns  = state.regs.a  + mem_val + ((state.regs.flags & CARRY_MASK) ? 1 : 0);

    // Clear affected flags
    state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);

    // Set flags, based on extended result
    state.regs.flags |= (res_uns >= (bcd ? 100U : 0x100U))        ? CARRY_MASK : 0;
    state.regs.flags |= ((result % (bcd ? 100 : 0x100)) == 0)     ? ZERO_MASK  : 0;
    state.regs.flags |= (result < -128 || result > 127)           ? OVFLW_MASK : 0;
    state.regs.flags |= (result & 0x80)                           ? SIGN_MASK  : 0;

    // Store result in accumulator
    state.regs.a      = result %  (bcd ? 100 : 0x100);

    if (bcd)
    {
        state.regs.a  = (state.regs.a % 10) + ((state.regs.a / 10) << 4);
    }

    // Return number of cycles used
    return p_op->exec_cycles + (page_crossed ? 1 : 0);

}

int cpu6502::AND (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.a      = state.regs.a & rd_mem(addr);

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::ASL (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint32_t result   = ((p_op->mode == ACC) ? state.regs.a : rd_mem(addr)) << 1;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result % 0x100 == 0) ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x100)      ? CARRY_MASK : 0;
    state.regs.flags |= (result & 0x80)       ? SIGN_MASK  : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & 0xff;
    }
    else
    {
        wr_mem(addr, result & 0xff);
    }

    return p_op->exec_cycles;
}

int cpu6502::BCC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (!(state.regs.flags & CARRY_MASK))
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BCS (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (state.regs.flags & CARRY_MASK)
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BEQ (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (state.regs.flags & ZERO_MASK)
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BIT (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    uint8_t  mem_val  = rd_mem(addr);

    bool is_zero      = (state.regs.a & mem_val) ? false : true;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | OVFLW_MASK | SIGN_MASK);

    state.regs.flags |= is_zero          ? ZERO_MASK  : 0;
    state.regs.flags |= (mem_val & 0x40) ? OVFLW_MASK : 0;
    state.regs.flags |= (mem_val & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::BMI (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (state.regs.flags & SIGN_MASK)
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BNE (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (!(state.regs.flags & ZERO_MASK))
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BPL (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (!(state.regs.flags & SIGN_MASK))
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BRK (const op_t* p_op)
{
    state.regs.flags |= BRK_MASK;

    // Klaus Dormann's test suite is expecting the flag register's bit 5 to be set
    state.regs.flags |= 0x20;

    // BRK has a pad byte which needs acounting for when RTI executed
    state.regs.pc++;

    wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & 0xff); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.pc & 0xff); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.flags); state.regs.sp--;

    state.regs.flags |= INT_MASK;

    state.regs.pc     = (uint16_t)rd_mem(0xfffe) | ((uint16_t)rd_mem(0xffff) << 8);

    return p_op->exec_cycles;
}

int cpu6502::BVC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (!(state.regs.flags & OVFLW_MASK))
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::BVS (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    if (state.regs.flags & OVFLW_MASK)
    {
        state.regs.pc = addr;

        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        return p_op->exec_cycles;
    }
}

int cpu6502::CLC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags &= ~(CARRY_MASK);

    return p_op->exec_cycles;
}

int cpu6502::CLD (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags &= ~(BCD_MASK);

    return p_op->exec_cycles;
}

int cpu6502::CLI (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags &= ~(INT_MASK);

    return p_op->exec_cycles;
}

int cpu6502::CLV (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags &= ~(OVFLW_MASK);

    return p_op->exec_cycles;
}

int cpu6502::CMP (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr         = calc_addr(p_op->mode, &state.regs, page_crossed);

    int32_t result        = (int32_t)state.regs.a - (int32_t)rd_mem(addr);

    state.regs.flags     &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags     |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags     |= (result >= 0)   ? CARRY_MASK : 0;
    state.regs.flags     |= (result & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::CPX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    int32_t result    = (int32_t)state.regs.x - (int32_t)rd_mem(addr);

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result >= 0)   ? CARRY_MASK : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::CPY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    int32_t result    = (int32_t)state.regs.y - (int32_t)rd_mem(addr);

    state.regs.flags     &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result >= 0)   ? CARRY_MASK : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::DEC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = rd_mem(addr) - 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    wr_mem(addr, result);

    return p_op->exec_cycles;
}

int cpu6502::DEX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = state.regs.x - 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    state.regs.x      = result;

    return p_op->exec_cycles;
}

int cpu6502::DEY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = state.regs.y - 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    state.regs.y      = result;

    return p_op->exec_cycles;
}

int cpu6502::EOR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.a      = state.regs.a ^ rd_mem(addr);

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);
                     
    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::INC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = rd_mem(addr) + 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    wr_mem(addr, result);

    return p_op->exec_cycles;
}

int cpu6502::INX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = state.regs.x + 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    state.regs.x      = result;

    return p_op->exec_cycles;
}

int cpu6502::INY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t result    = state.regs.y + 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    state.regs.y      = result;

    return p_op->exec_cycles;
}

int cpu6502::JMP (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.pc     = addr;

    return p_op->exec_cycles;
}

int cpu6502::JSR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint16_t pc_m1    = state.regs.pc - 1;
    
    wr_mem(state.regs.sp | 0x100, (pc_m1 >> 8) & 0xff); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100,  pc_m1 & 0xff);       state.regs.sp--;

    state.regs.pc     = addr;

    return p_op->exec_cycles;
}

int cpu6502::LDA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.a      = rd_mem(addr);
    
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.a == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::LDX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.x      = rd_mem(addr);
    
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.x == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.x & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::LDY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.y      = rd_mem(addr);
    
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.y == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.y & 0x80) ? SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::LSR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint32_t val      = (p_op->mode == ACC) ? state.regs.a : rd_mem(addr);
    uint32_t result   = val >> 1; 

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;
    state.regs.flags |= (val    & 0x01) ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & 0xff;
    }
    else
    {
        wr_mem(addr, result & 0xff);
    }

    return p_op->exec_cycles;
}

int cpu6502::NOP (const op_t* p_op)
{
    bool page_crossed;

    // Could reach here for undocumented instructions. Opcode table has 
    // an 'address mode' for each  instruction to indicate the instructions 
    // probable byte size, so call calc_addr to skip these bytes.
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    return 0;
}

int cpu6502::ORA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.a      = state.regs.a | rd_mem(addr);

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::PHA (const op_t* p_op)
{
    wr_mem(state.regs.sp | 0x100, state.regs.a); state.regs.sp--;

    return p_op->exec_cycles;
}

int cpu6502::PHP (const op_t* p_op)
{
    wr_mem(state.regs.sp | 0x100, state.regs.flags | 0x30); state.regs.sp--;

    return p_op->exec_cycles;
}

int cpu6502::PLA (const op_t* p_op)
{
    state.regs.sp++;
    state.regs.a      = rd_mem(state.regs.sp | 0x100);

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::PLP (const op_t* p_op)
{
    state.regs.sp++;
    
    state.regs.flags  = rd_mem(state.regs.sp | 0x100);

    return p_op->exec_cycles;
}

int cpu6502::ROL (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint32_t val      = (p_op->mode == ACC) ? state.regs.a : rd_mem(addr);
    uint32_t result   = (val << 1) | ((state.regs.flags & CARRY_MASK) ? 1 : 0);

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= ((result & 0xff) == 0) ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80)        ? SIGN_MASK  : 0;
    state.regs.flags |= (result & 0x100)       ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & 0xff;
    }
    else
    {
        wr_mem(addr, result & 0xff);
    };

    return p_op->exec_cycles;
}

int cpu6502::ROR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint32_t val      = (p_op->mode == ACC) ? state.regs.a : rd_mem(addr);
    uint32_t result   = (val >> 1) | ((state.regs.flags & CARRY_MASK) ? 0x80 : 0);

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;
    state.regs.flags |= (val    & 0x01) ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & 0xff;
    }
    else
    {
        wr_mem(addr, result & 0xff);
    }

    return p_op->exec_cycles;
}

int cpu6502::RTI (const op_t* p_op)
{
    state.regs.sp++;
    state.regs.flags  = rd_mem(state.regs.sp | 0x100);
    state.regs.sp++;
    state.regs.pc     = rd_mem(state.regs.sp | 0x100);
    state.regs.sp++;
    state.regs.pc    |= rd_mem(state.regs.sp | 0x100) << 8;

    return p_op->exec_cycles;
}

int cpu6502::RTS (const op_t* p_op)
{
    state.regs.sp++;
    state.regs.pc     = rd_mem(state.regs.sp | 0x100);
    state.regs.sp++;
    state.regs.pc    |= rd_mem(state.regs.sp | 0x100) << 8;

    state.regs.pc++;

    return p_op->exec_cycles;
}

int cpu6502::SBC (const op_t* p_op)
{
    bool page_crossed;

    bool bcd = (state.regs.flags & BCD_MASK) ? true : false;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    // Fetch the operand from memory
    uint8_t mem_val   = rd_mem(addr);

    // In BCD mode, convert accumulator and operand value back to a straight binary value
    if (bcd)
    {
        state.regs.a  = (state.regs.a  & 0xf) + (state.regs.a  >> 4) * 10;
        mem_val       = (mem_val & 0xf) + (mem_val >> 4) * 10;
    }

    // Do addition into extended result
    int32_t a         = (int8_t)state.regs.a;
    int32_t m         = (int8_t)mem_val;

    uint32_t result   = a - m - ((state.regs.flags & CARRY_MASK) ? 0 : 1);
    uint32_t res_uns  = state.regs.a - mem_val - ((state.regs.flags & CARRY_MASK) ? 0 : 1);

    // Clear affected flags
    state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);

    // Set flags, based on extended result
    state.regs.flags |= !(res_uns >= (bcd ? 100U : 0x100U))       ?  CARRY_MASK : 0;
    state.regs.flags |= ((result % (bcd ? 100 : 0x100)) == 0)     ?  ZERO_MASK  : 0;
    state.regs.flags |= (((result>>1) ^ res_uns) & 0x80) && !bcd  ?  OVFLW_MASK : 0;
    state.regs.flags |= (result & 0x80)                           ?  SIGN_MASK  : 0;

    if (bcd)
    {
        state.regs.a  = (((int32_t)result < 0) ? (result + 100) : result) % 100;
        state.regs.a  = (state.regs.a % 10) + ((state.regs.a / 10) << 4);
    }
    else
    {
        // Store result in accumulator
        state.regs.a  = result & 0xff;
    }

    // Return number of cycles used
    return p_op->exec_cycles + (page_crossed ? 1 : 0);;
}

int cpu6502::SEC (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags |= CARRY_MASK;

    return p_op->exec_cycles;
}

int cpu6502::SED (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags |= BCD_MASK;

    return p_op->exec_cycles;
}

int cpu6502::SEI (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    state.regs.flags |= INT_MASK;

    return p_op->exec_cycles;
}

int cpu6502::STA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    wr_mem(addr, state.regs.a);

    return p_op->exec_cycles;
}

int cpu6502::STX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    wr_mem(addr, state.regs.x);

    return p_op->exec_cycles;
}

int cpu6502::STY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    wr_mem(addr, state.regs.y);

    return p_op->exec_cycles;
}

int cpu6502::TAX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.x      = state.regs.a;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.x == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.x & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::TAY (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.y      = state.regs.a;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.y == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.y & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;;
}

int cpu6502::TSX (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.x      = state.regs.sp;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.x == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.x & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::TXA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.a      = state.regs.x;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);
                     
    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

int cpu6502::TXS (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.sp     = state.regs.x;

    return p_op->exec_cycles;
}

int cpu6502::TYA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.a      = state.regs.y;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (state.regs.a == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.a & 0x80) ?  SIGN_MASK  : 0;

    return p_op->exec_cycles;
}

// -------------------------------------------------------------------------
// disassemble()
//
// Disassemble an opcode to a named log file. The pc input must point to the 
// location *after* the opcode. The default output looks something like:
// 
//   0222   29 F0       AND   #$F0 
//   0224   85 0D       STA   $0D  
//   0226   20 59 02    JSR   $0259
//               *
//   0259   F8          SED        
//   025a   C0 01       CPY   #$01 
//
// The discontinuity mark ('*') can be disabled, and display of register 
// values can be enabled. An example of the output would then look like:
// 
//   0222   29 F0       AND   #$F0      a=08 x=00 y=01 sp=ff flags=05 (sp)=02
//   0224   85 0D       STA   $0D       a=00 x=00 y=01 sp=ff flags=07 (sp)=02
//   0226   20 59 02    JSR   $0259     a=00 x=00 y=01 sp=ff flags=07 (sp)=02
//   0259   F8          SED             a=00 x=00 y=01 sp=fd flags=07 (sp)=02
//   025a   C0 01       CPY   #$01      a=00 x=00 y=01 sp=fd flags=0f (sp)=02
// 
// Note that, as the register values are supplied as arguments, their values
// can be that of before or after the instruction is executed, depending
// from where disassemble() is called. When called from execute(), it would
// display the state *before* the opcode is executed.
//
// -------------------------------------------------------------------------

void cpu6502::disassemble (const int      opcode, 
                           const uint16_t pc, 
                           const uint64_t cycles, 
                           const bool     disable_jmp_mrk, 
                           const bool     enable_regs_disp, 
                           const uint8_t  a, 
                           const uint8_t  x, 
                           const uint8_t  y, 
                           const uint8_t  sp, 
                           const uint8_t  flags,
                           const char*    fname)
{

    if (fp == NULL)
    {
        fp = fopen(fname, "wb");
        fprintf(fp, "CPU6502 Disassembler output\n\n");
    }

    uint8_t pc0       = rd_mem(pc);
    uint8_t pc1       = rd_mem(pc+1);
    uint8_t pc2       = rd_mem(pc+2);

    if (!disable_jmp_mrk && nextPc != INVALID_NEXT_PC && nextPc != pc)
    {
        fprintf(fp, "            *\n");
    }


    const char* str   = instr_tbl[opcode].op_str;

#ifdef WY65_EN_PRINT_CYCLES
    fprintf(fp, "%8d : %04x   ", cycles, pc);
#else
    fprintf(fp, "%04x   ", pc);
#endif

    switch (instr_tbl[opcode].addr_mode)
    {
    case NON: fprintf(fp, "%02X          %s          ", pc0, str);
        nextPc        = pc + 1;
        break;

    case ACC: 
        fprintf(fp, "%02X          %s   A      ", pc0, str); 
        nextPc        = pc + 1;
        break;

    case IMM: 
        fprintf(fp, "%02X %02X       %s   #$%02X   ", pc0, pc1, str, pc1); 
        nextPc        = pc + 2;
        break;

    case ABS: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X  ", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ZPG:
        fprintf(fp, "%02X %02X       %s   $%02X    ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case REL: 
        fprintf(fp, "%02X %02X       %s   $%02X    ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case ABX: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X,X", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ABY: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X,Y", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ZPX: 
        fprintf(fp, "%02X %02X       %s   $%02X,X  ",   pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case ZPY: 
        fprintf(fp, "%02X %02X       %s   $%02X,Y  ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IDX: 
        fprintf(fp, "%02X %02X       %s   ($%02X,X)", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IDY: 
        fprintf(fp, "%02X %02X       %s   ($%02X),Y", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IND: 
        fprintf(fp, "%02X %02X %02X    %s   ($%04X)", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;
    }

    if (enable_regs_disp)
    {
        fprintf(fp, "   a=%02x x=%02x y=%02x sp=%02x flags=%02x (sp)=%02x", a, x, y, sp, flags, rd_mem(0x100 | sp));
    }

    fprintf(fp, "\n");
    fflush(fp);

}

// -------------------------------------------------------------------------
// irq()
//
// Checks for, and execution of, maskable interrupts. IRQs are level 
// sensitive (active low). The nirq_line state has 16 lines bitmapped,
// and if any are low, and interrupts are enabled (I flag is low),
// The PC and state are pushed on the stack, the I flag set, and the
// PC set to the interrupt vector defined at address 0xfffe.
//
// -------------------------------------------------------------------------

void cpu6502::irq ()
{
    if (state.nirq_line != NO_ACTIVE_IRQS && !(state.regs.flags & INT_MASK))
    {
        wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & 0xff);  state.regs.sp--;
        wr_mem(state.regs.sp | 0x100, state.regs.pc & 0xff);         state.regs.sp--;
        wr_mem(state.regs.sp | 0x100, state.regs.flags & ~BRK_MASK); state.regs.sp--;
        
        state.regs.flags |= INT_MASK;
        
        state.regs.pc     = (uint16_t)rd_mem(0xfffe) | ((uint16_t)rd_mem(0xffff) << 8);

        state.cycles      += IRQ_CYCLES;
    }
}

// -------------------------------------------------------------------------
// execute()
//
// Executes a single instruction. Internally checks for outstanding IRQ 
// first before proceeding to execute the opcoden. Optional control of 
// disassemling instruction if icount is between start_count and stop_count.
// Jump marks (marked spaces between PC discontinuities) can be enabled/
// disabled.
//
// Returns the PC after the instruction, and the cycles taked to execute. 
//
// -------------------------------------------------------------------------

wy65_exec_status_t cpu6502::execute (const uint32_t icount, const uint32_t start_count, const uint32_t stop_count, const bool en_jmp_mrks)
{
    op_t          op;
    wy65_exec_status_t rtn_val;
    int                num_cycles;

    // Check for maskable interrupts
    irq();

    op.opcode         = rd_mem(state.regs.pc++);
    curr_instr        = instr_tbl[op.opcode];

    if (icount >= start_count && icount < stop_count)
    {
        // In BeebEm testing, disassemble in the execute() function
        disassemble(op.opcode, 
                    state.regs.pc-1, 
                    state.cycles, 
                    en_jmp_mrks, 
                    true, 
                    state.regs.a, 
                    state.regs.x, 
                    state.regs.y, 
                    state.regs.sp, 
                    state.regs.flags);
    }

    op.mode           = curr_instr.addr_mode;
    op.exec_cycles    = curr_instr.exec_cycles;

    // Execute instruction and get number of cycles (which may be more than op.exec_cycles; e.g. page crossing)
    num_cycles = (this->*curr_instr.pFunc)(&op);

    state.cycles     += num_cycles;

    rtn_val.cycles    = num_cycles;
    rtn_val.pc        = state.regs.pc;

    return rtn_val;
}

// -------------------------------------------------------------------------
// nmi_interrupt()
//
// Generates an NMI. NMI is falling edge triggered, and calling this 
// function emulates this single event. The flags an PC are pushed on to the
// stack, the interrupt mask set (I flag), and the PC set to the vector
// defined at address 0xfffa
//
// -------------------------------------------------------------------------

void cpu6502::nmi_interrupt ()
{
    
    wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & 0xff); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.pc & 0xff);        state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.flags);            state.regs.sp--;

    state.regs.flags |= INT_MASK;

    state.regs.pc     = (uint16_t)rd_mem(0xfffa) | ((uint16_t)rd_mem(0xfffb) << 8);

    state.cycles     += NMI_CYCLES;
}

// -------------------------------------------------------------------------
// activate_irq()
//
// Activates (sets low) one of the sixteen IRQ lines, as defined by id. This
// has a default value of 0.
//
// -------------------------------------------------------------------------

void cpu6502::activate_irq (const uint16_t id)
{
    // Activate the IRQ line if 'id' in range, else ignore.
    if (id < NUM_INTERNAL_IRQS)
    {
        state.nirq_line  &= ~(1 << id);
    }
}

// -------------------------------------------------------------------------
// deactivate_irq()
//
// Dectivates (sets low) one of the sixteen IRQ lines, as defined by id. 
// This has a default value of 0.
//
// -------------------------------------------------------------------------

void cpu6502::deactivate_irq (const uint16_t id)
{
    // Deactivate the IRQ line if 'id' in range, else ignore.
    if (id < NUM_INTERNAL_IRQS)
    {
        state.nirq_line  |= 1 << id;
    }
}

// -------------------------------------------------------------------------
// reset()
//
// Emulates a hard reset. All the 6502 registers are reset, the PC is
// updated with the vector at 0xfffc and interrupts are disabled. In
// addition, the internal cycle count is cleared.
// 
// -------------------------------------------------------------------------

void cpu6502::reset ()
{
    // Reset the CPU state.
    state.regs.flags  = INT_MASK;
    state.regs.pc     = (uint16_t)rd_mem(0xfffc) | ((uint16_t)rd_mem(0xfffd) << 8);
    state.regs.a      = 0;
    state.regs.x      = 0;
    state.regs.y      = 0;
    state.regs.sp     = 0xff;

    state.cycles      = RST_CYCLES;
    state.nirq_line   = NO_ACTIVE_IRQS;
}

// -------------------------------------------------------------------------
// get_state()
//
// Return a copy of the internal state of the model.
//
// -------------------------------------------------------------------------

wy65_cpu_state_t cpu6502::get_state ()
{
    return state;
}

// -------------------------------------------------------------------------
// register_mem_funcs()
//
// Register external memory functions for use in memory read/write accesses,
// to allow interfacing with external memory system. By default, internal
// memory is accessed. If this function is used to set two external
// functions all memory access will use these instead. The external
// functions must be of type:
//
//   void <ExtWrFuncName> (int addr, unsigned char data)
//   int  <ExtRdFuncName> (int addr)
//
// If the external application functions are not quite these types, then 
// wrapper functions will need constructing. The types defined here were
// chosen based on BeebEm's memory access functions. (see website at
// http://www.mkw.me.uk/beebem).
//
// It is possible to override only one access method (not sure why anyone
// would want to) by setting one or the other arguments to NULL.
//
// -------------------------------------------------------------------------

void cpu6502::register_mem_funcs (wy65_p_writemem_t p_wfunc, wy65_p_readmem_t p_rfunc)
{
    ext_wr_mem        = p_wfunc;
    ext_rd_mem        = p_rfunc;
}

#ifdef WY65_STANDALONE

// -------------------------------------------------------------------------
// STATIC VARIABLES FOR STANDALONE 
// -------------------------------------------------------------------------

// The cpu6502 model
static cpu6502  cpu;

// -------------------------------------------------------------------------
// wr_mem()
//
// Local non-class memory write function to pass into IHX routines
// (can't get a true pointer to a class member function).
//
// -------------------------------------------------------------------------
/*
static void wr_mem(int addr, unsigned char data)
{
    cpu.wr_mem(addr, data);
}
*/

// -------------------------------------------------------------------------
// main()
//
// Top level function with command line interface for loading programs,
// enabling/disabling disassembly and rudimentary debug breaking. In
// particular, it is used to execute a modified test suite from 
// Klaus Dorman:
//
//   https://github.com/Klaus2m5/6502_65C02_functional_tests
//
// The test is modified to write a good/bad status on completion to
// 0xfff8/9 and this is inspected to print a message.
//
// -------------------------------------------------------------------------

int main (int argc, char** argv)
{

    bool     read_bin         = true;
    bool     read_srecord     = false;
    uint16_t load_addr        = DEFAULT_LOAD_ADDR;
    uint16_t start_addr       = DEFAULT_START_ADDR;
    uint16_t debug_start_addr = DEFAULT_DEBUG_ADDR;
    uint32_t icount_brk       = DEFAULT_DEBUG_ICOUNT;
    uint32_t start_dis_count  = DEFAULT_START_DIS_CNT;
    uint32_t stop_dis_count   = DEFAULT_STOP_DIS_CNT;

    char*    fname            = DEFAULT_PROG_FILE_NAME;

    FILE*    prog_fp          = NULL;
    int      option;

    // Process command line options
    while ((option = getopt(argc, argv, "f:I:M:l:s:d:i:S:E:h")) != EOF)
    {
        switch(option)
        {
        case 'f':
            fname             = optarg;
            break;
        case 'I':
            fname             = optarg;
            read_bin          = false;
            break;
        case 'M':
            fname             = optarg;
            read_bin          = false;
            read_srecord      = true;
            break;
        case 'l':
            load_addr         = (uint16_t)strtol(optarg, NULL, 0);
            break;
        case 's':
            start_addr        = (uint16_t)strtol(optarg, NULL, 0);
            break;
        case 'd':
            debug_start_addr  = (uint16_t)strtol(optarg, NULL, 0);
            break;
        case 'i':
            icount_brk        = strtol(optarg, NULL, 0);
            break;
        case 'S':
            start_dis_count   = strtol(optarg, NULL, 0);
            break;
        case 'E':
            stop_dis_count    = strtol(optarg, NULL, 0);
            break;
        case 'h':
        case 'q':
            fprintf(stderr, "Usage: %s [[-f | -I | -M] <filename>][-l <addr>>][-s <addr>]\n"
                "        [-d <addr>][-i <count>][-S <count>][-E <count>]\n\n"
                "    -f Binary program file name            (default %s)\n"
                "    -I Intel Hex program file name\n"
                "    -M Motorola S-Record program file name\n"
                "    -l Load start address of binary image  (default 0x%04x)\n"
                "    -s Start address of program execution  (default 0x%04x)\n"
                "    -d Debug break address                 (default 0x%04x)\n"
                "    -i Debug instruction count             (default 0x%04x)\n"
                "    -S Disassamble start instruction count (default 0x%08x)\n"
                "    -E Disassamble end instruction count   (default 0x%08x)\n"
                "\n"
                          , argv[0]
                          , DEFAULT_PROG_FILE_NAME
                          , DEFAULT_LOAD_ADDR
                          , DEFAULT_START_ADDR
                          , DEFAULT_DEBUG_ADDR
                          , DEFAULT_DEBUG_ICOUNT
                          , DEFAULT_START_DIS_CNT
                          , DEFAULT_STOP_DIS_CNT
                          );
            return option == 'h' ? GOOD_RTN_STATUS : BAD_OPTION;
            break;
        }
    }

    if (read_bin)
    {
        if (cpu.read_bin(fname, load_addr) != BIN_NO_ERROR)
        {
            return BAD_FILE_OPEN;
        }
    }
    else if (read_srecord)
    {
        if (cpu.read_srec (fname) != SREC_NO_ERROR)
        {
            return BAD_FILE_OPEN;
        }
    }
    else
    {
        if (cpu.read_ihx (fname) != IHX_NO_ERROR)
        {
            return BAD_FILE_OPEN;
        }
    }

    // Load the reset vector with the user start location, if specified
    cpu.wr_mem(0xfffc,  start_addr       & 0xff);
    cpu.wr_mem(0xfffd, (start_addr >> 8) & 0xff);


    // Load failure status to nominated location
    cpu.wr_mem(0xfff8,  BAD_TEST_STATUS       & 0xff);
    cpu.wr_mem(0xfff9, (BAD_TEST_STATUS >> 8) & 0xff);

    // Assert a reset
    cpu.reset();

    wy65_exec_status_t status;
    bool     terminate        = false;
    uint32_t instr_count      = 0;
    uint16_t prev_pc          = 0;

    fprintf(stdout, "Executing %s from address 0x%04x ...\n\n", fname, start_addr); 

    // Start executing instructions
    do 
    {
        // This is here purely to serve as debug breakpoint for 
        // a given address after a certain number of executed instructions.
        if (prev_pc == debug_start_addr && instr_count > icount_brk)
        {
            prev_pc           = prev_pc; // Does nothing
        }

        status = cpu.execute(instr_count++, start_dis_count, stop_dis_count);

        // Terminate if looping back to same instruction (i.e. deliberately hung)
        terminate = (status.pc == prev_pc);

        prev_pc               = status.pc;

    }
    while (!terminate);

    // Get test status values from memory
    uint16_t test_status = (cpu.rd_mem(0xfff9) << 8) | cpu.rd_mem(0xfff8);

    if (test_status == GOOD_TEST_STATUS)
    {
        fprintf(stdout, "********\n");
        fprintf(stdout, "* PASS *\n");
        fprintf(stdout, "********\n\n");

        fprintf(stdout, "Executed %d instructions\n\n", instr_count);
    }
    else
    {
        fprintf(stderr, "********\n");
        fprintf(stderr, "* FAIL *\n");
        fprintf(stderr, "********\n\n");

        fprintf(stderr, "Terminated at PC = 0x%04x after %d instructions\n\n", status.pc, instr_count);
    }

    return GOOD_RTN_STATUS;
}

#endif

