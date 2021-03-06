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
// $Id: cpu6502.h,v 1.9 2017/03/03 11:15:39 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/cpu6502.h,v $
//
//=============================================================

#ifndef _CPU6502_H_
#define _CPU6502_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdint.h>
#include "cpu6502_api.h"

#if !(defined _WIN32) && !(defined _WIN64)
#ifdef CYGWIN
#include <GetOpt.h>
#else
#include <unistd.h>
#endif
#else 
extern "C" {
extern int getopt(int nargc, char** nargv, char* ostr);
extern char* optarg;
extern int optind;
}
#endif

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

// Defines used by main()
#define DEFAULT_PROG_FILE_NAME   "test.bin"
#define DEFAULT_LOAD_ADDR        0x000a
#define DEFAULT_START_ADDR       0x0400
#define DEFAULT_START_DIS_CNT    0xffffffff
#define DEFAULT_STOP_DIS_CNT     0xffffffff
#define BAD_TEST_STATUS          0x0bad
#define GOOD_TEST_STATUS         0x900d
#define GOOD_RTN_STATUS          0
#define BAD_OPTION               1
#define BAD_FILE_OPEN            2

// Opcode utility defines
#define INVALID_ADDR             0xffffffff
#define CARRY_MASK               0x01
#define ZERO_MASK                0x02
#define INT_MASK                 0x04
#define BCD_MASK                 0x08
#define BRK_MASK                 0x10
#define RSVD_MASK                0x20
#define OVFLW_MASK               0x40
#define SIGN_MASK                0x80

// Support up to 16 wired-or IRQs
#define NUM_INTERNAL_IRQS        16
#define NO_ACTIVE_IRQS           ((1U << NUM_INTERNAL_IRQS) -1)

#define IRQ_CYCLES               7
#define NMI_CYCLES               7
#define RST_CYCLES               0 /* Should be 7? */

#define INVALID_NEXT_PC          0xffffffff

#define MASK_LO_NIB              0x0fU
#define MASK_HI_NIB              0xf0U
#define MASK_8BIT                0xffU
#define MASK_16BIT               0xffffU
#define MASK_32BIT               0xffffffffU

#define NOP_OPCODE_BASE          0xea
#define CLI_OPCODE               0x58
#define WAI_OPCODE               0xcb
#define STP_OPCODE               0xdb

#define NMI_VEC_ADDR             0xfffa
#define RESET_VEC_ADDR           0xfffc
#define IRQ_VEC_ADDR             0xfffe

#define TEST_ADDR1               0xfff0
#define TEST_ADDR2               0xfff2
#define TEST_STATUS_ADDR         0xfff8
#define TEST_ADDR3               0x0400

// The 65C02 0x4B NOP instruction needs to be a two byte instruction
// (apparently) when running on the BeeEm platform. Otherwise some programs
// do not work correctly (e.g. Zalaga). But the 65c02 should map all 
// undocumented instructions to NOP (with no operand), and the test_65c02.a65
// code expects this, and fails if it isn't. So make it different when
// in BeebEm (where TEST_CPU6502 will be defined).

#ifdef TEST_CPU6502
#define N4B                      IMM /* two byte instruction */
#else   
#define N4B                      NON /* Single byte instruction */
#endif

#endif