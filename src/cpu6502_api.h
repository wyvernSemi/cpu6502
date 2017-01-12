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
// $Id: cpu6502_api.h,v 1.2 2017/01/12 10:02:55 simon Exp $
// $Source: /home/simon/CVS/src/cpu/cpu6502/src/cpu6502_api.h,v $
//
//=============================================================

#ifndef _CPU6502_API_H_
#define _CPU6502_API_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdint.h>

// -------------------------------------------------------------------------
// DEFINES (override-able)
// -------------------------------------------------------------------------

// Allow internal memory size to be overridden at compile time, with a 
// default size of 65536 (the maximum addressable space).
#ifndef WY65_MEM_SIZE
#define WY65_MEM_SIZE                 0x10000
#endif

// Define WY65_INDIRECT_FIX to fix the NMOS 6502 indirection bug (i.e to be like 65C02)
// #define WY65_INDIRECT_FIX

// Define WY65_STANDALONE to add top level main() test function to drive model.
// #define WY65_STANDALONE

// -------------------------------------------------------------------------
// DEFINES (non-override-able)
// -------------------------------------------------------------------------

// The 8 bit architecture give a maximum of 256 possible opcodes.
#define WY65_INSTR_SPACE_SIZE         256

// -------------------------------------------------------------------------
// TYPE DEFINITIONS
// -------------------------------------------------------------------------

// Structure for return status of wy65_execute() function
typedef struct 
{
    uint16_t          pc;      // PC value *after* instruction
    uint32_t          cycles;  // Cycle count of instruction execution

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
    uint8_t           mem[WY65_MEM_SIZE];
    wy65_reg_t        regs;
    uint32_t          cycles;
    uint16_t          nirq_line;

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
    
    } tbl_t; 

// Public methods
public:

    // Constructor
                       cpu6502(); 

    // Reset function. Also clears cycle count and any active IRQ lines.
    void               reset              (void);

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

    // Get copy of processor mode state
    wy65_cpu_state_t   get_state          (void);

    // Register external memory functions for use in memory read/write accesses,
    // to allow interfacing with external memory system.
    void               register_mem_funcs (wy65_p_writemem_t p_wfunc, wy65_p_readmem_t  p_rfunc);

    // Read Binary, Intel HEX or Motorola S-Record files into memory. Call *after* 
    // register_mem_funcs(), if this is used.
    int                read_bin           (const char *filename, const uint16_t start_addr = 0);
    int                read_ihx           (const char *filename);
    int                read_srec          (const char *filename);

// Private member functions
private:
    // Internal check and execution of maskable interrupts
    void               irq                (void);

    // Disassemble opcode to logfile
    void               disassemble        (const int      opcode, 
                                           const uint16_t pc, 
                                           const uint32_t cycles, 
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
    inline void        set_tbl_entry      (tbl_t &t, char* s, pInstrFunc_t f, uint32_t c, addr_mode_e m) {
                                              t.op_str      =  s;
                                              t.pFunc       =  f;
                                              t.exec_cycles =  c;
                                              t.addr_mode   =  m;
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
                                                  ext_wr_mem(addr, data); 
                                              else 
                                                  state.mem[addr] = data;
                                          };

    // Read from memory---either local, or via externally set method
    inline int         rd_mem             (int addr) {
                                               if (ext_rd_mem != NULL) 
                                                   return ext_rd_mem(addr); 
                                               else 
                                                   return state.mem[addr]; 
                                           };
private:
    // Instructions functions
    int                ADC                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X
    int                AND                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X
    int                ASL                (const op_t* op); // - Absolute; Absolute,X; Accumulator; Zero Page; Zero Page,X                  
    int                BCC                (const op_t* op);
    int                BCS                (const op_t* op);
    int                BEQ                (const op_t* op);
    int                BIT                (const op_t* op); // - Absolute; Zero Page
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
    int                CMP                (const op_t* op); // - (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X; (Indirect@,Y                   
    int                CPX                (const op_t* op); // - Absolute; Immediate; Zero Page
    int                CPY                (const op_t* op); // - Absolute; Immediate; Zero Page            
    int                DEC                (const op_t* op); // - Absolute; Absolute,X; Zero Page; Zero Page,X                    
    int                DEX                (const op_t* op);          
    int                DEY                (const op_t* op);        
    int                EOR                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X 
    int                INC                (const op_t* op); // - Absolute; Absolute,X; Zero Page; Zero Page,X
    int                INX                (const op_t* op);
    int                INY                (const op_t* op);
    int                JMP                (const op_t* op); // - Absolute; Indirect
    int                JSR                (const op_t* op);
    int                LDA                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X
    int                LDX                (const op_t* op); // - Absolute; Absolute,Y; Immediate; Zero Page; Zero Page,Y
    int                LDY                (const op_t* op); // - Absolute; Absolute,X; Immediate; Zero Page; Zero Page,X
    int                LSR                (const op_t* op); // - Absolute; Absolute,X; Accumulator; Zero Page; Zero Page,X          
    int                NOP                (const op_t* op);
    int                ORA                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X          
    int                PHA                (const op_t* op);
    int                PHP                (const op_t* op);
    int                PLA                (const op_t* op);
    int                PLP                (const op_t* op); //
    int                ROL                (const op_t* op); // - Absolute; Absolute,X; Accumulator; Zero Page; Zero Page,X
    int                ROR                (const op_t* op); // - Absolute; Absolute,X; Accumulator; Zero Page; Zero Page,X         
    int                RTI                (const op_t* op);
    int                RTS                (const op_t* op);
    int                SBC                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Immediate; Zero Page; Zero Page,X
    int                SEC                (const op_t* op);
    int                SED                (const op_t* op);
    int                SEI                (const op_t* op);
    int                STA                (const op_t* op); // - (Indirect),Y; (Indirect,X); Absolute; Absolute,X; Absolute,Y; Zero Page; Zero Page,X          
    int                STX                (const op_t* op); // - Absolute; Zero Page; Zero Page,Y                   
    int                STY                (const op_t* op); // - Absolute; Zero Page; Zero Page,X          
    int                TAX                (const op_t* op);
    int                TAY                (const op_t* op);
    int                TSX                (const op_t* op);
    int                TXA                (const op_t* op);         
    int                TXS                (const op_t* op);       
    int                TYA                (const op_t* op);

// Private member variables
private:

    // Pointers to external memory access methods. When NULL, internal memory used
    wy65_p_writemem_t  ext_wr_mem;
    wy65_p_readmem_t   ext_rd_mem;

    // CPU state
    wy65_cpu_state_t   state;

    // Disassemble state
    FILE*              fp;
    uint32_t           nextPc;

    // Instruction table entry array, and current selected entry
    tbl_t              instr_tbl [WY65_INSTR_SPACE_SIZE];
    tbl_t              curr_instr;
};

#endif