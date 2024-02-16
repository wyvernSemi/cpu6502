//=============================================================
//
// Copyright (c) 2024 Simon Southwell. All rights reserved.
//
// Date: 16th February 2024
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
//=============================================================

#include <cstdio>

#ifndef _PIA_H_

#if defined _WIN32 || defined _WIN64
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

// New line character definitions
#define CR              0x0D
#define LF              0x0A

#define BIT7            0x80
#define BYTEMASK        0xFF
#define ASCIIMASK       0x7F

// PIA register offset definitions
#define KBD             0x0010
#define KBDCR           0x0011
#define DSP             0x0012
#define DSPCR           0x0013

// -------------------------------------------------------------------------
// MACRO DEFINITIONS
// -------------------------------------------------------------------------

// Hide input and output function specifics for ease of future updating
#define LM32_OUTPUT_TTY(_x) putchar(_x)
#define LM32_INPUT_RDY_TTY _kbhit
#define LM32_GET_INPUT_TTY _getch

// -------------------------------------------------------------------------
// EXTERNAL PROTOTYPES
// -------------------------------------------------------------------------

extern int pia (const int addr, const int wbyte, const bool rnw, const bool nolf = false);

#endif