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

#include "pia.h"

// -------------------------------------------------------------------------
// Keyboard input LINUX/mingw64 emulation functions
// -------------------------------------------------------------------------

#if !(defined _WIN32) && !defined(_WIN64)
// -------------------------------------------------------------------------
// Keyboard input LINUX/CYGWIN emulation functions
// -------------------------------------------------------------------------

// Implement _kbhit() locally for non-windows platforms
static int _kbhit(void)
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
// Simple model of PIA
// -------------------------------------------------------------------------

int pia (const int addr, const int wbyte, const bool rnw, const bool nolf)
{
    // Static store for last keyboard press
    static int lastkey = 0;

    // Return byte defaults to 0
    int rbyte          = 0;

    if (rnw)
    {

        switch(addr)
        {
        case KBD:
            // return last keyboard input with b7 set
            rbyte      = lastkey | BIT7;
            break;

        case KBDCR:
            // if keyboard input available, return 0x80, else 0x00
            if (LM32_INPUT_RDY_TTY())
            {
                lastkey = LM32_GET_INPUT_TTY() & BYTEMASK;
                rbyte   = BIT7;
            }
            break;

        // All other register reads return default of 0
        default:
            break;
        }
    }
    else
    {
        switch(addr)
        {
        // If a CR, output CR and LF
        case DSP:
            if ((wbyte & ASCIIMASK) == CR)
            {
                LM32_OUTPUT_TTY(CR);
                if (!nolf)
                {
                    LM32_OUTPUT_TTY(LF);
                }
            }
            // Valid byte if top bit set. Output with b7 cleared.
            else if (wbyte > ASCIIMASK)
            {
                LM32_OUTPUT_TTY(wbyte & ASCIIMASK);
            }
            break;

        // All other register writes ignored
        default:
            break;
        }
    }

    return rbyte;
}