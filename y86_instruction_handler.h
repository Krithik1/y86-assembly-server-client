#ifndef Y86_INSTRUCTION_HANDLER_H // Include guard
#define Y86_INSTRUCTION_HANDLER_H

#include <inttypes.h>
#include <cstring>
#include <iostream>
#include <memory> // Include for smart pointers
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>

using namespace std;

#define FLAG_O 0x20
#define FLAG_Z 0x40
#define FLAG_S 0x04

struct y86_state {
    uint8_t memory[1024];
    uint64_t start_addr;
    uint64_t valid_mem;     
    uint64_t registers[16];
    uint64_t pc;
    uint8_t flags; 

    // Constructor
    y86_state(const uint8_t mem[], uint64_t start_addr, uint64_t valid_mem, const uint64_t registers[], uint64_t pc, uint8_t flags) 
        : start_addr(start_addr), valid_mem(valid_mem), pc(pc), flags(flags) {
        // Copy memory array
        std::memcpy(this->memory, mem, sizeof(this->memory));
        // Copy registers array
        std::memcpy(this->registers, registers, sizeof(this->registers));
    }
};

struct y86_inst {
    uint8_t rA;
    uint8_t rB;
    uint64_t constval;
    char instruction[10];

    // Constructor
    y86_inst(uint8_t rA, uint8_t rB, uint64_t constval, const char* instruction) 
        : rA(rA), rB(rB), constval(constval) {
        // Copy instruction string, ensuring null-termination
        strncpy(this->instruction, instruction, sizeof(this->instruction) - 1);
        // Null-terminate the string in case of overflow
        this->instruction[sizeof(this->instruction) - 1] = '\0';
    }
};

enum inst_t {
    I_NOP,
    I_HALT,
    I_RRMOVQ,
    I_IRMOVQ,
    I_RMMOVQ,
    I_MRMOVQ,
    I_PUSHQ,
    I_POPQ,
    I_CALL,
    I_RET,
    I_J,
    I_JEQ,
    I_JNE,
    I_JL,
    I_JLE,
    I_JG,
    I_JGE,
    I_ADDQ,
    I_SUBQ,
    I_MULQ,
    I_MODQ,
    I_DIVQ,
    I_ANDQ,
    I_XORQ,
    I_CMOVEQ,
    I_CMOVNE,
    I_CMOVL,
    I_CMOVLE,
    I_CMOVG,
    I_CMOVGE,
    I_INVALID
};

class y86_instruction_handler {
    private:
        unique_ptr<y86_state> state; // Use smart pointer for state
        unique_ptr<y86_inst> inst;   // Use smart pointer for inst
        inst_t inst_to_enum(char* str);
        void convert_to_inst(string& instruction);
        int read_quad(uint64_t address, uint64_t* value);
        int write_quad(uint64_t address, uint64_t value);
        void update_PC();
        int irmovq();
        int rrmovq();
        int rmmovq();
        int mrmovq();
        int addq();
        int subq();
        int mulq();
        int divq();
        int xorq();
        int andq();
        int modq();
        int cmov(int cc);
        int jmpCond(int cc);
        int pushq();
        int popq();
        int call();
        int ret();
        string dump_state();

    public:
        y86_instruction_handler();
        string handle_instruction(string& instruction);
};

#endif // Y86_INSTRUCTION_HANDLER_H
