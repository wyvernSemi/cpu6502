//=============================================================
// 
// Copyright (c) 2012-2017 Simon Southwell. All rights reserved.
//
// Date: 25th August 2012  
//
// This file is part of the cpu6502 instruction set simulator
// and contains the intel hex format code loader  header for
// the ISS.
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
// $Id: read_ihx.h,v 1.4 2017/02/11 08:41:46 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/read_ihx.h,v $
//
//=============================================================

#ifndef _READ_IHX_H_
#define _READ_IHX_H_

#include <stdint.h>

#include "cpu6502_api.h"

#define IHX_NO_ERROR                0
#define IHX_FILE_ERROR              1
#define IHX_FORMAT_ERROR            2
#define IHX_UNRECOGNISED_TYPE       3

#define IHX_MAXLINE                 1024

#define IHX_TYPE_DATA               0
#define IHX_TYPE_EOF                1

#define SREC_DATA_ADDR16            1
#define SREC_DATA_ADDR24            2
#define SREC_DATA_ADDR32            3
#define SREC_RSVD                   4
#define SREC_COUNT16                5
#define SREC_COUNT32                6
#define SREC_START_ADDR32           7
#define SREC_START_ADDR24           8
#define SREC_START_ADDR16           9

#define SREC_NO_ERROR               IHX_NO_ERROR
#define SREC_FILE_ERROR             IHX_FILE_ERROR
#define SREC_FORMAT_ERROR           IHX_FORMAT_ERROR
#define SREC_UNRECOGNISED_TYPE      IHX_UNRECOGNISED_TYPE 

#define BIN_NO_ERROR                IHX_NO_ERROR
#define BIN_FILE_ERROR              IHX_FILE_ERROR

#define PROG_NO_ERROR               IHX_NO_ERROR

#endif
