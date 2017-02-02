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
// $Id: cpu6502_api.h,v 1.9 2017/02/02 10:51:42 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/cpu6502_api.h,v $
//
//=============================================================

#ifndef _CPU6502_API_H_
#define _CPU6502_API_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>

// -------------------------------------------------------------------------
// DEFINES (override-able)
// -------------------------------------------------------------------------

// Allow internal memory size to be overridden at compile time, with a 
// default size of 65536 (the maximum addressable space).
#ifndef WY65_MEM_SIZE
#define WY65_MEM_SIZE                 0x10000
#endif

// Define WY65_STANDALONE to add top level main() test function to drive model.
// #define WY65_STANDALONE

// Define WY65_EN_PRINT_CYCLES to enable cycle counts in disassemble output
// #define WY65_EN_PRINT_CYCLES

// -------------------------------------------------------------------------
// DEFINES (non-override-able)
// -------------------------------------------------------------------------

// The 8 bit architecture give a maximum of 256 possible opcodes.
#define WY65_INSTR_SPACE_SIZE         256

// -------------------------------------------------------------------------
// TYPE DEFINITIONS
// -------------------------------------------------------------------------

// Enumerated type for opcode variant modes
enum cpu_type_e {
    BASE = 0, // base MOS6502 instructions
    C02  = 1, // instructions introduced by 65C02
    WRK  = 2, // Additional instructions in WDC and Rockwell
    WDC  = 3, // Addition WDC only instructions
    DEFAULT   // Use default (or existing) mode
};

enum prog_type_e {
    BIN,
    HEX,
    SREC
};

// Structure for return status of wy65_execute() function
typedef struct 
{
    uint16_t          pc;      // PC value *after* instruction
    uint32_t          cycles;  // Cycle count of instruction execution
    uint8_t           flags;

} wy65_exec_status_t;

// Structure for model's registers
typedef struct
{
    uint16_t          pc;
    uint8_t           a;
    uint8_t           x;
    uint8_t           y;
    uint8_t           sp;        // SP is a 16 bit number, but implied upper byte is always 0x01 (i.e. page 1)
    uint8_t           flags;

} wy65_reg_t;

// Collected internal state of processor model 
typedef struct 
{
    uint8_t           mem [WY65_MEM_SIZE];
    wy65_reg_t        regs;
    uint64_t          cycles;
    uint16_t          nirq_line;
    cpu_type_e        mode_c;
    bool              waiting;
    bool              stopped;
} wy65_cpu_state_t;

// Define required types for external memory access functions
typedef void (*wy65_p_writemem_t)(int, unsigned char);
typedef int  (*wy65_p_readmem_t) (int);

// -------------------------------------------------------------------------
// CLASS DEFINITION
// -------------------------------------------------------------------------

// Class definitions of the model
class cpu6502
{
// Type definitions private to this class
private:
    // Address mode enumeration
    enum addr_mode_e {
        IND,
        IDX,
        IDY,
        ABS,
        ABX,
        ABY,
        IMM,
        ZPG,
        ZPX,
        ZPY,
        ACC,
        REL,
        ZPR, // WDC65C02 only
        IAX, // 65C02
        IDZ, // 65C02
        NON
    };

    // Opcode decode information
    typedef struct
    {
        uint8_t           opcode;
        addr_mode_e       mode;
        uint32_t          exec_cycles;

    } op_t;
    
    // Instruction function pointer type, for use in instruction table
    typedef int (cpu6502::* pInstrFunc_t) (const op_t*);
    
    // Instruction table entry type.
    typedef struct
    {
        const char*       op_str;
        pInstrFunc_t      pFunc;
        uint32_t          exec_cycles;
        addr_mode_e       addr_mode;
        cpu_type_e        cpu_type;
    } tbl_t; 

// Public methods
public:

    // Constructor
                       cpu6502(); 

    // Reset function. Also clears cycle count and any active IRQ lines. Sets
    // supported opcode mode (default BASE)
    void               reset              (cpu_type_e mode                 = DEFAULT);

    // Generate an NMI. NMI is falling edge triggered, and this function call
    // emulates this single event.
    void               nmi_interrupt      (void);

    // Activate (set to 0)/ deactive (set 1) a level sensitive IRQ line, with optional ID (default 0). 
    // ID can be used to emulate up to 16 wire-ORed IRQs.
    void               activate_irq       (const uint16_t id               = 0);
    void               deactivate_irq     (const uint16_t id               = 0);

    // Execute a single instruction. Internally checks for outstanding IRQ first, 
    // before proceeding to execute an instruction. Optional control of disassembling
    // instruction if icount is between start_count and stop_count. Jump marks
    // (marked spaces between PC discontinuities) can be enabled/disabled.
    wy65_exec_status_t execute            (const uint32_t icount           = 0, 
                                           const uint32_t start_count      = 0xffffffff, 
                                           const uint32_t stop_count       = 0xffffffff,
                                           const bool     en_jmp_mrks      = true);

    // Register external memory functions for use in memory read/write accesses,
    // to allow interfacing with external memory system.
    void               register_mem_funcs (wy65_p_writemem_t p_wfunc, wy65_p_readmem_t  p_rfunc);

    // Read program into memory. Call *after* register_mem_funcs(), if this is used.
    int                read_prog          (const char *filename, const prog_type_e type = HEX, const uint16_t start_addr = 0);

// Private member functions
private:
    // Read Binary, Intel HEX or Motorola S-Record files into memory
    int                read_bin           (const char *filename, const uint16_t start_addr = 0);
    int                read_ihx           (const char *filename);
    int                read_srec          (const char *filename);

    // Internal check and execution of maskable interrupts
    void               irq                (void);

    // Disassemble opcode to logfile
    void               disassemble        (const int      opcode, 
                                           const uint16_t pc, 
                                           const uint64_t cycles, 
                                           const bool     disable_jmp_mrk, 
                                           const bool     enable_regs_disp, 
                                           const uint8_t  a, 
                                           const uint8_t  x, 
                                           const uint8_t  y, 
                                           const uint8_t  sp, 
                                           const uint8_t  flags,
                                           const char*    fname = "cpu6502.log");

    // Calculate and return the operand address, based on instruction addressing mode
    uint32_t           calc_addr          (const addr_mode_e mode, wy65_reg_t* p_regs, bool &pg_crossed);

    // Utility method to set an entry in the instruction table
    inline void        set_tbl_entry      (tbl_t &t, char* s, pInstrFunc_t f, uint32_t c, addr_mode_e m, cpu_type_e cpu) {
                                              t.op_str      =  s;
                                              t.pFunc       =  f;
                                              t.exec_cycles =  c;
                                              t.addr_mode   =  m;
                                              t.cpu_type    =  cpu;
                                          };
    // Utility to write program data to memory
    void               prog_write_data    (const uint32_t byte_count, const uint32_t addr, const uint8_t* buf_ptr);

    // Utility to convert an ASCII hex string to an integer
    int                hex2int            (const uint8_t *buf, int num_chars);

#ifdef WY65_STANDALONE
// In stand-alone mode, export the local memory access methods, otherwise keep private
public:
#else
private:
#endif
    // Write to memory---either local, or via externally set method
    inline void        wr_mem             (int addr, unsigned char data) {
                                              if (ext_wr_mem != NULL) 
                                                  ext_wr_mem(addr, data);   // LCOV_EXCL_LINE
                                              else 
                                                  state.mem[addr] = data;
                                          };

    // Read from memory---either local, or via externally set method
    inline int         rd_mem             (int addr) {
                                               if (ext_rd_mem != NULL) 
                                                   return ext_rd_mem(addr);   // LCOV_EXCL_LINE
                                               else 
                                                   return state.mem[addr]; 
                                           };
private:
    // Instructions functions for base 6502 implementation
    int                ADC                (const op_t* op);
    int                AND                (const op_t* op);
    int                ASL                (const op_t* op);
    int                BCC                (const op_t* op);
    int                BCS                (const op_t* op);
    int                BEQ                (const op_t* op);
    int                BIT                (const op_t* op);
    int                BMI                (const op_t* op);
    int                BNE                (const op_t* op);
    int                BPL                (const op_t* op);
    int                BRK                (const op_t* op);
    int                BVC                (const op_t* op);
    int                BVS                (const op_t* op);
    int                CLC                (const op_t* op);
    int                CLD                (const op_t* op);
    int                CLI                (const op_t* op);
    int                CLV                (const op_t* op);
    int                CMP                (const op_t* op);                
    int                CPX                (const op_t* op);
    int                CPY                (const op_t* op);
    int                DEC                (const op_t* op);
    int                DEX                (const op_t* op);
    int                DEY                (const op_t* op);
    int                EOR                (const op_t* op);
    int                INC                (const op_t* op);
    int                INX                (const op_t* op);
    int                INY                (const op_t* op);
    int                JMP                (const op_t* op);
    int                JSR                (const op_t* op);
    int                LDA                (const op_t* op);
    int                LDX                (const op_t* op);
    int                LDY                (const op_t* op);
    int                LSR                (const op_t* op);
    int                NOP                (const op_t* op);
    int                ORA                (const op_t* op);       
    int                PHA                (const op_t* op);
    int                PHP                (const op_t* op);
    int                PLA                (const op_t* op);
    int                PLP                (const op_t* op);
    int                ROL                (const op_t* op);
    int                ROR                (const op_t* op);
    int                RTI                (const op_t* op);
    int                RTS                (const op_t* op);
    int                SBC                (const op_t* op);
    int                SEC                (const op_t* op);
    int                SED                (const op_t* op);
    int                SEI                (const op_t* op);
    int                STA                (const op_t* op);
    int                STX                (const op_t* op);
    int                STY                (const op_t* op);
    int                TAX                (const op_t* op);
    int                TAY                (const op_t* op);
    int                TSX                (const op_t* op);
    int                TXA                (const op_t* op);
    int                TXS                (const op_t* op);
    int                TYA                (const op_t* op);

    // Functions for 65C02/WDC65C02 instructions
    int                BBR                (const op_t* op);
    int                BBS                (const op_t* op);
    int                RMB                (const op_t* op);
    int                SMB                (const op_t* op);
    int                BRA                (const op_t* op);
    int                TRB                (const op_t* op);
    int                TSB                (const op_t* op);
    int                STZ                (const op_t* op);
    int                PHX                (const op_t* op);
    int                PHY                (const op_t* op);
    int                PLX                (const op_t* op);
    int                PLY                (const op_t* op);
    int                WAI                (const op_t* op);
    int                STP                (const op_t* op);

// Private member variables
private:

    // Pointers to external memory access methods. When NULL, internal memory used
    wy65_p_writemem_t  ext_wr_mem;
    wy65_p_readmem_t   ext_rd_mem;

    // Disassemble state
    FILE*              fp;
    uint32_t           nextPc;

    // Instruction table entry array
    tbl_t              instr_tbl [WY65_INSTR_SPACE_SIZE]; 

    // CPU state
    wy65_cpu_state_t   state;
};

#endif