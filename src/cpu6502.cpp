//=============================================================
// 
// Copyright (c) 2016-2017 Simon Southwell. All rights reserved.
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
// $Id: cpu6502.cpp,v 1.18 2017/03/03 11:15:00 simon Exp $
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

static double tv_diff;
#if !(defined _WIN32) && !(defined _WIN64)
#include <sys/time.h>
static struct timeval tv_start, tv_stop;

#else
#include <Windows.h>
LARGE_INTEGER freq, start, stop;
#endif

// -------------------------------------------------------------------------
// Terminal control utility functions for enabling/diabling input echoing
// -------------------------------------------------------------------------

static void pre_run_setup()
{
#if !(defined _WIN32) && !(defined _WIN64)
    // Log time just before running (LINUX only)
    (void)gettimeofday(&tv_start, NULL);
#else
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
#endif 
}

static void post_run_setup()
{
#if !(defined _WIN32) && !(defined _WIN64)
    // Get time just after running, and calculate run time (LINUX only)
    (void)gettimeofday(&tv_stop, NULL);
    tv_diff = ((float)(tv_stop.tv_sec - tv_start.tv_sec)*1e6) + ((float)(tv_stop.tv_usec - tv_start.tv_usec));
#else
    QueryPerformanceCounter(&stop);
    tv_diff = (double)(stop.QuadPart - start.QuadPart)*1e6/(double)freq.QuadPart;
#endif 
}

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
    state.waiting     = false;
    state.stopped     = false;
    state.mode_c      = BASE;

    // Initialise instruction table
    int idx           = 0;

    set_tbl_entry(instr_tbl[idx++], "BRK",  &cpu6502::BRK, 7, NON, BASE /* 0x00 */);
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 6, IDX, BASE /* 0x01 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0x02 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x03 */);
    set_tbl_entry(instr_tbl[idx++], "TSB",  &cpu6502::TSB, 5, ZPG, C02  /* 0x04 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 3, ZPG, BASE /* 0x05 */);
    set_tbl_entry(instr_tbl[idx++], "ASL",  &cpu6502::ASL, 5, ZPG, BASE /* 0x06 */);
    set_tbl_entry(instr_tbl[idx++], "RMB0", &cpu6502::RMB, 5, ZPG, WRK  /* 0x07 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "PHP",  &cpu6502::PHP, 3, NON, BASE /* 0x08 */);
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 2, IMM, BASE /* 0x09 */);
    set_tbl_entry(instr_tbl[idx++], "ASL",  &cpu6502::ASL, 2, ACC, BASE /* 0x0A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x0B */);
    set_tbl_entry(instr_tbl[idx++], "TSB",  &cpu6502::TSB, 6, ABS, C02  /* 0x0C */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 4, ABS, BASE /* 0x0D */);
    set_tbl_entry(instr_tbl[idx++], "ASL",  &cpu6502::ASL, 6, ABS, BASE /* 0x0E */);
    set_tbl_entry(instr_tbl[idx++], "BBR0", &cpu6502::BBR, 5, ZPR, WRK  /* 0x0F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BPL",  &cpu6502::BPL, 2, REL, BASE /* 0x10 */);
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 5, IDY, BASE /* 0x11 */);
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 5, IDZ, C02  /* 0x12 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x13 */);
    set_tbl_entry(instr_tbl[idx++], "TRB",  &cpu6502::TRB, 5, ZPG, C02  /* 0x14 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 4, ZPX, BASE /* 0x15 */);
    set_tbl_entry(instr_tbl[idx++], "ASL",  &cpu6502::ASL, 6, ZPX, BASE /* 0x16 */);
    set_tbl_entry(instr_tbl[idx++], "RMB1", &cpu6502::RMB, 5, ZPG, WRK  /* 0x17 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CLC",  &cpu6502::CLC, 2, NON, BASE /* 0x18 */);
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 4, ABY, BASE /* 0x19 */);
    set_tbl_entry(instr_tbl[idx++], "INC",  &cpu6502::INC, 2, ACC, C02  /* 0x1A */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x1B */);
    set_tbl_entry(instr_tbl[idx++], "TRB",  &cpu6502::TRB, 6, ABS, C02  /* 0x1C */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ORA",  &cpu6502::ORA, 4, ABX, BASE /* 0x1D */);
    set_tbl_entry(instr_tbl[idx++], "ASL",  &cpu6502::ASL, 7, ABX, BASE /* 0x1E */);
    set_tbl_entry(instr_tbl[idx++], "BBR1", &cpu6502::BBR, 5, ZPR, WRK  /* 0x1F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "JSR",  &cpu6502::JSR, 6, ABS, BASE /* 0x20 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 6, IDX, BASE /* 0x21 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0x22 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x23 */);
    set_tbl_entry(instr_tbl[idx++], "BIT",  &cpu6502::BIT, 3, ZPG, BASE /* 0x24 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 3, ZPG, BASE /* 0x25 */);
    set_tbl_entry(instr_tbl[idx++], "ROL",  &cpu6502::ROL, 5, ZPG, BASE /* 0x26 */);
    set_tbl_entry(instr_tbl[idx++], "RMB2", &cpu6502::RMB, 5, ZPG, WRK  /* 0x27 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "PLP",  &cpu6502::PLP, 4, NON, BASE /* 0x28 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 2, IMM, BASE /* 0x29 */);
    set_tbl_entry(instr_tbl[idx++], "ROL",  &cpu6502::ROL, 2, ACC, BASE /* 0x2A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x2B */);
    set_tbl_entry(instr_tbl[idx++], "BIT",  &cpu6502::BIT, 4, ABS, BASE /* 0x2C */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 4, ABS, BASE /* 0x2D */);
    set_tbl_entry(instr_tbl[idx++], "ROL",  &cpu6502::ROL, 6, ABS, BASE /* 0x2E */);
    set_tbl_entry(instr_tbl[idx++], "BBR2", &cpu6502::BBR, 5, ZPR, WRK  /* 0x2F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BMI",  &cpu6502::BMI, 2, REL, BASE /* 0x30 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 5, IDY, BASE /* 0x31 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 5, IDZ, C02  /* 0x32 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x33 */);
    set_tbl_entry(instr_tbl[idx++], "BIT",  &cpu6502::BIT, 4, ZPX, C02  /* 0x34 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 4, ZPX, BASE /* 0x35 */);
    set_tbl_entry(instr_tbl[idx++], "ROL",  &cpu6502::ROL, 6, ZPX, BASE /* 0x36 */);
    set_tbl_entry(instr_tbl[idx++], "RMB3", &cpu6502::RMB, 5, ZPG, WRK  /* 0x37 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "SEC",  &cpu6502::SEC, 2, NON, BASE /* 0x38 */);
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 4, ABY, BASE /* 0x39 */);
    set_tbl_entry(instr_tbl[idx++], "DEC",  &cpu6502::DEC, 2, ACC, C02  /* 0x3A */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x3B */);
    set_tbl_entry(instr_tbl[idx++], "BIT",  &cpu6502::BIT, 4, ABX, C02  /* 0x3C */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "AND",  &cpu6502::AND, 4, ABX, BASE /* 0x3D */);
    set_tbl_entry(instr_tbl[idx++], "ROL",  &cpu6502::ROL, 7, ABX, BASE /* 0x3E */);
    set_tbl_entry(instr_tbl[idx++], "BBR3", &cpu6502::BBR, 5, ZPR, WRK  /* 0x3F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "RTI",  &cpu6502::RTI, 6, NON, BASE /* 0x40 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 6, IDX, BASE /* 0x41 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0x42 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x43 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 3, ZPG, C02  /* 0x44 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 3, ZPG, BASE /* 0x45 */);
    set_tbl_entry(instr_tbl[idx++], "LSR",  &cpu6502::LSR, 5, ZPG, BASE /* 0x46 */);
    set_tbl_entry(instr_tbl[idx++], "RMB4", &cpu6502::RMB, 5, ZPG, WRK  /* 0x47 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "PHA",  &cpu6502::PHA, 3, NON, BASE /* 0x48 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 2, IMM, BASE /* 0x49 */);
    set_tbl_entry(instr_tbl[idx++], "LSR",  &cpu6502::LSR, 2, ACC, BASE /* 0x4A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, N4B, C02  /* 0x4B */);
    set_tbl_entry(instr_tbl[idx++], "JMP",  &cpu6502::JMP, 3, ABS, BASE /* 0x4C */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 4, ABS, BASE /* 0x4D */);
    set_tbl_entry(instr_tbl[idx++], "LSR",  &cpu6502::LSR, 6, ABS, BASE /* 0x4E */);
    set_tbl_entry(instr_tbl[idx++], "BBR4", &cpu6502::BBR, 5, ZPR, WRK  /* 0x4F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BVC",  &cpu6502::BVC, 2, REL, BASE /* 0x50 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 5, IDY, BASE /* 0x51 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 5, IDZ, BASE /* 0x52 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x53 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 4, ZPX, C02  /* 0x54 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 4, ZPX, BASE /* 0x55 */);
    set_tbl_entry(instr_tbl[idx++], "LSR",  &cpu6502::LSR, 6, ZPX, BASE /* 0x56 */);
    set_tbl_entry(instr_tbl[idx++], "RMB5", &cpu6502::RMB, 5, ZPG, WRK  /* 0x57 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CLI",  &cpu6502::CLI, 2, NON, BASE /* 0x58 */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 4, ABY, BASE /* 0x59 */);
    set_tbl_entry(instr_tbl[idx++], "PHY",  &cpu6502::PHY, 3, NON, C02  /* 0x5A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x5B */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 8, ABX, C02  /* 0x5C */);
    set_tbl_entry(instr_tbl[idx++], "EOR",  &cpu6502::EOR, 4, ABX, BASE /* 0x5D */);
    set_tbl_entry(instr_tbl[idx++], "LSR",  &cpu6502::LSR, 7, ABX, BASE /* 0x5E */);
    set_tbl_entry(instr_tbl[idx++], "BBR5", &cpu6502::BBR, 5, ZPR, WRK  /* 0x5F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "RTS",  &cpu6502::RTS, 6, NON, BASE /* 0x60 */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 6, IDX, BASE /* 0x61 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0x62 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x63 */);
    set_tbl_entry(instr_tbl[idx++], "STZ",  &cpu6502::STZ, 3, ZPG, C02  /* 0x64 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 3, ZPG, BASE /* 0x65 */);
    set_tbl_entry(instr_tbl[idx++], "ROR",  &cpu6502::ROR, 5, ZPG, BASE /* 0x66 */);
    set_tbl_entry(instr_tbl[idx++], "RMB6", &cpu6502::RMB, 5, ZPG, WRK  /* 0x67 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "PLA",  &cpu6502::PLA, 4, NON, BASE /* 0x68 */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 2, IMM, BASE /* 0x69 */);
    set_tbl_entry(instr_tbl[idx++], "ROR",  &cpu6502::ROR, 2, ACC, BASE /* 0x6A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x6B */);
    set_tbl_entry(instr_tbl[idx++], "JMP",  &cpu6502::JMP, 5, IND, BASE /* 0x6C */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 4, ABS, BASE /* 0x6D */);
    set_tbl_entry(instr_tbl[idx++], "ROR",  &cpu6502::ROR, 6, ABS, BASE /* 0x6E */);
    set_tbl_entry(instr_tbl[idx++], "BBR6", &cpu6502::BBR, 5, ZPR, WRK  /* 0x6F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BVS",  &cpu6502::BVS, 2, REL, BASE /* 0x70 */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 5, IDY, BASE /* 0x71 */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 5, IDZ, C02  /* 0x72 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x73 */);
    set_tbl_entry(instr_tbl[idx++], "STZ",  &cpu6502::STZ, 4, ZPX, C02  /* 0x74 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 4, ZPX, BASE /* 0x75 */);
    set_tbl_entry(instr_tbl[idx++], "ROR",  &cpu6502::ROR, 6, ZPX, BASE /* 0x76 */);
    set_tbl_entry(instr_tbl[idx++], "RMB7", &cpu6502::RMB, 5, ZPG, WRK  /* 0x77 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "SEI",  &cpu6502::SEI, 2, NON, BASE /* 0x78 */);
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 4, ABY, BASE /* 0x79 */);
    set_tbl_entry(instr_tbl[idx++], "PLY",  &cpu6502::PLY, 4, NON, C02  /* 0x7A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x7B */);
    set_tbl_entry(instr_tbl[idx++], "JMP",  &cpu6502::JMP, 6, IAX, C02  /* 0x7C */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "ADC",  &cpu6502::ADC, 4, ABX, BASE /* 0x7D */);
    set_tbl_entry(instr_tbl[idx++], "ROR",  &cpu6502::ROR, 7, ABX, BASE /* 0x7E */);
    set_tbl_entry(instr_tbl[idx++], "BBR7", &cpu6502::BBR, 5, ZPR, WRK  /* 0x7F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BRA",  &cpu6502::BRA, 3, REL, C02  /* 0x80 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 6, IDX, BASE /* 0x81 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0x82 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x83 */);
    set_tbl_entry(instr_tbl[idx++], "STY",  &cpu6502::STY, 3, ZPG, BASE /* 0x84 */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 3, ZPG, BASE /* 0x85 */);
    set_tbl_entry(instr_tbl[idx++], "STX",  &cpu6502::STX, 3, ZPG, BASE /* 0x86 */);
    set_tbl_entry(instr_tbl[idx++], "SMB0", &cpu6502::SMB, 5, ZPG, WRK  /* 0x87 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "DEY",  &cpu6502::DEY, 2, NON, BASE /* 0x88 */);
    set_tbl_entry(instr_tbl[idx++], "BIT",  &cpu6502::BIT, 2, IMM, C02  /* 0x89 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "TXA",  &cpu6502::TXA, 2, NON, BASE /* 0x8A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x8B */);
    set_tbl_entry(instr_tbl[idx++], "STY",  &cpu6502::STY, 4, ABS, BASE /* 0x8C */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 4, ABS, BASE /* 0x8D */);
    set_tbl_entry(instr_tbl[idx++], "STX",  &cpu6502::STX, 4, ABS, BASE /* 0x8E */);
    set_tbl_entry(instr_tbl[idx++], "BBS0", &cpu6502::BBS, 5, ZPR, WRK  /* 0x8F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BCC",  &cpu6502::BCC, 2, REL, BASE /* 0x90 */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 6, IDY, BASE /* 0x91 */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 5, IDZ, C02  /* 0x92 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x93 */);
    set_tbl_entry(instr_tbl[idx++], "STY",  &cpu6502::STY, 4, ZPX, BASE /* 0x94 */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 4, ZPX, BASE /* 0x95 */);
    set_tbl_entry(instr_tbl[idx++], "STX",  &cpu6502::STX, 4, ZPY, BASE /* 0x96 */);
    set_tbl_entry(instr_tbl[idx++], "SMB1", &cpu6502::SMB, 5, ZPG, WRK  /* 0x97 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "TYA",  &cpu6502::TYA, 2, NON, BASE /* 0x98 */);
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 5, ABY, BASE /* 0x99 */);
    set_tbl_entry(instr_tbl[idx++], "TXS",  &cpu6502::TXS, 2, NON, BASE /* 0x9A */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0x9B */);
    set_tbl_entry(instr_tbl[idx++], "STZ",  &cpu6502::STZ, 4, ABS, C02  /* 0x9C */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "STA",  &cpu6502::STA, 5, ABX, BASE /* 0x9D */);
    set_tbl_entry(instr_tbl[idx++], "STZ",  &cpu6502::STZ, 5, ABX, C02  /* 0x9E */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "BBS1", &cpu6502::BBS, 5, ZPR, WRK  /* 0x9F */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "LDY",  &cpu6502::LDY, 2, IMM, BASE /* 0xA0 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 6, IDX, BASE /* 0xA1 */);
    set_tbl_entry(instr_tbl[idx++], "LDX",  &cpu6502::LDX, 2, IMM, BASE /* 0xA2 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xA3 */);
    set_tbl_entry(instr_tbl[idx++], "LDY",  &cpu6502::LDY, 3, ZPG, BASE /* 0xA4 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 3, ZPG, BASE /* 0xA5 */);
    set_tbl_entry(instr_tbl[idx++], "LDX",  &cpu6502::LDX, 3, ZPG, BASE /* 0xA6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB2", &cpu6502::SMB, 5, ZPG, WRK  /* 0xA7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "TAY",  &cpu6502::TAY, 2, NON, BASE /* 0xA8 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 2, IMM, BASE /* 0xA9 */);
    set_tbl_entry(instr_tbl[idx++], "TAX",  &cpu6502::TAX, 2, NON, BASE /* 0xAA */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xAB */);
    set_tbl_entry(instr_tbl[idx++], "LDY",  &cpu6502::LDY, 4, ABS, BASE /* 0xAC */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 4, ABS, BASE /* 0xAD */);
    set_tbl_entry(instr_tbl[idx++], "LDX",  &cpu6502::LDX, 4, ABS, BASE /* 0xAE */);
    set_tbl_entry(instr_tbl[idx++], "BBS2", &cpu6502::BBS, 5, ZPR, WRK  /* 0xAF */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BCS",  &cpu6502::BCS, 2, REL, BASE /* 0xB0 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 5, IDY, BASE /* 0xB1 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 5, IDZ, C02  /* 0xB2 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xB3 */);
    set_tbl_entry(instr_tbl[idx++], "LDY",  &cpu6502::LDY, 4, ZPX, BASE /* 0xB4 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 4, ZPX, BASE /* 0xB5 */);
    set_tbl_entry(instr_tbl[idx++], "LDX",  &cpu6502::LDX, 4, ZPY, BASE /* 0xB6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB3", &cpu6502::SMB, 5, ZPG, WRK  /* 0xB7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CLV",  &cpu6502::CLV, 2, NON, BASE /* 0xB8 */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 4, ABY, BASE /* 0xB9 */);
    set_tbl_entry(instr_tbl[idx++], "TSX",  &cpu6502::TSX, 2, NON, BASE /* 0xBA */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xBB */);
    set_tbl_entry(instr_tbl[idx++], "LDY",  &cpu6502::LDY, 4, ABX, BASE /* 0xBC */);
    set_tbl_entry(instr_tbl[idx++], "LDA",  &cpu6502::LDA, 4, ABX, BASE /* 0xBD */);
    set_tbl_entry(instr_tbl[idx++], "LDX",  &cpu6502::LDX, 4, ABY, BASE /* 0xBE */);
    set_tbl_entry(instr_tbl[idx++], "BBS3", &cpu6502::BBS, 5, ZPR, WRK  /* 0xBF */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CPY",  &cpu6502::CPY, 2, IMM, BASE /* 0xC0 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 6, IDX, BASE /* 0xC1 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0xC2 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xC3 */);
    set_tbl_entry(instr_tbl[idx++], "CPY",  &cpu6502::CPY, 3, ZPG, BASE /* 0xC4 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 3, ZPG, BASE /* 0xC5 */);
    set_tbl_entry(instr_tbl[idx++], "DEC",  &cpu6502::DEC, 5, ZPG, BASE /* 0xC6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB4", &cpu6502::SMB, 5, ZPG, WRK  /* 0xC7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "INY",  &cpu6502::INY, 2, NON, BASE /* 0xC8 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 2, IMM, BASE /* 0xC9 */);
    set_tbl_entry(instr_tbl[idx++], "DEX",  &cpu6502::DEX, 2, NON, BASE /* 0xCA */);
    set_tbl_entry(instr_tbl[idx++], "WAI",  &cpu6502::WAI, 3, NON, WDC  /* 0xCB */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CPY",  &cpu6502::CPY, 4, ABS, BASE /* 0xCC */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 4, ABS, BASE /* 0xCD */);
    set_tbl_entry(instr_tbl[idx++], "DEC",  &cpu6502::DEC, 6, ABS, BASE /* 0xCE */);
    set_tbl_entry(instr_tbl[idx++], "BBS4", &cpu6502::BBS, 5, ZPR, WRK  /* 0xCF */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BNE",  &cpu6502::BNE, 2, REL, BASE /* 0xD0 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 5, IDY, BASE /* 0xD1 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 5, IDZ, C02  /* 0xD2 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xD3 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 4, ZPX, C02  /* 0xD4 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 4, ZPX, BASE /* 0xD5 */);
    set_tbl_entry(instr_tbl[idx++], "DEC",  &cpu6502::DEC, 6, ZPX, BASE /* 0xD6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB5", &cpu6502::SMB, 5, ZPG, WRK  /* 0xD7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CLD",  &cpu6502::CLD, 2, NON, BASE /* 0xD8 */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 4, ABY, BASE /* 0xD9 */);
    set_tbl_entry(instr_tbl[idx++], "PHX",  &cpu6502::PHX, 3, NON, C02  /* 0xDA */);
    set_tbl_entry(instr_tbl[idx++], "STP",  &cpu6502::STP, 3, NON, WDC  /* 0xDB */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 4, ABS, C02  /* 0xDC */);
    set_tbl_entry(instr_tbl[idx++], "CMP",  &cpu6502::CMP, 4, ABX, BASE /* 0xDD */);
    set_tbl_entry(instr_tbl[idx++], "DEC",  &cpu6502::DEC, 7, ABX, BASE /* 0xDE */);
    set_tbl_entry(instr_tbl[idx++], "BBS5", &cpu6502::BBS, 5, ZPR, WRK  /* 0xDF */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "CPX",  &cpu6502::CPX, 2, IMM, BASE /* 0xE0 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 6, IDX, BASE /* 0xE1 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, IMM, C02  /* 0xE2 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xE3 */);
    set_tbl_entry(instr_tbl[idx++], "CPX",  &cpu6502::CPX, 3, ZPG, BASE /* 0xE4 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 3, ZPG, BASE /* 0xE5 */);
    set_tbl_entry(instr_tbl[idx++], "INC",  &cpu6502::INC, 5, ZPG, BASE /* 0xE6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB6", &cpu6502::SMB, 5, ZPG, WRK  /* 0xE7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "INX",  &cpu6502::INX, 2, NON, BASE /* 0xE8 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 2, IMM, BASE /* 0xE9 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 2, NON, BASE /* 0xEA */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xEB */);
    set_tbl_entry(instr_tbl[idx++], "CPX",  &cpu6502::CPX, 4, ABS, BASE /* 0xEC */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 4, ABS, BASE /* 0xED */);
    set_tbl_entry(instr_tbl[idx++], "INC",  &cpu6502::INC, 6, ABS, BASE /* 0xEE */);
    set_tbl_entry(instr_tbl[idx++], "BBS6", &cpu6502::BBS, 5, ZPR, WRK  /* 0xEF */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "BEQ",  &cpu6502::BEQ, 2, REL, BASE /* 0xF0 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 5, IDY, BASE /* 0xF1 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 5, IDZ, C02  /* 0xF2 */); // 65C02
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xF3 */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 4, ZPX, C02  /* 0xF4 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 4, ZPX, BASE /* 0xF5 */);
    set_tbl_entry(instr_tbl[idx++], "INC",  &cpu6502::INC, 6, ZPX, BASE /* 0xF6 */);
    set_tbl_entry(instr_tbl[idx++], "SMB7", &cpu6502::SMB, 5, ZPG, WRK  /* 0xF7 */); // WDC65C02
    set_tbl_entry(instr_tbl[idx++], "SED",  &cpu6502::SED, 2, NON, BASE /* 0xF8 */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 4, ABY, BASE /* 0xF9 */);
    set_tbl_entry(instr_tbl[idx++], "PLX",  &cpu6502::PLX, 4, NON, C02  /* 0xFA */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 1, NON, C02  /* 0xFB */);
    set_tbl_entry(instr_tbl[idx++], "NOP",  &cpu6502::NOP, 4, ABS, C02  /* 0xFC */);
    set_tbl_entry(instr_tbl[idx++], "SBC",  &cpu6502::SBC, 4, ABX, BASE /* 0xFD */);
    set_tbl_entry(instr_tbl[idx++], "INC",  &cpu6502::INC, 7, ABX, BASE /* 0xFE */);
    set_tbl_entry(instr_tbl[idx++], "BBS7", &cpu6502::BBS, 5, ZPR, WRK  /* 0xFF */); // WDC65C02
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
        tmp_addr     |= rd_mem(((p_regs->pc & MASK_8BIT) == MASK_8BIT && state.mode_c == BASE) ? (p_regs->pc & 0xff00) :  p_regs->pc+1) << 8;
        addr          = rd_mem(tmp_addr);
        addr         |= rd_mem(tmp_addr+1) << 8;
        
        p_regs->pc   += 2;
        break;       
                     
    case IDX:        
        tmp_addr      = (rd_mem(p_regs->pc) + p_regs->x) & MASK_8BIT;
        addr          = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
        p_regs->pc   += 1;
        break;

    case IDY:
        tmp_addr      = rd_mem(p_regs->pc);
        tmp_addr      = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
        addr          = (tmp_addr + p_regs->y) & MASK_16BIT;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 1;
        break;

    case ABS:
        addr          = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        p_regs->pc   += 2;
        break;

    case ABX:
        tmp_addr      = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        addr          = (tmp_addr + p_regs->x) & MASK_16BIT;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 2;
        break;

    case ABY:
        tmp_addr      = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8);
        addr          = (tmp_addr + p_regs->y) & MASK_16BIT;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc   += 2;
        break;

    case ZPG:
        addr          = rd_mem(p_regs->pc) & MASK_8BIT;
        p_regs->pc   += 1;
        break;

    case ZPX:
        addr          = (rd_mem(p_regs->pc) + p_regs->x) & MASK_8BIT;
        p_regs->pc   += 1;
        break;

    case ZPY:
        addr          = (rd_mem(p_regs->pc) + p_regs->y) & MASK_8BIT;
        p_regs->pc   += 1;
        break;

    case REL:
        tmp_addr      = p_regs->pc + 1;                             // Location of next instruction in cpu_memory

        addr          = (tmp_addr + (int8_t)rd_mem(p_regs->pc)) & MASK_16BIT;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        p_regs->pc    = tmp_addr;                                   // Default PC to next instruction
        break;

    case IMM:
        addr          = p_regs->pc;
        p_regs->pc   += 1;
        break;

    // 65C02 only
    case IAX:
        // Get 16 bit operand and add X 
        tmp_addr      = rd_mem(p_regs->pc) | (rd_mem(p_regs->pc+1) << 8); 
        tmp_addr      = (tmp_addr + p_regs->x) & MASK_16BIT;

        // Address is value located at above address location
        addr          = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
        p_regs->pc   += 2;
        break;

    // WDC 65C02 only. Unlike relative addressing PC remains pointing 
    // at byte after opcode (for Zero page address), and BBS/BBR instructions
    // must advance PC by 2 when branch not taken.
    case ZPR:
        tmp_addr      = p_regs->pc + 2;                             // Location of next instruction in cpu_memory

        addr          = (tmp_addr + (int8_t)rd_mem(p_regs->pc+1)) & MASK_16BIT;
        pg_crossed    = ((addr ^ tmp_addr) >> 8) ? true : false;
        break;

    case IDZ:
        tmp_addr      = rd_mem(p_regs->pc);
        addr          = rd_mem(tmp_addr) | (rd_mem(tmp_addr+1) << 8);
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
    bool     page_crossed;
    int32_t  result;

    bool bcd          = (state.regs.flags & BCD_MASK) ? true : false;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    // Fetch the operand from memory
    uint8_t mem_val   = rd_mem(addr);
    uint8_t acc       = state.regs.a;

    if (bcd)
    {
        uint16_t tmp;
        uint16_t lo_nib;
        uint16_t hi_nib;
        bool     bcd_carry;
        
        // Do low digit BCD addition and flag if result has carried ( > 9)
        tmp               = (acc & MASK_LO_NIB) + (mem_val & MASK_LO_NIB) + (state.regs.flags & CARRY_MASK);
        bcd_carry         = tmp > 9;

        // Get low digit result, adjusting if carried
        lo_nib            = bcd_carry ? (tmp + 0x06) & MASK_LO_NIB : tmp;
                
        // Do high digit BCD addition, adding in carry from low digit as applicable, and flag if carried
        tmp               = (acc & MASK_HI_NIB) + (mem_val & MASK_HI_NIB) + (bcd_carry ? 0x10 : 0);
        bcd_carry         = (tmp > 0x90);

        // Get high digit result, adjusting if carried
        hi_nib            = bcd_carry ? (tmp + 0x60) & MASK_HI_NIB : tmp;
                     
        // Combine two digits into accumulator
        state.regs.a      = (hi_nib | lo_nib) & MASK_8BIT;

        // Clear affected flags
        state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);

        state.regs.flags |= (state.regs.a == 0)                             ? ZERO_MASK  : 0;
        state.regs.flags |= ((tmp & SIGN_MASK)==0) ^ ((acc & SIGN_MASK)==0) ? OVFLW_MASK : 0;
        state.regs.flags |= bcd_carry                                       ? CARRY_MASK : 0;
        state.regs.flags |= state.regs.a & SIGN_MASK;
    }
    else
    {
        // Do addition into extended result
        int8_t a          = (int8_t)state.regs.a;
        int8_t m          = (int8_t)mem_val;
        
        result            = a  + m + ((state.regs.flags & CARRY_MASK) ? 1 : 0);
        uint32_t res_uns  = state.regs.a  + mem_val + ((state.regs.flags & CARRY_MASK) ? 1 : 0);
        
        // Clear affected flags
        state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);
        
        // Set flags, based on extended result
        state.regs.flags |= (res_uns >=  0x100U)            ? CARRY_MASK : 0;
        state.regs.flags |= ((result & MASK_8BIT) == 0)     ? ZERO_MASK  : 0;
        state.regs.flags |= (result < -128 || result > 127) ? OVFLW_MASK : 0;
        state.regs.flags |= (result & SIGN_MASK)            ? SIGN_MASK  : 0;
        
        // Store result in accumulator
        state.regs.a      = result & MASK_8BIT;
    }

    // Return number of cycles used
    return p_op->exec_cycles + (page_crossed ? 1 : 0) + (bcd ? 1 : 0);
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
    state.regs.flags |= state.regs.a & SIGN_MASK;

    return p_op->exec_cycles + (page_crossed ? 1 : 0);
}

int cpu6502::ASL (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint8_t val      = ((p_op->mode == ACC) ? (uint32_t)state.regs.a : (uint32_t)rd_mem(addr));
    uint8_t result   = val << 1;

    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= !(result & MASK_8BIT) ? ZERO_MASK  : 0;
    state.regs.flags |= (val & 0x80)          ? CARRY_MASK : 0;
    state.regs.flags |= result & SIGN_MASK;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result;
    }
    else
    {
        wr_mem(addr, result);
    }

    // For 65C02 1 cycle quicker for ABX and no page crossing
    return p_op->exec_cycles - ((!page_crossed && p_op->mode == ABX) ? 1 : 0);  
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
    state.regs.flags &= ~ZERO_MASK;

    state.regs.flags |= is_zero ? ZERO_MASK  : 0;

    // O and N bits not altered for immediate mode
    if (p_op->mode != IMM)
    {
      state.regs.flags &= ~(OVFLW_MASK | SIGN_MASK);

      state.regs.flags |= mem_val & OVFLW_MASK;
      state.regs.flags |= mem_val & SIGN_MASK;
    }

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

    wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & MASK_8BIT); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.pc & MASK_8BIT); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.flags); state.regs.sp--;

    state.regs.flags |= INT_MASK;

    if (state.mode_c > BASE)
    {
        state.regs.flags &= ~BCD_MASK;
    }

    state.regs.pc     = (uint16_t)rd_mem(IRQ_VEC_ADDR) | ((uint16_t)rd_mem(IRQ_VEC_ADDR+1) << 8);

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

    // After clearing the I flag, check for interrupt
    irq();

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

    uint8_t result    = ((p_op->mode == ACC) ? state.regs.a : rd_mem(addr)) - 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result;
    }
    else
    {
        wr_mem(addr, result);
    }

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

    uint8_t result    = ((p_op->mode == ACC) ? state.regs.a : rd_mem(addr)) + 1;

    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & MASK_8BIT;
    }
    else
    {
        wr_mem(addr, result & MASK_8BIT);
    }

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
    
    wr_mem(state.regs.sp | 0x100, (pc_m1 >> 8) & MASK_8BIT); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100,  pc_m1 & MASK_8BIT);       state.regs.sp--;

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

    uint8_t val      = (p_op->mode == ACC) ? state.regs.a : rd_mem(addr);
    uint8_t result   = (val >> 1) & 0x7f; 

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;
    state.regs.flags |= (val    & 0x01) ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result;
    }
    else
    {
        wr_mem(addr, result);
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

    // I flags status updated, so check for interrupts
    irq();

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

    state.regs.flags |= ((result & MASK_8BIT) == 0) ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80)             ? SIGN_MASK  : 0;
    state.regs.flags |= (val    & 0x80)             ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & MASK_8BIT;
    }
    else
    {
        wr_mem(addr, result & MASK_8BIT);
    };

    return p_op->exec_cycles;
}

int cpu6502::ROR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);

    uint32_t val      = (p_op->mode == ACC) ? state.regs.a : rd_mem(addr);
    uint32_t result   = ((val >> 1) & 0x7f) | ((state.regs.flags & CARRY_MASK) ? 0x80 : 0);

    state.regs.flags &= ~(ZERO_MASK | CARRY_MASK | SIGN_MASK);

    state.regs.flags |= (result == 0)   ? ZERO_MASK  : 0;
    state.regs.flags |= (result & 0x80) ? SIGN_MASK  : 0;
    state.regs.flags |= (val    & 0x01) ? CARRY_MASK : 0;

    if (p_op->mode == ACC)
    {
        state.regs.a  = result & MASK_8BIT;
    }
    else
    {
        wr_mem(addr, result & MASK_8BIT);
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
    uint8_t acc       = state.regs.a;

    if (bcd)
    {
        int16_t tmp;
        int16_t lo_nib;
        int16_t hi_nib;
        bool    bcd_borrow;
        
        // Do low digit BCD addition and flag if result has borrowed ( < 0)
        tmp = (acc & MASK_LO_NIB) - (mem_val & MASK_LO_NIB) - ((state.regs.flags & CARRY_MASK) ? 0 : 1);
        bcd_borrow = tmp < 0;

        // Get low digit result, adjusting if borrowed
        lo_nib = bcd_borrow ? (tmp - 6) & MASK_LO_NIB : tmp;

        // Do high digit BCD addition, factoring in borrow from low digit as applicable, and flag if borrowed
        tmp = (acc & MASK_HI_NIB) - (mem_val & MASK_HI_NIB) - (bcd_borrow ? 0x10 : 0);
        bcd_borrow = tmp < 0;

        // Get high digit result, adjusting if borrowed
        hi_nib = bcd_borrow ? (tmp - 0x60) & MASK_HI_NIB : tmp;

        // Combine two digits into accumulator
        state.regs.a  = hi_nib | lo_nib;

        // Clear affected flags
        state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);

        state.regs.flags |= (state.regs.a == 0)                                 ? ZERO_MASK  : 0;
        state.regs.flags |= ((tmp & SIGN_MASK) == 0) ^ ((acc & SIGN_MASK) == 0) ? OVFLW_MASK : 0;
        state.regs.flags |= !bcd_borrow                                         ? CARRY_MASK : 0;
        state.regs.flags |= state.regs.a & SIGN_MASK;
    }
    else
    {
        // Do addition into extended result
        int32_t a         = (int8_t)state.regs.a;
        int32_t m         = (int8_t)mem_val;
        
        uint32_t result   = a - m - ((state.regs.flags & CARRY_MASK) ? 0 : 1);
        uint32_t res_uns  = state.regs.a - mem_val - ((state.regs.flags & CARRY_MASK) ? 0 : 1);
        
        // Clear affected flags
        state.regs.flags &= ~(CARRY_MASK | ZERO_MASK | OVFLW_MASK | SIGN_MASK);
        
        // Set flags, based on extended result
        state.regs.flags |= !(res_uns >=  0x100U)             ?  CARRY_MASK : 0;
        state.regs.flags |= ((result & MASK_8BIT) == 0)       ?  ZERO_MASK  : 0;
        state.regs.flags |= (((result>>1) ^ res_uns) & 0x80)  ?  OVFLW_MASK : 0;
        state.regs.flags |= (result & 0x80)                   ?  SIGN_MASK  : 0;
        
        // Store result in accumulator
        state.regs.a  = result & MASK_8BIT;
    }

    // Return number of cycles used
    return p_op->exec_cycles + (page_crossed ? 1 : 0) + (bcd ? 1 : 0);
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

    return p_op->exec_cycles;
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
// Instruction for the 65C02/WDC 65C02

int cpu6502::BBR (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of branch
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    // Fetch byte at indicated zero page address (PC is pointing to it)
    uint8_t zp_byte = rd_mem(rd_mem(state.regs.pc));
    
    if (!(zp_byte & (1 << ((p_op->opcode >> 4) & 0x7))))
    {
        state.regs.pc = addr;
    
        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        // Advance PC to next instruction
        state.regs.pc += 2;
        return p_op->exec_cycles;
    }

}

int cpu6502::BBS (const op_t* p_op)
{
    bool page_crossed;
  
    // Fetch address of branch
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    // Fetch byte at indicated zero page address (PC is pointing to it)
    uint8_t zp_byte = rd_mem(rd_mem(state.regs.pc));
    
    if (zp_byte & (1 << ((p_op->opcode >> 4) & 0x7)))
    {
        state.regs.pc = addr;
    
        return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
    }
    else
    {
        // Advance PC to next instruction
        state.regs.pc += 2;
        return p_op->exec_cycles;
    }
}

int cpu6502::RMB (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of branch
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    uint8_t zp_byte = rd_mem(addr) & ~(1 << ((p_op->opcode >> 4) & 0x7));
    
    wr_mem(addr, zp_byte);
    
    return p_op->exec_cycles;
}

int cpu6502::SMB (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of branch
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    uint8_t zp_byte = rd_mem(addr) | (1 << ((p_op->opcode >> 4) & 0x7));
    
    wr_mem(addr, zp_byte);
    
    return p_op->exec_cycles;
}

int cpu6502::BRA (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    
    state.regs.pc = addr;
    
    return p_op->exec_cycles + 1 + (page_crossed ? 1 : 0);
}

int cpu6502::TRB (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    uint8_t  mem_val  = rd_mem(addr);
    
    wr_mem(addr, mem_val & ~(state.regs.a));
    
    bool is_zero      = (state.regs.a & mem_val) ? false : true;
    
    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK);
    
    state.regs.flags |= is_zero ? ZERO_MASK  : 0;
    
    return p_op->exec_cycles;
}

int cpu6502::TSB (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
    uint8_t  mem_val  = rd_mem(addr);
    
    wr_mem(addr, mem_val | state.regs.a);
    
    bool is_zero      = (state.regs.a & mem_val) ? false : true;
    
    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK);
    
    state.regs.flags |= is_zero ? ZERO_MASK  : 0;
    
    return p_op->exec_cycles;
}

int cpu6502::STZ (const op_t* p_op)
{
    bool page_crossed;

    // Fetch address of data (and update PC)
    uint32_t addr     = calc_addr(p_op->mode, &state.regs, page_crossed);
        
    wr_mem(addr, 0x00);
        
    return p_op->exec_cycles;
}

int cpu6502::PHX (const op_t* p_op)
{

    wr_mem(state.regs.sp | 0x100, state.regs.x); state.regs.sp--;
        
    return p_op->exec_cycles;
}

int cpu6502::PHY (const op_t* p_op)
{
    wr_mem(state.regs.sp | 0x100, state.regs.y); state.regs.sp--;

    return p_op->exec_cycles;
}

int cpu6502::PLX (const op_t* p_op)
{
    state.regs.sp++;
    state.regs.x      = rd_mem(state.regs.sp | 0x100);
    
    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);
    
    state.regs.flags |= (state.regs.x == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.x & 0x80) ?  SIGN_MASK  : 0;
    
    return p_op->exec_cycles;
}

int cpu6502::PLY (const op_t* p_op)
{
    state.regs.sp++;
    state.regs.y      = rd_mem(state.regs.sp | 0x100);
    
    // Clear affected flags
    state.regs.flags &= ~(ZERO_MASK | SIGN_MASK);
    
    state.regs.flags |= (state.regs.y == 0)   ?  ZERO_MASK  : 0;
    state.regs.flags |= (state.regs.y & 0x80) ?  SIGN_MASK  : 0;
    
    return p_op->exec_cycles;
}

int cpu6502::WAI (const op_t* p_op)
{
    // Cycles returned only on first execution of wait
    uint32_t cycles = state.waiting ? 0 : p_op->exec_cycles;
    
    // If here, and an active interrupt, then I bit set, and we simply continue
    if (state.nirq_line != NO_ACTIVE_IRQS)
    {
        state.waiting = false;
    }
    else
    {
        // If no interrupt, then go back to WAI address
        state.regs.pc--;
        state.waiting = true;
    }
    
    return cycles;
}

int cpu6502::STP (const op_t* p_op)
{
    uint32_t cycles = state.stopped ? 0 : p_op->exec_cycles;

    // When stopped, go back to STP instruction
    state.regs.pc--;
    state.stopped = true;

    return cycles;
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

// LCOV_EXCL_START

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

    // The opcode string is forced to be "???" when the cpu type for the opcode
    // is later than the current revision mode.
    const char* str   = (instr_tbl[opcode].cpu_type <= state.mode_c) ? instr_tbl[opcode].op_str : "???";

#ifdef WY65_EN_PRINT_CYCLES
    fprintf(fp, "%8d : %04x   ", cycles, pc);
#else
    fprintf(fp, "%04x   ", pc);
#endif

    switch (instr_tbl[opcode].addr_mode)
    {
    case NON: fprintf(fp, "%02X          %s            ", pc0, str);
        nextPc        = pc + 1;
        break;

    case ACC: 
        fprintf(fp, "%02X          %s   A        ", pc0, str); 
        nextPc        = pc + 1;
        break;

    case IMM: 
        fprintf(fp, "%02X %02X       %s   #$%02X     ", pc0, pc1, str, pc1); 
        nextPc        = pc + 2;
        break;

    case ABS: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X    ", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ZPG:
        fprintf(fp, "%02X %02X       %s%s  $%02X      ", pc0, pc1, str, ((opcode & 0xf) == 7) ? "" : " ", pc1);
        nextPc        = pc + 2;
        break;

    case REL: 
        fprintf(fp, "%02X %02X       %s   $%02X      ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case ABX: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X,X  ", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ABY: 
        fprintf(fp, "%02X %02X %02X    %s   $%04X,Y  ", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case ZPX: 
        fprintf(fp, "%02X %02X       %s   $%02X,X    ",   pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case ZPY: 
        fprintf(fp, "%02X %02X       %s   $%02X,Y    ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IDX: 
        fprintf(fp, "%02X %02X       %s   ($%02X,X)  ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IDY: 
        fprintf(fp, "%02X %02X       %s   ($%02X),Y  ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;

    case IND: 
        fprintf(fp, "%02X %02X %02X    %s   ($%04X)", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    // WDC6502 addressing mode for BBRn/BBSn instructions
    case ZPR:
        fprintf(fp, "%02X %02X %02X    %s  $%02x,$%02X", pc0, pc1, pc2, str, pc1, pc2);
        nextPc        = pc + 3;
        break;

    case IAX:
        fprintf(fp, "%02X %02X %02X    %s   ($%04X,X)", pc0, pc1, pc2, str, pc1 | pc2 << 8);
        nextPc        = pc + 3;
        break;

    case IDZ:
        fprintf(fp, "%02X %02X       %s   ($%02X)    ", pc0, pc1, str, pc1);
        nextPc        = pc + 2;
        break;
    }

    if (enable_regs_disp)
    {
        fprintf(fp, "   a=%02x x=%02x y=%02x sp=%02x flags=%02x (sp)=%02x", a, x, y, sp, flags, rd_mem(0x100 | sp));
    }

    fprintf(fp, "\n");
    fflush(fp);

}

// LCOV_EXCL_STOP

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
    op_t               op;
    wy65_exec_status_t rtn_val;
    int                num_cycles;

    op.opcode         = rd_mem(state.regs.pc++);

    tbl_t curr_instr  = instr_tbl[op.opcode];

    // If the opcode points to an instruction that is greater than the selected 6502 type
    // redirect pFunc to the NOP function.
    if (curr_instr.cpu_type > state.mode_c)
    {
        curr_instr.pFunc = instr_tbl[NOP_OPCODE_BASE].pFunc; // LCOV_EXCL_LINE --- testing with all instructions enabled
    }

    if (icount >= start_count && icount < stop_count)
    {
        // In BeebEm testing, disassemble in the execute() function
        disassemble(op.opcode, 
                    state.regs.pc-1, 
                    state.cycles, 
                    !en_jmp_mrks, 
                    true, 
                    state.regs.a, 
                    state.regs.x, 
                    state.regs.y, 
                    state.regs.sp, 
                    state.regs.flags);
    }

    op.exec_cycles    = curr_instr.exec_cycles;
    op.mode           = curr_instr.addr_mode; 

    // Execute instruction and get number of cycles (which may be more than op.exec_cycles; e.g. page crossing)
    num_cycles        = (this->*curr_instr.pFunc)(&op);

    state.cycles     += num_cycles;

    rtn_val.cycles    = num_cycles;
    rtn_val.pc        = state.regs.pc;
    rtn_val.flags     = state.regs.flags;

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
    // If waiting, PC was not advanced past the WAI opcode, so do this before pushing PC
    if (state.waiting)
    {
        state.regs.pc++;
        state.waiting = false;
    }

    wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & MASK_8BIT); state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.pc & MASK_8BIT);        state.regs.sp--;
    wr_mem(state.regs.sp | 0x100, state.regs.flags);                 state.regs.sp--;

    state.regs.flags |= INT_MASK;

    state.regs.pc     = (uint16_t)rd_mem(NMI_VEC_ADDR) | ((uint16_t)rd_mem(NMI_VEC_ADDR+1) << 8);

    state.cycles     += NMI_CYCLES;
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
    if (state.nirq_line != NO_ACTIVE_IRQS && !(state.regs.flags & INT_MASK) && !state.stopped)
    {
        // If waiting, PC was not advanced past the WAI opcode, so do this before pushing PC
        if (state.waiting)
        {
            state.regs.pc++;
            state.waiting = false;
        }

        wr_mem(state.regs.sp | 0x100, (state.regs.pc >> 8) & MASK_8BIT);  state.regs.sp--;
        wr_mem(state.regs.sp | 0x100, state.regs.pc & MASK_8BIT);         state.regs.sp--;
        wr_mem(state.regs.sp | 0x100, state.regs.flags & ~BRK_MASK);      state.regs.sp--;
        
        state.regs.flags |= INT_MASK;
        
        state.regs.pc     = (uint16_t)rd_mem(IRQ_VEC_ADDR) | ((uint16_t)rd_mem(IRQ_VEC_ADDR+1) << 8);

        state.cycles      += IRQ_CYCLES;
    }
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

    // Check for interrupt at each assertion of an IRQ line
    irq();
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

void cpu6502::reset (cpu_type_e mode)
{
    // Reset the CPU state.
    state.regs.flags  = INT_MASK;
    state.regs.pc     = (uint16_t)rd_mem(RESET_VEC_ADDR) | ((uint16_t)rd_mem(RESET_VEC_ADDR+1) << 8);
    state.regs.a      = 0;
    state.regs.x      = 0;
    state.regs.y      = 0;
    state.regs.sp     = 0xff;

    state.cycles      = RST_CYCLES;
    state.nirq_line   = NO_ACTIVE_IRQS;
    state.waiting     = false;
    state.stopped     = false;

    if (mode != DEFAULT)
    {
        state.mode_c      = mode; // Set which CPU variant mode we're in from argument (default BASE)
    }
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

// LCOV_EXCL_START
void cpu6502::register_mem_funcs (wy65_p_writemem_t p_wfunc, wy65_p_readmem_t p_rfunc)
{
    ext_wr_mem        = p_wfunc;
    ext_rd_mem        = p_rfunc;
}
// LCOV_EXCL_STOP

#ifdef WY65_STANDALONE

// -------------------------------------------------------------------------
// STATIC VARIABLES FOR STANDALONE 
// -------------------------------------------------------------------------

// The cpu6502 model
static cpu6502  cpu;

// -------------------------------------------------------------------------
// interrupt_test()
//
// Performs testing of IRQ and NMI interrupts
//
// -------------------------------------------------------------------------

bool interrupt_test(cpu_type_e mode_c, uint16_t start_addr)
{
    bool     error        = false;
    uint16_t old_irq_vec;

    // ------------------------------------
    // NMI Interrupt test
    // ------------------------------------

    // Reset
    cpu.reset(mode_c);

    // Execute an instruction
    wy65_exec_status_t status = cpu.execute();

    // Check returned PC is within three of the reset vector
    if (status.pc < start_addr || status.pc >= (start_addr+3))
    {
        error = true; // LCOV_EXCL_LINE
    }
    // Make sure that the flags have the interrupt bit set
    else if (!(status.flags & INT_MASK))
    {
        error = true; // LCOV_EXCL_LINE
    }

    if (!error)
    {
        uint16_t nmi_addr = TEST_ADDR1;

        // Remember the IRQ vector already programmed in memory
        old_irq_vec  = cpu.rd_mem(IRQ_VEC_ADDR);
        old_irq_vec |= cpu.rd_mem(IRQ_VEC_ADDR+1) << 8;

        // Load the NMI vector with the user start location, if specified
        cpu.wr_mem(NMI_VEC_ADDR,    nmi_addr       & MASK_8BIT);
        cpu.wr_mem(NMI_VEC_ADDR+1, (nmi_addr >> 8) & MASK_8BIT);

        // Load NMI address with a NOP
        cpu.wr_mem(nmi_addr,    NOP_OPCODE_BASE); // Load a NOP opcode
        cpu.wr_mem(nmi_addr+1,  CLI_OPCODE);      // Load a CLI opcode

        // Raise an interrupt
        cpu.nmi_interrupt();

        // Execute an instruction
        status = cpu.execute();

        // Check returned PC is address after nmi_addr
        if (status.pc != (nmi_addr+1))
        {
            error = true; // LCOV_EXCL_LINE
        }
        // Check that the flags has the interrupt bit set, but not the break flag
        else if (((status.flags & INT_MASK) == 0) || (status.flags & BRK_MASK))
        {
            error = true; // LCOV_EXCL_LINE
        }
        else
        {
            // Execute next instruction (CLI)
            status = cpu.execute();

            // Check interrupt flag clear, ready for IRQ testing
            if (status.flags & INT_MASK)
            {
                error = true; // LCOV_EXCL_LINE
            }
        }
    }

    // ------------------------------------
    // IRQ Interrupt test
    // ------------------------------------

    if (!error)
    {
        uint16_t irq_addr = TEST_ADDR2;

        // Load the IRQ vector with the user start location, if specified
        cpu.wr_mem(IRQ_VEC_ADDR,    irq_addr       & MASK_8BIT);
        cpu.wr_mem(IRQ_VEC_ADDR+1, (irq_addr >> 8) & MASK_8BIT);

        // Load IRQ address with a NOP
        cpu.wr_mem(irq_addr,    NOP_OPCODE_BASE);

        // Activate an IRQ
        cpu.activate_irq();

        // Execute an instruction
        status = cpu.execute();

        // Remove active IRQ
        cpu.deactivate_irq();

        // Check PC is irq_addr + 1
        if (status.pc != (irq_addr+1))
        {
            error = true; // LCOV_EXCL_LINE
        }
        // Check interrupt flag set
        else if (!(status.flags & INT_MASK))
        {
            error = true; // LCOV_EXCL_LINE
        }

        // Restore IRQ vector (for BRK instruction testing in main test)
        cpu.wr_mem(IRQ_VEC_ADDR,    old_irq_vec       & MASK_8BIT);
        cpu.wr_mem(IRQ_VEC_ADDR+1, (old_irq_vec >> 8) & MASK_8BIT);
    }

    return error;
}

// -------------------------------------------------------------------------
// wait_stop_tests()
//
// Performs testing of WAI and STP instructions
//
// -------------------------------------------------------------------------

bool wait_stop_tests(uint16_t start_addr)
{

    wy65_exec_status_t status;
    uint16_t           irq_vec;
    uint16_t           nmi_vec;
    bool               error        = false;

    // ------------------------------------
    // WAI test 
    // ------------------------------------

    // Get the IRQ vector already programmed in memory
    irq_vec  = cpu.rd_mem(IRQ_VEC_ADDR);
    irq_vec |= cpu.rd_mem(IRQ_VEC_ADDR+1) << 8;

	// Update reset vector
    uint16_t rst_addr = TEST_ADDR1;
    cpu.wr_mem(RESET_VEC_ADDR,    rst_addr       & MASK_8BIT);
    cpu.wr_mem(RESET_VEC_ADDR+1, (rst_addr >> 8) & MASK_8BIT);

    // Program at rst_addr: WAI CLI WAI NOP
    cpu.wr_mem(rst_addr,    WAI_OPCODE);
    cpu.wr_mem(rst_addr+1,  CLI_OPCODE);
    cpu.wr_mem(rst_addr+2,  WAI_OPCODE);
    cpu.wr_mem(rst_addr+3,  NOP_OPCODE_BASE);

    cpu.reset();

	// Execute WAI (I is set)
    status = cpu.execute();

	// Check pc = rst_addr, and cycles != 0 --- i.e. now waiting (for the first time)
	if (status.pc != rst_addr || status.cycles == 0)
	{
        error = true; // LCOV_EXCL_LINE
	}
	else
	{
	    // Execute WAI (I is set)
        status = cpu.execute();
    
	    // Check pc = rst_addr, and cycles == 0 --- i.e. still waiting (not for the first time)
	    if (status.pc != rst_addr || status.cycles != 0)
	    {
            error = true; // LCOV_EXCL_LINE
	    }
	}

    // Test IRQ when waiting with I bit set
    if (!error)
    {
        // Activate an interrupt (but I still set)
        cpu.activate_irq();

        // Execute WAI for a third time
        status = cpu.execute();

        // Deactivate IRQ
        cpu.deactivate_irq();

        // Check pc = rst_addr+1, and cycles != 0
	    if (status.pc != rst_addr+1 || status.cycles != 0)
	    {
            error = true; // LCOV_EXCL_LINE
	    }
    }
    
    // Clear I flag and get to next WAI instruction, which then waits
    if (!error)
    {
        // Execute CLI instruction
        status = cpu.execute();

        // Execute second WAI
        status = cpu.execute();

        // Check pc = rst_addr+2, count != 0
        if (status.pc != rst_addr+2 || status.cycles == 0)
        {
            error = true;  // LCOV_EXCL_LINE
        }
        else
        {
            // Execute WAI again
            status = cpu.execute();
            // Check pc = rst_addr+2, count != 0
            if (status.pc != rst_addr+2 || status.cycles != 0)
            {
                error = true;  // LCOV_EXCL_LINE
            }
        }
    }

    // Test IRQ when waiting with I bit clear
    if (!error)
    {
        // Activate an interrupt (but I now clear)
        cpu.activate_irq();

        // Execute
        status = cpu.execute();

        // Check pc = irq_vec+1, count != 0
        if (status.pc != irq_vec+1 || status.cycles == 0)
        {
            error = true;  // LCOV_EXCL_LINE
        }
    }

    // Test NMI when waiting

    nmi_vec  = TEST_ADDR3;

    // Load the NMI vector
    cpu.wr_mem(NMI_VEC_ADDR,    nmi_vec       & MASK_8BIT);
    cpu.wr_mem(NMI_VEC_ADDR+1, (nmi_vec >> 8) & MASK_8BIT);

    if (!error)
    {
        // Reset one more
        cpu.reset();

        // Execute WAI 
        status = cpu.execute();

        // Check pc = rst_addr, count != 0
        if (status.pc != rst_addr || status.cycles == 0)
        {
            error = true;  // LCOV_EXCL_LINE
        }
        else
        {
            // Execute WAI again
            status = cpu.execute();

            // Check pc = rst_addr, count == 0
            if (status.pc != rst_addr || status.cycles != 0)
            {
                error = true;  // LCOV_EXCL_LINE
            }
        }

        if (!error)
        {
            // Activate an NMI
            cpu.nmi_interrupt();
            
            // Execute
            status = cpu.execute();
            
            // Check pc = nmi_vec+1, count != 0
            if (status.pc != nmi_vec+1 || status.cycles == 0)
            {
                error = true;  // LCOV_EXCL_LINE
            }
        }
    }

    // ------------------------------------
    // STP test
    // ------------------------------------

    if (!error)
    {
        // Update reset vector
        uint16_t rst_addr = TEST_ADDR1;
        cpu.wr_mem(RESET_VEC_ADDR,    rst_addr       & MASK_8BIT);
        cpu.wr_mem(RESET_VEC_ADDR+1, (rst_addr >> 8) & MASK_8BIT);

        // Program at rst_addr: CLI STP NOP
        cpu.wr_mem(rst_addr,    CLI_OPCODE);
        cpu.wr_mem(rst_addr+1,  STP_OPCODE);
        cpu.wr_mem(rst_addr+2,  NOP_OPCODE_BASE);

        cpu.reset();

        // Execute CLI
        status = cpu.execute();

        // Execute STP
        status = cpu.execute();

        // Check pc = rst_addr+1, and cycles != 0
	    if (status.pc != rst_addr+1 || status.cycles == 0)
	    {
            error = true; // LCOV_EXCL_LINE
	    }
        else
        {
            // Execute again
            status = cpu.execute();

            // Check pc = rst_addr+1, and cycles == 0
	        if (status.pc != rst_addr+1 || status.cycles != 0)
	        {
                error = true; // LCOV_EXCL_LINE
	        }
        }

        // Chekc IRQs ignored when stopped, even when I bit clear
        if (!error)
        {
            // Raise interrupt (I is clear)
            cpu.activate_irq();

            // Execute again
            status = cpu.execute();

            // Deactivate IRQ
            cpu.deactivate_irq();

            // Check pc = rst_addr+1, and cycles == 0 -- i.e. Still stopped
	        if (status.pc != rst_addr+1 || status.cycles != 0)
	        {
                error = true; // LCOV_EXCL_LINE
	        }
        }

        // Restore reset vector
        cpu.wr_mem(RESET_VEC_ADDR,    start_addr       & MASK_8BIT);
        cpu.wr_mem(RESET_VEC_ADDR+1, (start_addr >> 8) & MASK_8BIT);
    }

    return error;
}

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

    bool               read_bin         = true;
    bool               read_srecord     = false;
    bool               disable_testing  = false;
    cpu_type_e         mode_c           = BASE;
    uint16_t           load_addr        = DEFAULT_LOAD_ADDR;
    uint16_t           start_addr       = DEFAULT_START_ADDR;
    uint32_t           start_dis_count  = DEFAULT_START_DIS_CNT;
    uint32_t           stop_dis_count   = DEFAULT_STOP_DIS_CNT;
    uint32_t           instr_count      = 0;

    wy65_exec_status_t status;
    bool               error            = false;
    char*              fname            = DEFAULT_PROG_FILE_NAME;
    FILE*              prog_fp          = NULL;
    int                option;

    // Process command line options
    while ((option = getopt(argc, argv, "f:I:M:l:s:S:E:cDh")) != EOF)
    {
        switch(option)
        {
        case 'f':
            fname             = optarg;
            read_bin          = true;
            read_srecord      = false;
            break;
        case 'I':
            fname             = optarg;
            read_bin          = false;
            read_srecord      = false;
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
        case 'S':
            start_dis_count   = strtol(optarg, NULL, 0);
            break;
        case 'E':
            stop_dis_count    = strtol(optarg, NULL, 0);
            break;
        case 'c':
            mode_c = WDC; // Turn on all opcodes
            break;
        case 'D':
            disable_testing = true;
            break;
        //LCOV_EXCL_START
        case 'h':
        case 'q':
            fprintf(stderr, "Usage: %s [[-f | -I | -M] <filename>][-l <addr>>][-s <addr>]\n"
                "        [-S <count>][-E <count>][-c][-D]\n\n"
                "    -f Binary program file name            (default %s)\n"
                "    -I Intel Hex program file name\n"
                "    -M Motorola S-Record program file name\n"
                "    -l Load start address of binary image  (default 0x%04x)\n"
                "    -s Start address of program execution  (default 0x%04x)\n"
                "    -S Disassemble start instruction count (default 0x%08x)\n"
                "    -E Disassemble end instruction count   (default 0x%08x)\n"
                "    -c Enable 65C02 features               (default off)\n"
                "    -D Disable testing and just run prog   (default enabled)\n"
                "\n"
                          , argv[0]
                          , DEFAULT_PROG_FILE_NAME
                          , DEFAULT_LOAD_ADDR
                          , DEFAULT_START_ADDR
                          , DEFAULT_START_DIS_CNT
                          , DEFAULT_STOP_DIS_CNT
                          );
            return option == 'h' ? GOOD_RTN_STATUS : BAD_OPTION;
            break;
        //LCOV_EXCL_STOP
        }
    }

    // Select program format type, based on user selections
    prog_type_e ptype = read_bin ? BIN : read_srecord ? SREC : HEX;

    // Read in specified program to memory
    if (cpu.read_prog(fname, ptype,  load_addr) != PROG_NO_ERROR)
    {
        return BAD_FILE_OPEN;
    }

    // Load the reset vector with the user start location, if specified
    cpu.wr_mem(RESET_VEC_ADDR,    start_addr       & MASK_8BIT);
    cpu.wr_mem(RESET_VEC_ADDR+1, (start_addr >> 8) & MASK_8BIT);

    // ------------------------------------
    // Interrupt tests
    // ------------------------------------

    if (!disable_testing && !error)
    {
        error = interrupt_test(mode_c, start_addr);
    }

    // ------------------------------------
    // WAI and STP tests
    // ------------------------------------

    // Only execute WAI and STP if selected opcode mode supports these
    if (!disable_testing && !error && mode_c >= WDC)
    {
        error = wait_stop_tests(start_addr);
    }

    // ------------------------------------
    // Main test program run
    // ------------------------------------

    if (!error)
    {
        bool     terminate        = false;
        uint16_t prev_pc          = 0;

        // Initialise status.pc to something for termination condition
        status.pc = 0xffff;

        if (!disable_testing)
        {
            // Load failure status to nominated location
            cpu.wr_mem(TEST_STATUS_ADDR,    BAD_TEST_STATUS       & MASK_8BIT);
            cpu.wr_mem(TEST_STATUS_ADDR+1, (BAD_TEST_STATUS >> 8) & MASK_8BIT);
        }

        fprintf(stdout, "Executing %s from address 0x%04x ...\n\n", fname, start_addr); 

        // Assert a reset
        cpu.reset();

        // Start the clock
        pre_run_setup();  

        // Start executing instructions
        do 
        {
            prev_pc = status.pc;

            status = cpu.execute(instr_count++, start_dis_count, stop_dis_count, true);
        }
        // Terminate if looping back to same instruction (i.e. deliberately hung)
        while (prev_pc != status.pc);

        // Stop the clock
        post_run_setup();

        if (!disable_testing)
        {
            // Get test status values from memory
            uint16_t test_status = (cpu.rd_mem(TEST_STATUS_ADDR+1) << 8) | cpu.rd_mem(TEST_STATUS_ADDR);

            error = test_status != GOOD_TEST_STATUS;
        }
    }

    // -----------------------------------
    // Display PASS/FAIL result
    // -----------------------------------

    if (!disable_testing)
    {
        if (!error)
        {
            fprintf(stdout, "********\n");
            fprintf(stdout, "* PASS *\n");
            fprintf(stdout, "********\n\n");
        
            fprintf(stdout, "Executed %.2f million instructions (%.1f MIPS)\n\n", (float)instr_count/1e6, (float)instr_count/tv_diff);
        }
        // LCOV_EXCL_START
        else
        {
            fprintf(stderr, "********\n");
            fprintf(stderr, "* FAIL *\n");
            fprintf(stderr, "********\n\n");
        
            fprintf(stderr, "Terminated at PC = 0x%04x after %d instructions\n\n", status.pc, instr_count);
        }
        // LCOV_EXCL_STOP
    }
    else
    {
        fprintf(stderr, "Terminated at PC = 0x%04x after %d instructions\n\n", status.pc, instr_count);
    }

    return GOOD_RTN_STATUS;
}

#endif