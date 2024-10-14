#include "y86_instruction_handler.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <iomanip>

struct cmd_map_t {
    char* cmd_str;
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
    array<uint8_t, 1024> memory = { 0 };
    array<uint64_t, 16> registers = { 0 };

    // Initialize the unique pointer to y86_state
    state = make_unique<y86_state>(
        memory.data(),   
        0,               
        1024,            
        registers.data(),
        0,               
        0                
    );
}

void y86_instruction_handler::convert_to_inst(string& instruction) {
    // Split the instruction into tokens
    vector<string> tokens = split(instruction);

    if (tokens.empty()) {
        throw invalid_argument("Invalid instruction format");
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
        if (tokens.size() < 2 || tokens[1].size() < 2 || tokens[1][0] != 'r') {
            throw invalid_argument("Invalid register in instruction");
        }
        if (tokens[1].size() == 2) {
            rA = stoi(tokens[1].substr(1));
        } else {
            rA = stoi(tokens[1].substr(1, 2));
        }
        inst = make_unique<y86_inst>(rA, 0, 0, inst_name);
    } else if (token.substr(0) == "j") {
        constval = stoull(tokens[1]);
        inst = make_unique<y86_inst>(0, 0, constval, inst_name);
    } else if (token == "rrmovq" || token.substr(0, 4) == "cmov" || token.size() == 4) {
        // Conditional move or register move
        if (tokens.size() < 3 || tokens[1].size() < 2 || tokens[1][0] != 'r') {
            throw invalid_argument("Invalid register in instruction");
        }
        if (tokens[1].size() == 2) {
            rA = stoi(tokens[1].substr(1));
        } else {
            rA = stoi(tokens[1].substr(1, 2));
        }

        if (tokens[2].size() < 2 || tokens[2][0] != 'r') {
            throw invalid_argument("Invalid register in instruction");
        }
        if (tokens[2].size() == 2) {
            rB = stoi(tokens[2].substr(1));
        } else {
            rB = stoi(tokens[2].substr(1, 2));
        }
        
        inst = make_unique<y86_inst>(rA, rB, 0, inst_name); // Use inst_name here
    } else if (token == "irmovq") {
        // Immediate value followed by register
        if (tokens.size() < 3 || tokens[1].empty() || tokens[2].size() < 2 || tokens[2][0] != 'r') {
            throw invalid_argument("Invalid instruction format");
        }
        constval = stoull(tokens[1]);
        if (tokens[2].size() == 2) {
            rB = stoi(tokens[2].substr(1));
        } else {
            rB = stoi(tokens[2].substr(1, 2));
        }
        inst = make_unique<y86_inst>(0, rB, constval, inst_name); // Use inst_name here
    } else if (token == "rmmovq") {
        // Register move with displacement
        if (tokens.size() < 2 || tokens[1].size() < 2 || tokens[1][0] != 'r') {
            throw invalid_argument("Invalid register in instruction");
        }
        if (tokens[1].size() == 2) {
            rA = stoi(tokens[1].substr(1));
        } else {
            rA = stoi(tokens[1].substr(1, 2));
        }

        // D(rB) form
        if (tokens.size() < 3 || tokens[2].size() < 4 || 
            (tokens[2][tokens[2].size() - 4] != '(' && tokens[2][tokens[2].size() - 5] != '(') || 
            (tokens[2][tokens[2].size() - 3] != 'r' && tokens[2][tokens[2].size() - 4] != 'r')) {
            throw invalid_argument("Invalid instruction format");
        }

        // Get the displacement (assumed to be 0 here; modify as needed)
        constval = stoull(tokens[2].substr(0, tokens[2].size() - 4));
        rB = stoi(tokens[2].substr(tokens[2].size() - 2, 1));

        inst = make_unique<y86_inst>(rA, rB, constval, inst_name); // Use inst_name here
    } else if (token == "mrmovq") {
        // Similar to rmmovq
        if (tokens.size() < 3 || tokens[1].size() < 4 || 
            (tokens[1][tokens[1].size() - 4] != '(' && tokens[1][tokens[1].size() - 5] != '(') || 
            (tokens[1][tokens[1].size() - 3] != 'r' && tokens[1][tokens[1].size() - 4] != 'r')) {
            throw invalid_argument("Invalid instruction format");
        }
        if (tokens[1][tokens[1].size() - 4] == '(') {
            constval = stoull(tokens[1].substr(0, tokens[1].size() - 4));
            rB = stoi(tokens[1].substr(tokens[1].size() - 2, 1));
        } else {
            constval = stoull(tokens[1].substr(0, tokens[1].size() - 5));
            rB = stoi(tokens[1].substr(tokens[1].size() - 3, 2));
        }

        if (tokens.size() < 3 || tokens[2].size() < 2 || tokens[2][0] != 'r') {
            throw invalid_argument("Invalid register in instruction");
        }
        if (tokens[2].size() == 2) {
            rA = stoi(tokens[2].substr(1));
        } else {
            rA = stoi(tokens[2].substr(1, 2));
        }

        inst = make_unique<y86_inst>(rA, rB, constval, inst_name); // Use inst_name here
    } else {
        throw invalid_argument("Unknown instruction");
    }
}

string y86_instruction_handler::dump_state() {
    stringstream ss;

    ss << "REGS: ";

    for (uint64_t reg_val : state->registers) {
        ss << "0x" << hex << setw(16) << setfill('0') << reg_val << " ";
    }

    ss << "\n";
    ss << "FLAGS: ";

    switch ((state->flags & 0x64)) {
        case 0:
            ss << "---";
            break;
        case FLAG_O:
            ss << "O--";
            break;
        case FLAG_S:
            ss << "-S-";
            break;
        case FLAG_Z:
            ss << "--Z";
            break;
        case FLAG_O + FLAG_S:
            ss << "OS-";
            break;
        case FLAG_S + FLAG_Z:
            ss << "-SZ";
            break;
        case FLAG_O + FLAG_Z:
            ss << "O-Z";
            break;
        case FLAG_O + FLAG_S + FLAG_Z:
            ss << "OSZ";
            break;
        default:
            break;
    }
    ss << "\n";
    ss << "PC: " << "0x" << hex << setw(16) << setfill('0') << state->pc << "\n";
    ss << "STARTADDR: " << "0x" << hex << setw(16) << setfill('0') << state->start_addr << "\n";
    ss << "VALIDMEM: " << "0x" << hex << setw(16) << setfill('0') << state->valid_mem << "\n";

    return ss.str();
}

int y86_instruction_handler::read_quad(uint64_t address, uint64_t *value) {
	if (address < state->start_addr || address + 8 > state->start_addr + state->valid_mem) {
        return 0;
    }
    uint64_t index = address - state->start_addr;

    memcpy(value, &state->memory[index], 8);

    return 1;
}

int y86_instruction_handler::write_quad(uint64_t address, uint64_t value) {
    if (address < state->start_addr || address + 8 > state->start_addr + state->valid_mem) {
        return 0;
    }
    uint64_t index = address - state->start_addr;
	uint64_t * temp;
	temp = &value;

    memcpy(&state->memory[index],temp, 8);

    return 1;
}

void y86_instruction_handler::update_PC() {
    inst_t enum_inst = inst_to_enum(inst->instruction);
    if (enum_inst == I_NOP) {
		state->pc += 1;
	} else if (enum_inst == I_RRMOVQ || enum_inst == I_CMOVEQ || 
				enum_inst == I_CMOVNE || enum_inst == I_CMOVL || 
				enum_inst == I_CMOVLE || enum_inst == I_CMOVG || 
				enum_inst == I_CMOVGE || enum_inst == I_ADDQ  ||
				enum_inst == I_SUBQ || enum_inst == I_MULQ ||
				enum_inst == I_MODQ || enum_inst == I_DIVQ ||
				enum_inst == I_ANDQ || enum_inst == I_XORQ ||
				enum_inst == I_PUSHQ || enum_inst == I_POPQ) {
		state->pc += 2;
	} else if (enum_inst == I_IRMOVQ || enum_inst == I_RMMOVQ ||
				enum_inst == I_MRMOVQ) {
		state->pc += 10;
	} else if (enum_inst == I_HALT) {
		state->pc = state->pc;
	} else if (enum_inst == I_J) {
		state->pc = inst->constval;
	}
}

int y86_instruction_handler::irmovq() {
    if(inst->rB < 0x0 || inst->rB >= 0xF) {
		return 0;
	}
	state->registers[(int) inst->rB] = inst->constval;
	return 1;
}

int y86_instruction_handler::rrmovq() {
	// Validate Registers
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
	return 1;
}

int y86_instruction_handler::addq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];
	
	int64_t valE = valA + valB;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::subq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];
	
	int64_t valE = valB - valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::mulq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];
	
	int64_t valE = valB * valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::xorq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];
	
	int64_t valE = valB ^ valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::andq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];
	
	int64_t valE = valB & valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::divq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];

	if (valA == 0) {
		return 0;
	}
	
	int64_t valE = valB / valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::modq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	int64_t valA = (int64_t) state->registers[inst->rA];
	int64_t valB = (int64_t) state->registers[inst->rB];

	if ((valA == 0) && (valB == 0)) {
		state->flags = 0x40;
		return 0;
	}
	if (valA == 0) {
		return 1;
	}
	
	int64_t valE = valB % valA;
	state->registers[inst->rB] = valE;

	state->flags = 0;
	if (valE == 0) {
		state->flags |= 0x40;
	}
	if (valE < 0) {
		state->flags |= 0x04;
	}
	return 1;
}

int y86_instruction_handler::cmov(int cc) {
	// Validate Registers
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	switch (cc) {
		case 1:
			if ((state->flags == FLAG_Z) || (state->flags == FLAG_S)) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		case 2:
			if ((state->flags == FLAG_S) && (state->flags != FLAG_Z)) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		case 3:
			if ((state->flags == FLAG_Z) && (state->flags != FLAG_S)) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		case 4:
			if (state->flags != FLAG_Z) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		case 5:
			if ((state->flags == FLAG_Z) || (state->flags != FLAG_S)) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		case 6:
			if ((state->flags != FLAG_S) && (state->flags != FLAG_Z)) {
				state->registers[(int) inst->rB] = state->registers[(int) inst->rA];
			}
			break;
		default:
		{}
	}
	return 1;
}

int y86_instruction_handler::jmpCond(int cc) {

	switch (cc) {
		case 1:
			if ((state->flags == FLAG_Z) || (state->flags == FLAG_S)) {
				state->pc = inst->constval;
			}
			break;
		case 2:
			// printf("Flags: %x\n", state->flags);
			if ((state->flags == FLAG_S) && (state->flags != FLAG_Z)) {
				state->pc = inst->constval;
			}
			break;
		case 3:
			if ((state->flags == FLAG_Z) && (state->flags != FLAG_S)) {
				state->pc = inst->constval;
			}
			break;
		case 4:
			if (state->flags != FLAG_Z) {
				state->pc = inst->constval;
			}
			break;
		case 5:
			if ((state->flags == FLAG_Z) || (state->flags != FLAG_S)) {
				state->pc = inst->constval;
			}
			break;
		case 6:
			if ((state->flags != FLAG_S) && (state->flags != FLAG_Z)) {
				state->pc = inst->constval;
			}
			break;
		default:
		{}
	}
	if (state->pc != inst->constval) {
		state->pc += 9;
	}
	return 1;
}

int y86_instruction_handler::rmmovq(){
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	uint64_t valA = (uint64_t) state->registers[inst->rA];
	uint64_t valB = (uint64_t) state->registers[inst->rB];

	if(!write_quad(valB + (uint64_t) inst->constval, valA)) {
		return 0;
	}
	return 1;
}

int y86_instruction_handler::mrmovq(){
	if (inst->rA < 0x0 || inst->rA >= 0xf || inst->rB < 0x0 || inst->rB >= 0xf) {
		return 0;
	}
	uint64_t valB = (uint64_t) state->registers[inst->rB];

	if(!read_quad(valB + inst->constval, state->registers + inst->rA)) {
		return 0;
	}
	return 1;
}

int y86_instruction_handler::pushq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf) {
		return 0;
	}
	uint64_t valA = (uint64_t) state->registers[inst->rA];
	uint64_t valRSP = (uint64_t) state->registers[4];
	if (valRSP < 8) {
		return 0;
	}
	if (!write_quad(valRSP - 8, valA)) {
		return 0;
	}
	state->registers[4] = valRSP - 8;
	return 1;
}

int y86_instruction_handler::popq() {
	if (inst->rA < 0x0 || inst->rA >= 0xf) {
		return 0;
	}
	uint64_t valRSP = (uint64_t) state->registers[4];
	if (valRSP < 0) {
		return 0;
	}
	if (!read_quad(valRSP, &state->registers[inst->rA])) {
		return 0;
	}
	state->registers[4] = valRSP + 8;
	return 1;
}

int y86_instruction_handler::call() {
	uint64_t valRSP = (uint64_t) state->registers[4];
	state->pc += 9;
	// state->flags = 0;
	// printf("%lx\n", valRSP);
	if (valRSP == 0) {
		state->pc -= 9;
		return 0;
	}
	if (!write_quad(valRSP - 8, state->pc)) {
		state->pc -= 9;
		return 0;
	}
	state->registers[4] = valRSP - 8;
	state->pc = inst->constval;
	return 1;
}

int y86_instruction_handler::ret() {
	uint64_t valRSP = (uint64_t) state->registers[4];
	if (!read_quad(valRSP, &state->pc)) {
		return 0;
	}
	state->registers[4] = valRSP + 8;
	return 1;
}

inst_t y86_instruction_handler::inst_to_enum(char* str) {
    for (int i = 0; i < sizeof(cmd_map)/sizeof(cmd_map[0]); i++) {
		if (strcmp(str, cmd_map[i].cmd_str) == 0) {
			return cmd_map[i].cmd;
		}
	}
	return I_INVALID;
}

string y86_instruction_handler::handle_instruction(string& instruction) {
    stringstream ss;
    if (instruction == "dump") {
        return dump_state();
    }
    try {
        convert_to_inst(instruction);
        if (!inst) {
            throw runtime_error("Instruction not created");
        }
    } catch (const exception& e) {
        return string("Error: ") + e.what(); // Handle error
    }

    inst_t enum_inst = inst_to_enum(inst->instruction);

    if (enum_inst == I_INVALID) {
        return "Error Occured";
    }
    if (enum_inst == I_HALT) {
        return "Halt. Program Ended";
    }
    if (enum_inst == I_IRMOVQ) {
        if (!irmovq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_RRMOVQ) {
        if (!rrmovq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_ADDQ) {
        if (!addq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_SUBQ) {
        if (!subq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_MULQ) {
        if (!mulq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_XORQ) {
        if (!xorq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_ANDQ) {
        if (!andq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_DIVQ) {
        if (!divq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_MODQ) {
        if (!modq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVLE) {
        if (!cmov(1)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVL) {
        if (!cmov(2)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVEQ) {
        if (!cmov(3)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVNE) {
        if (!cmov(4)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVGE) {
        if (!cmov(5)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CMOVG) {
        if (!cmov(6)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JLE) {
        if (!jmpCond(1)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JL) {
        if (!jmpCond(2)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JEQ) {
        if (!jmpCond(3)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JNE) {
        if (!jmpCond(4)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JGE) {
        if (!jmpCond(5)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_JG) {
        if (!jmpCond(6)) {
            return "Error Occured";
        }
    } else if (enum_inst == I_RMMOVQ) {
        if (!rmmovq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_MRMOVQ) {
        if (!mrmovq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_PUSHQ) {
        if (!pushq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_POPQ) {
        if (!popq()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_CALL) {
        if (!call()) {
            return "Error Occured";
        }
    } else if (enum_inst == I_RET) {
        if (!ret()) {
            return "Error Occured";
        }
    }
    update_PC();
    return "Instruction Executed";
}
