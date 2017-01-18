//=============================================================
// 
// Copyright (c) 2012-2017 Simon Southwell. All rights reserved.
//
// Date: 25th August 2012  
//
// This file is part of the cpu6502 instruction set simulator
// and contains the intel hex format code loader for the ISS.
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
// $Id: read_ihx.cpp,v 1.5 2017/01/18 12:24:34 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/read_ihx.cpp,v $
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "read_ihx.h"

// -------------------------------------------------------------------------
// hex2int()
//
//   Convert characters in a buffer representing hex data into an integer
//   value
// -------------------------------------------------------------------------

int cpu6502::hex2int(const uint8_t *buf, int num_chars)
{
    int idx;
    int value = 0;

    for (idx = 0; idx < num_chars; idx++)
    {
        int digit;

        digit = *(buf+idx);

        if (digit >= 'A' && digit <= 'F')
        {
            digit = tolower(digit);
        }

        value <<= 4;

        if ((digit >= 'a') && (digit <= 'f'))
        {
            value |= digit - 'a' + 10;
        }
        else
        {
            value |= digit - '0';
        }
    }

    return value;
}


// -------------------------------------------------------------------------
// write_data()
//
//   Convert two byte ASCII from buf_ptr onwards, convert to byte value 
//   and write to memory, starting at  address, for as many output bytes as 
//   specified in byte_count.
// -------------------------------------------------------------------------

void cpu6502::prog_write_data(const uint32_t byte_count, const uint32_t addr, const uint8_t* buf_ptr)
{
    uint32_t data_byte;
    uint32_t address = addr;

    for (unsigned idx = 0; idx < (byte_count << 1); idx +=2)
    {
        // Get data bytes
        data_byte = hex2int(buf_ptr + idx, 2);

        wr_mem(address++, data_byte);
    }
}

// -------------------------------------------------------------------------
// read_ihx()
//
//   Read a file in intel hex format and write to memory. (No checksum 
//   verification --- 2's complement of sum mod 256 over data bytes.) 
// -------------------------------------------------------------------------

int cpu6502::read_ihx (const char *filename)
{

    uint32_t byte_count;
    uint32_t address;
    uint32_t record_type;

    char line [IHX_MAXLINE];
    int eof = 0;

    FILE *file = fopen (filename, "r");

    if (file == NULL)
    {
        return IHX_FILE_ERROR;              // LCOV_EXCL_LINE
    }

    // Read in a line at a time (i.e. one record)
    while (fgets (line, sizeof line, file) != NULL) 
    {
        uint8_t* buf_ptr = (uint8_t *)line;

        // Check for intel hex format leading colon
        if (*buf_ptr != ':')
        {
            fclose(file);                   // LCOV_EXCL_LINE
            return IHX_FORMAT_ERROR;        // LCOV_EXCL_LINE
        }
        else
        {
            buf_ptr++;
        }

        // Get byte count
        byte_count = hex2int(buf_ptr, 2);
        buf_ptr += 2;

        // Get address
        address    = hex2int(buf_ptr, 4);
        buf_ptr += 4;

        // Get record type
        record_type = hex2int(buf_ptr, 2);
        buf_ptr += 2;
        
        // If data record, then process
        if (record_type == IHX_TYPE_DATA)
        {
            prog_write_data(byte_count, address, buf_ptr);
        }
        // If EOF type, the break from loop
        else if (record_type == IHX_TYPE_EOF)
        {
            break;
        }
        // Got a type we don't support
        else
        {
            fclose(file);                  // LCOV_EXCL_LINE
            return IHX_UNRECOGNISED_TYPE;  // LCOV_EXCL_LINE
        }
    }

    fclose(file);

    // Got here if everything okay
    return IHX_NO_ERROR;
}

// -------------------------------------------------------------------------
// read_srec()
//
//   Read a file in Motorola S-Record format and write to memory. (No 
//   checksum verification --- inverse of sum mod 256 over byte count, 
//   address and data.)
// -------------------------------------------------------------------------

int cpu6502::read_srec (const char *filename)
{

    uint32_t byte_count;
    uint32_t address;
    uint32_t record_type;

    char line [IHX_MAXLINE];
    int eof = 0;

    FILE *file = fopen (filename, "r");

    if (file == NULL)
    {
        return SREC_FILE_ERROR;            // LCOV_EXCL_LINE
    }

    // Read in a line at a time (i.e. one record)
    while (fgets (line, sizeof line, file) != NULL) 
    {
        uint8_t* buf_ptr = (uint8_t *)line;

        // Check for Motorola S-REC format leading 'S'
        if (*buf_ptr != 'S')
        {
            fclose(file);                  // LCOV_EXCL_LINE
            return SREC_FORMAT_ERROR;      // LCOV_EXCL_LINE
        }
        else
        {
            buf_ptr++;
        }

        // Get record type
        record_type = hex2int(buf_ptr, 1);
        buf_ptr += 1;

        // Get byte count
        byte_count = hex2int(buf_ptr, 2);
        buf_ptr += 2;

        // Get address -- S1 record is 2 byte address, S2 is 3 bytes and S3 is 4 bytes
        address    = hex2int(buf_ptr, (1 +  record_type) << 1);
        buf_ptr += (1 +  record_type) << 1;

        // Adjust byte count to remove address and checksum, leaving just data count
        byte_count -= (1 +  record_type) + 1;
        
        // If data record, then process
        if (record_type >=  SREC_DATA_ADDR16 && record_type <= SREC_DATA_ADDR32)
        {
            prog_write_data(byte_count, address, buf_ptr);
        }
        // Got an unsupported type. (Recognised, but Non-data, records skipped)
        else if (record_type > SREC_START_ADDR16 || record_type == SREC_RSVD)
        {
            fclose(file);                  // LCOV_EXCL_LINE
            return SREC_UNRECOGNISED_TYPE; // LCOV_EXCL_LINE
        }
    }

    fclose(file);

    // Got here if everything okay
    return SREC_NO_ERROR;
}

// -------------------------------------------------------------------------
// read_bin()
//
//   Read a file in raw binary format and write to memory. A start_addr
//   arugment is needed (default of 0), as this information is not supplied
//   in the binary file.
//
// -------------------------------------------------------------------------

int cpu6502::read_bin (const char *filename, const uint16_t start_addr)
{
    FILE* file;
    uint16_t load_addr = start_addr;

    // Load internal memory with binary file, starting at defined adddress
    if ((file = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "***ERROR: unable to open file %s for reading\n", filename); // LCOV_EXCL_LINE
        return BIN_FILE_ERROR;                                                       // LCOV_EXCL_LINE
    }

    int c;

    while ((c = fgetc(file)) != EOF)
    {
        wr_mem(load_addr++, c & 0xff);
    }

    fclose(file);

    return BIN_NO_ERROR;
}
