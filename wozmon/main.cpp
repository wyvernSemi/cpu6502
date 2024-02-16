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
#include <cstring>

#include "cpu6502.h"

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

#define STRBUFSIZE      256

#define DONT_CARE       0
#define MEMTOP          0x10000
#define START_ADDR      0xff00

#define KBD             0xD010
#define KBDCR           0xD011
#define DSP             0xD012
#define DSPCR           0xD013

#define LOAD_BIN_ADDR   0x8000

#define HEXPROGNAME     "cpu6502.ihex"
#define BINPROGNAME     "cpu6502.bin"

#define CR              0x0D
#define LF              0x0A

// -------------------------------------------------------------------------
// MACRO DEFINITIONS
// -------------------------------------------------------------------------

// Hide input and output function specifics for ease of future updating
#define LM32_OUTPUT_TTY(_x) {putchar(_x);}
#define LM32_INPUT_RDY_TTY _kbhit
#define LM32_GET_INPUT_TTY _getch

// -------------------------------------------------------------------------
// LOCAL STATICS
// -------------------------------------------------------------------------

static uint8_t mem[MEMTOP];
static bool    nolf;

// -------------------------------------------------------------------------
// Keyboard input LINUX/mingw64 emulation functions
// -------------------------------------------------------------------------

#if !(defined _WIN32) && !defined(_WIN64)
// -------------------------------------------------------------------------
// Keyboard input LINUX/CYGWIN emulation functions
// -------------------------------------------------------------------------


// Implement _kbhit() locally for non-windows platforms
int _kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  // Get current terminal attributes and copy to new
  tcgetattr(STDIN_FILENO, &oldt);
  newt           = oldt;
  
  // Configure for non-canonical and no echo
  newt.c_lflag &= ~(ICANON | ECHO);

  // Set new attributes
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  
  // Get stdin attributes
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  
  // Set stdin attributes for non-blocking
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  // Attempt to read a character. Returns EOF if non-available.
  ch = getchar();
 
  // Restore all attributes.
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  // If a character was available, return 1 and put the character
  // back in the queue, mapping LF to CR
  if(ch != EOF)
  {
      // Map line feed generated in Linux to carriage return
      // expected by MS Basic
      if (ch == LF)
      {
          ungetc(CR, stdin);
      }
      // Else print the character
      else
      {
          ungetc(ch, stdin);
      }
      return 1;
  }
 
  return 0;
}

// getchar() is okay for _getch() on non-windows platforms
#define _getch getchar

#endif

// -------------------------------------------------------------------------
// Initialise curses terminal for Linux (do nothing for Windows except
// clear the screen)
// -------------------------------------------------------------------------

void init_term()
{
#if 0 && !(defined _WIN32) && !defined(_WIN64)
    initscr();
    noecho();
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
        switch(addr)
        {
        // If a CR, output CR and LF
        case DSP:
            if ((wbyte & 0x7f) == 0x0d)
            {
                LM32_OUTPUT_TTY(0x0d);
                if (!nolf)
                {
                    LM32_OUTPUT_TTY(0x0a);
                }
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
// Command line argument parser
// -------------------------------------------------------------------------

static int parse_args(int argc, char**argv, bool &nolf, bool &disassem, prog_type_e &type, int &load_addr, int &rst_vector, char* fname)
{
    char option;

    // Default setting
    nolf       = false;
    disassem   = false;
    load_addr  = LOAD_BIN_ADDR;
    rst_vector = -1;
    type       = BIN;
    strncpy(fname, BINPROGNAME, STRBUFSIZE);
    
    bool fnamegiven  = false;

    // Process command line options
    while ((option = getopt(argc, argv, "f:t:l:r:ndh")) != EOF)
    {
        switch(option)
        {
        case 'n':
            nolf              = true;
            break;
        case 'd':
            disassem          = true;
            break;
        case 'l':
            load_addr         = (int)(strtol(optarg, NULL, 0) & 0xffff);
            break;
        case 'r':
            rst_vector        = (int)(strtol(optarg, NULL, 0) & 0xffff);
            break;
        case 't':
            if (!strcmp(optarg, "HEX") || !strcmp(optarg, "hex"))
            {
              type            = HEX;
              if (!fnamegiven)
              {
                  strncpy(fname, HEXPROGNAME, STRBUFSIZE);
              }
            }
            else if (!strcmp(optarg, "BIN") || !(strcmp(optarg, "bin")))
            {
              type            = BIN;
            }
            else
            {
                fprintf(stderr, "Unrecognised file type\n");
                return 1;
            }
            break;
        case 'f':
            strncpy(fname, optarg, STRBUFSIZE);
            fnamegiven        = true;
            break;
        case 'h':
            fprintf(stderr, "Usage: %s [-f <filename>][-l <addr>][-t <program type>][n][-d]\n\n"
                "    -t Program format type                 (default BIN)\n"
                "    -f program file name                   (default %s.[ihex|bin] depending on format)\n"
                "    -l Load start address of binary image  (default 0x%04x)\n"
                "    -r Reset vector address                (default set from program)\n"
                "    -n Disable line feed generation        (default false)\n"
                "    -d Enable disassembly                  (default false)\n"
                "\n"
                          , argv[0]
                          , "cpu6502"
                          , LOAD_BIN_ADDR
                          );
            return 1;
            break;
        }
    }

    return 0;
}


// -------------------------------------------------------------------------
// ---------------------------  M  A  I  N  --------------------------------
// -------------------------------------------------------------------------

int main (int argc, char** argv)
{
    bool        disassem = false;
    prog_type_e type = HEX;
    char        fname[STRBUFSIZE];
    int         load_addr;
    int         rst_vector;

    // Parse command line arguments
    if (parse_args(argc, argv, nolf, disassem, type, load_addr, rst_vector, fname))
    {
        return 1;
    }

    // Create a cp6502 CPU object
    cpu6502 *p_cpu = new cpu6502;

    // Register the memory access callback functions
    p_cpu->register_mem_funcs(write_cb, read_cb);

    if (p_cpu->read_prog(fname, type, load_addr))
    {
        fprintf(stderr, "***ERROR: failed to load program\n");
        return 1;
    }

    // Set the reset vector
    if (rst_vector >= 0)
    {
        p_cpu->wr_mem(RESET_VEC_ADDR,    rst_vector       & MASK_8BIT);
        p_cpu->wr_mem(RESET_VEC_ADDR+1, (rst_vector >> 8) & MASK_8BIT);
    }

    // Reset the CPU and choose Western Digital instruction extensions
    p_cpu->reset(WDC);

    // Initialise the output terminal
    init_term();

    // Run forever
    p_cpu->run_forever(disassem);

    // Shouldn't reach this point
    return 0;
}