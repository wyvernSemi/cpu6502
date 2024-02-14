//=============================================================
//
// Copyright (c) 2024 Simon Southwell. All rights reserved.
//
// Date: 12th February 2024
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
#include <cstdlib>
#include <cstdint>

#include "cpu6502.h"

#if defined _WIN32 || defined _WIN64
#include <conio.h>
#else
#include <curses.h>
#endif

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define DONT_CARE   0
#define MEMTOP      0x10000
#define START_ADDR  0xff00

#define KBD         0xD010
#define KBDCR       0xD011
#define DSP         0xD012
#define DSPCR       0xD013

#define PROGNAME    "wozmon.ihex"

// -------------------------------------------------------------------------
// MACRO DEFINITIONS
// -------------------------------------------------------------------------

// Hide input and output function specifics for ease of future updating
#define LM32_OUTPUT_TTY(_x) putchar(_x)
#define LM32_INPUT_RDY_TTY _kbhit
#define LM32_GET_INPUT_TTY _getch

// -------------------------------------------------------------------------
// LOCAL STATICS
// -------------------------------------------------------------------------

static uint8_t mem[MEMTOP];

// -------------------------------------------------------------------------
// Keyboard input LINUX/mingw64 emulation functions
// -------------------------------------------------------------------------

#if !(defined _WIN32) && !defined(_WIN64)

// Implement _kbhit() locally for non-windows platforms
// using curses
int _kbhit(void)
{
  int rtnval;
  int c;

  // Attempt to get a charcater from the input
  c = getch();

  // If the returned value is an error, then no character was ready
  // so return 0
  if (c == ERR)
  {
      rtnval =  0;
  }
  else
  {
      // Put the character just popped back in the queue and
      // flag that a character is ready
      ungetch(c);
      rtnval = 1;
  }

  return rtnval;
}

// Map curses getch to the windows equivalent
#define _getch getch

#endif

// -------------------------------------------------------------------------
// Initialise curses terminal for Linux (do nothing for Windows except
// clear the screen)
// -------------------------------------------------------------------------

void init_term()
{
#if !(defined _WIN32) && !defined(_WIN64)
    initscr();
    nodelay(stdscr, true);
    nonl();

    // Curses 'steals' the first output for some reason so output
    // the prompt
    addch('\\');
    addch('\n');
#else
    printf("\033c");
#endif
}

// -------------------------------------------------------------------------
// Callback for cpu6502 memory writes
// -------------------------------------------------------------------------

void write_cb (int addr, unsigned char wbyte)
{
    // If not a PIA register, access memory
    if (addr < KBD || addr > DSPCR)
    {
        mem[addr % MEMTOP] = wbyte;
    }
    // PIA register
    else
    {
        //fprintf(stderr, "addr=0x0%4x wbyte=0x%02x\n", addr, wbyte);
        switch(addr)
        {
        // If a CR, output CR and LF
        case DSP:
            //fprintf(stderr, "0x%02x\n", wbyte);
            if ((wbyte & 0x7f) == 0x0d)
            {
                LM32_OUTPUT_TTY(0x0d);
                LM32_OUTPUT_TTY(0x0a);
            }
            // Valid byte if top bit set. Output with b7 cleared.
            else if (wbyte > 0x7f)
            {
                LM32_OUTPUT_TTY(wbyte & 0x7f);
            }
            break;

        // All other register writes ignored
        default:
            break;
        }
    }
}

// -------------------------------------------------------------------------
// Callback for cpu6502 memory reads
// -------------------------------------------------------------------------

int  read_cb  (int addr)
{
    // Return byte defaults to 0
    int rbyte          = 0;

    // Static store for last keyboard press
    static int lastkey = 0;

    // If not a PIA register, access memory
    if (addr < KBD || addr > DSPCR)
    {
        rbyte          =  mem[addr % MEMTOP];
    }
    // PIA access
    else
    {
        switch(addr)
        {
        case KBD:
            // return last keyboard input with b7 set
            rbyte      = lastkey | 0x80;
            break;

        case KBDCR:
            // if keyboard input available, return 0x80, else 0x00
            if (LM32_INPUT_RDY_TTY())
            {
                lastkey = LM32_GET_INPUT_TTY() & 0xffU;
                rbyte   = 0x80;
            }
            break;

        // All other register reads return default of 0
        default:
            break;
        }
    }

    return rbyte;
}

// -------------------------------------------------------------------------
// ---------------------------  M  A  I  N  --------------------------------
// -------------------------------------------------------------------------

int main (int argc, char** argv)
{
    // Create a cp6502 CPU object
    cpu6502 *p_cpu = new cpu6502;

    // Initialise the output terminal
    init_term();

    // Register the memory access callback functions
    p_cpu->register_mem_funcs(write_cb, read_cb);

    // Load the Woz monitor program to memory
    p_cpu->read_prog(PROGNAME, HEX, DONT_CARE);

    // Reset the CPU and choose Western Digital instruction extensions
    p_cpu->reset(WDC);

    // Execution loop (forever)
    while (true)
    {
        p_cpu->execute();
    }
}