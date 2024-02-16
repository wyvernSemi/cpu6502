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
#include "pia.h"

// -------------------------------------------------------------------------
// LOCAL DEFINES
// -------------------------------------------------------------------------

#define STRBUFSIZE      256
#define MEMTOP          0x10000
#define RAMTOP          0x8000
#define PAGEMASK        0xF000
#define PIAPAGEADDR     0xD000
#define LOAD_BIN_ADDR   0x8000

#define UNSET           -1

// Default filenames
#define HEXPROGNAME     "cpu6502.ihex"
#define BINPROGNAME     "cpu6502.bin"

// -------------------------------------------------------------------------
// LOCAL STATICS
// -------------------------------------------------------------------------

static uint8_t mem[MEMTOP];
static bool    nolf;
static bool    rom_wr_en;

// -------------------------------------------------------------------------
// Callback for cpu6502 memory writes
// -------------------------------------------------------------------------

static void write_cb (int addr, unsigned char wbyte)
{
    // PIA register
    if ((addr & PAGEMASK) == PIAPAGEADDR)
    {
        pia (addr & ~PAGEMASK, wbyte, false, nolf);
    }
    // RAM write
    else
    {
        // Only write to ROM if enabled (for loading code)
        if (rom_wr_en || addr < RAMTOP)
        {
            mem[addr % MEMTOP] = wbyte;
        }
    }
}

// -------------------------------------------------------------------------
// Callback for cpu6502 memory reads
// -------------------------------------------------------------------------

static int read_cb (int addr)
{
    int rbyte;
    
    // PIA register
    if ((addr & PAGEMASK) == PIAPAGEADDR)
    {
        rbyte = pia (addr & ~PAGEMASK, 0, true);
    }
    // RAM read
    else
    {
        rbyte = mem[addr % MEMTOP];
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
    rst_vector = UNSET;
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

    // Allow ROM writes when loading program
    rom_wr_en = true;

    // Load the program to memory
    if (p_cpu->read_prog(fname, type, load_addr))
    {
        fprintf(stderr, "***ERROR: failed to load program\n");
        return 1;
    }

    // Disable ROM writes
    rom_wr_en = false;

    // Update the reset vector if set on the command line
    if (rst_vector != UNSET)
    {
        p_cpu->wr_mem(RESET_VEC_ADDR,    rst_vector       & MASK_8BIT);
        p_cpu->wr_mem(RESET_VEC_ADDR+1, (rst_vector >> 8) & MASK_8BIT);
    }

    // Reset the CPU and choose Western Digital instruction extensions
    p_cpu->reset(WDC);

    // Run forever
    p_cpu->run_forever(disassem);

    // Shouldn't reach this point
    return 0;
}