#include "y86_instruction_handler.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cstdint>

struct cmd_map_t {
    string cmd_str;
    inst_t cmd;
};

struct cmd_map_t cmd_map [30] = {
        { "nop", I_NOP},
        { "halt", I_HALT},
        { "rrmovq", I_RRMOVQ},
        { "irmovq", I_IRMOVQ},
        { "rmmovq", I_RMMOVQ},
        { "mrmovq", I_MRMOVQ},
        { "pushq", I_PUSHQ},
        { "popq", I_POPQ},
        { "call", I_CALL},
        { "ret", I_RET},
        { "jmp", I_J},
        { "je", I_JEQ},
        { "jne", I_JNE},
        { "jl", I_JL},
        { "jle", I_JLE},
        { "jg", I_JG},
        { "jge", I_JGE},
        { "addq", I_ADDQ},
        { "subq", I_SUBQ},
        { "mulq", I_MULQ},
        { "modq", I_MODQ},
        { "divq", I_DIVQ},
        { "andq", I_ANDQ},
        { "xorq", I_XORQ},
        { "cmove", I_CMOVEQ},
        { "cmovne", I_CMOVNE},
        { "cmovl", I_CMOVL},
        { "cmovle", I_CMOVLE},
        { "cmovg", I_CMOVG},
        { "cmovge", I_CMOVGE }
};

vector<string> split(const string& str) {
    vector<string> tokens;
    size_t start = 0, end = 0;

    while ((end = str.find(' ', start)) != string::npos) {
        if (end > start) {
            tokens.push_back(str.substr(start, end - start));
        }
        start = end + 1;
    }
    // Add the last token
    if (start < str.size()) {
        tokens.push_back(str.substr(start));
    }

    return tokens;
}

y86_instruction_handler::y86_instruction_handler() {
    // Initialize the memory and registers
    std::array<unsigned char, 1024> memory = {};
    std::array<long unsigned int, 16> registers = {};

    // Initialize the unique pointer to y86_state
    state = std::make_unique<y86_state>(
        memory.data(),   // Convert std::array to pointer
        0,               // Start address (example)
        1024,            // Valid memory (example)
        registers.data(),// Convert std::array to pointer
        0,               // Program Counter (example)
        0                // Flags (example)
    );
}

void y86_instruction_handler::convert_to_inst(string& instruction) {
    // Split the instruction into tokens
    vector<string> tokens = split(instruction);

    if (tokens.empty()) {
        throw std::invalid_argument("Invalid instruction format");
    }

    // First token is the instruction name
    string token = tokens[0];

    // Initialize registers and constant value
    uint8_t rA = 0, rB = 0;
    uint64_t constval = 0;

    // Check the instruction type and parse accordingly
    const char* inst_name = token.c_str();  // Get the instruction name

    if (token == "halt" || token == "nop") {
        // For instructions with no operands
        inst = make_unique<y86_inst>(0, 0, 0, inst_name);
    } else if (token == "ret") {
        // Similar to halt and nop
        inst = make_unique<y86_inst>(0, 0, 0, inst_name);
    } else if (token == "pushq" || token == "popq") {
        // Push/Pop uses only one register
        if (tokens.size() < 2 || tokens[1].size() != 2 || tokens[1][0] != 'r') {
            throw std::invalid_argument("Invalid register in instruction");
        }
        rA = std::stoi(tokens[1].substr(1));
        inst = make_unique<y86_inst>(rA, 0, 0, inst_name);
    } else if (token == "rrmovq" || token.substr(0, 4) == "cmov") {
        // Conditional move or register move
        if (tokens.size() < 3 || tokens[1].size() != 2 || tokens[1][0] != 'r') {
            throw std::invalid_argument("Invalid register in instruction");
        }
        rA = std::stoi(tokens[1].substr(1));

        if (tokens[2].size() != 2 || tokens[2][0] != 'r') {
            throw std::invalid_argument("Invalid register in instruction");
        }
        rB = std::stoi(tokens[2].substr(1));
        
        inst = make_unique<y86_inst>(rA, rB, 0, inst_name); // Use inst_name here
    } else if (token == "irmovq") {
        // Immediate value followed by register
        if (tokens.size() < 3 || tokens[1].empty() || tokens[2].size() != 2 || tokens[2][0] != 'r') {
            throw std::invalid_argument("Invalid instruction format");
        }
        constval = std::stoull(tokens[1]);
        rB = std::stoi(tokens[2].substr(1));
        inst = make_unique<y86_inst>(0, rB, constval, inst_name); // Use inst_name here
    } else if (token == "rmmovq") {
        // Register move with displacement
        if (tokens.size() < 2 || tokens[1].size() != 2 || tokens[1][0] != 'r') {
            throw std::invalid_argument("Invalid register in instruction");
        }
        rA = std::stoi(tokens[1].substr(1));

        // D(rB) form
        if (tokens.size() < 3 || tokens[2].size() < 4 || tokens[2][tokens[2].size() - 3] != '(' || tokens[2][tokens[2].size() - 1] != 'r') {
            throw std::invalid_argument("Invalid instruction format");
        }

        // Get the displacement (assumed to be 0 here; modify as needed)
        constval = std::stoull(tokens[2].substr(0, tokens[2].size() - 3));
        rB = std::stoi(tokens[2].substr(tokens[2].size() - 2, 1));

        inst = make_unique<y86_inst>(rA, rB, constval, inst_name); // Use inst_name here
    } else if (token == "mrmovq") {
        // Similar to rmmovq
        if (tokens.size() < 2 || tokens[1].size() < 4 || tokens[1][tokens[1].size() - 3] != '(' || tokens[1][tokens[1].size() - 1] != 'r') {
            throw std::invalid_argument("Invalid instruction format");
        }
        constval = std::stoull(tokens[1].substr(0, tokens[1].size() - 3));
        rB = std::stoi(tokens[1].substr(tokens[1].size() - 2, 1));

        if (tokens.size() < 3 || tokens[2].size() != 2 || tokens[2][0] != 'r') {
            throw std::invalid_argument("Invalid register in instruction");
        }
        rA = std::stoi(tokens[2].substr(1));

        inst = make_unique<y86_inst>(rA, rB, constval, inst_name); // Use inst_name here
    } else {
        throw std::invalid_argument("Unknown instruction");
    }
}

string y86_instruction_handler::handle_instruction(string& instruction) {
    try {
        convert_to_inst(instruction);
        if (inst) {
            return inst->instruction; // Ensure inst is valid
        } else {
            throw std::runtime_error("Instruction not created");
        }
    } catch (const std::exception& e) {
        return string("Error: ") + e.what(); // Handle error
    }
}
