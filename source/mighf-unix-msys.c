/*
    Created and designed by:

 ________  ________  ___  __        ___    ___ _____ ______   ________  ________     
|\   __  \|\   __  \|\  \|\  \     |\  \  /  /|\   _ \  _   \|\   __  \|\   ____\    
\ \  \|\  \ \  \|\  \ \  \/  /|_   \ \  \/  / | \  \\\__\ \  \ \  \|\  \ \  \___|    
 \ \  \\\  \ \   __  \ \   ___  \   \ \    / / \ \  \\|__| \  \ \   __  \ \  \       
  \ \  \\\  \ \  \ \  \ \  \\ \  \   \/  /  /   \ \  \    \ \  \ \  \ \  \ \  \____  
   \ \_______\ \__\ \__\ \__\\ \__\__/  / /      \ \__\    \ \__\ \__\ \__\ \_______\
    \|_______|\|__|\|__|\|__| \|__|\___/ /        \|__|     \|__|\|__|\|__|\|_______|
                                  \|___|/                                            
                                                                                     
                                                                                     
    DEVELOPER NOTES:
        This program was coded thinking of POSIX-compliant systems, non POSIX OSes will need a compatibility layer.
        On Windows, I recommend using MSYS2 to compile it.
        On systems you may need to patch the code, like on Classic MacOS or on RiscOS.
        This code was from my original Gist.
        On case of using this code, please credit me (OakyMac, or just put "r/OakyMac") as the original author.
    LICENSE:
        This code is licensed under the MIT License.
    
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define MEM_SIZE 1024
#define REG_COUNT 8
#define MAX_LINE 128

// Registers: R0-R7
uint32_t regs[REG_COUNT];
uint8_t memory[MEM_SIZE];
uint32_t pc = 0; // Program counter
uint8_t running = 1;

// Simple instruction set
typedef enum {
    NOP,    // 0
    MOV,    // 1: MOV reg, imm
    ADD,    // 2: ADD reg1, reg2
    SUB,    // 3: SUB reg1, reg2
    LOAD,   // 4: LOAD reg, addr
    STORE,  // 5: STORE reg, addr
    JMP,    // 6: JMP addr
    CMP,    // 7: CMP reg1, reg2
    JE,     // 8: JE addr
    HALT    // 9
} Instr;

typedef struct {
    Instr opcode;
    uint8_t op1;
    uint8_t op2;
    uint32_t imm;
} Instruction;

// Simple program memory
Instruction program[MEM_SIZE];

// Flags
uint8_t flag_zero = 0;

// Micro-architecture: fetch-decode-execute
void execute_instruction(Instruction *inst) {
    switch (inst->opcode) {
        case NOP:
            break;
        case MOV:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = inst->imm;
            break;
        case ADD:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] += regs[inst->op2];
            break;
        case SUB:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] -= regs[inst->op2];
            break;
        case LOAD:
            if (inst->op1 < REG_COUNT && inst->imm < MEM_SIZE)
                regs[inst->op1] = memory[inst->imm];
            break;
        case STORE:
            if (inst->op1 < REG_COUNT && inst->imm < MEM_SIZE)
                memory[inst->imm] = regs[inst->op1] & 0xFF;
            break;
        case JMP:
            if (inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
        case CMP:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                flag_zero = (regs[inst->op1] == regs[inst->op2]);
            break;
        case JE:
            if (flag_zero && inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
        case HALT:
            running = 0;
            break;

        // --- AArch64-inspired instructions ---
        // AND reg1, reg2
        case 10: // AND
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] &= regs[inst->op2];
            break;
        // ORR reg1, reg2
        case 11: // ORR
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] |= regs[inst->op2];
            break;
        // EOR reg1, reg2
        case 12: // EOR
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] ^= regs[inst->op2];
            break;
        // LSL reg1, imm
        case 13: // LSL
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] <<= inst->imm;
            break;
        // LSR reg1, imm
        case 14: // LSR
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] >>= inst->imm;
            break;
        // MUL reg1, reg2
        case 15: // MUL
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] *= regs[inst->op2];
            break;
        // UDIV reg1, reg2
        case 16: // UDIV
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT && regs[inst->op2] != 0)
                regs[inst->op1] /= regs[inst->op2];
            break;
        // NEG reg1
        case 17: // NEG
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = -regs[inst->op1];
            break;
        // MOVZ reg, imm (move zero-extended immediate)
        case 18: // MOVZ
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = (uint16_t)inst->imm;
            break;
        // MOVN reg, imm (move NOT of immediate)
        case 19: // MOVN
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = ~(inst->imm);
            break;
        default:
            break;
    }
}

// Simple assembler for demo
int assemble(const char *line, Instruction *inst) {
    char op[16], arg1[16], arg2[16];
    int n = sscanf(line, "%15s %15s %15s", op, arg1, arg2);
    if (n < 1) return 0;
    if (strcmp(op, "NOP") == 0) { inst->opcode = NOP; }
    else if (strcmp(op, "MOV") == 0 && n == 3) {
        inst->opcode = MOV;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "ADD") == 0 && n == 3) {
        inst->opcode = ADD;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "SUB") == 0 && n == 3) {
        inst->opcode = SUB;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "LOAD") == 0 && n == 3) {
        inst->opcode = LOAD;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "STORE") == 0 && n == 3) {
        inst->opcode = STORE;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "JMP") == 0 && n == 2) {
        inst->opcode = JMP;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "CMP") == 0 && n == 3) {
        inst->opcode = CMP;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "JE") == 0 && n == 2) {
        inst->opcode = JE;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "HALT") == 0) { inst->opcode = HALT; }

    // --- New instructions ---
    else if (strcmp(op, "AND") == 0 && n == 3) {
        inst->opcode = 10;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "ORR") == 0 && n == 3) {
        inst->opcode = 11;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "EOR") == 0 && n == 3) {
        inst->opcode = 12;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "LSL") == 0 && n == 3) {
        inst->opcode = 13;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "LSR") == 0 && n == 3) {
        inst->opcode = 14;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MUL") == 0 && n == 3) {
        inst->opcode = 15;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "UDIV") == 0 && n == 3) {
        inst->opcode = 16;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "NEG") == 0 && n == 2) {
        inst->opcode = 17;
        inst->op1 = arg1[1] - '0';
    }
    else if (strcmp(op, "MOVZ") == 0 && n == 3) {
        inst->opcode = 18;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MOVN") == 0 && n == 3) {
        inst->opcode = 19;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else return 0;
    return 1;
}

// UEFI Shell-like shell
void shell() {
    char line[MAX_LINE];
    printf("Welcome to mighf-embedded micro-arch shell!\n");
#if defined(__linux__)
    printf("Host platform: Linux (%s)\n", 
#if defined(__x86_64__)
        "x86_64"
#elif defined(__aarch64__)
        "aarch64"
#elif defined(__arm__)
        "arm"
#elif defined(__i386__)
        "i386"
#else
        "unknown"
#endif
    );
#elif defined(_WIN32)
    printf("Host platform: Windows\n");
#elif defined(__APPLE__)
    printf("Host platform: macOS\n");
#else
    printf("Host platform: Unknown\n");
#endif
    printf("Type 'help' for commands.\n");
    while (1) {
        printf("coreshell> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;
        else if (strncmp(line, "help", 4) == 0) {
            printf("Commands:\n");
            printf("  load <file>   - Load program\n");
            printf("  run           - Run program\n");
            printf("  regs          - Show registers\n");
            printf("  mem <addr>    - Show memory at addr\n");
            printf("  exit          - Exit shell\n");
        }
        else if (strncmp(line, "regs", 4) == 0) {
            for (int i = 0; i < REG_COUNT; i++)
                printf("R%d: %u\n", i, regs[i]);
        }
        else if (strncmp(line, "mem", 3) == 0) {
            int addr = atoi(line + 4);
            if (addr >= 0 && addr < MEM_SIZE)
                printf("MEM[%d]: %u\n", addr, memory[addr]);
            else
                printf("Invalid address\n");
        }
        else if (strncmp(line, "load", 4) == 0) {
            char fname[64];
            if (sscanf(line, "load %63s", fname) == 1) {
                FILE *f = fopen(fname, "r");
                if (!f) { printf("Cannot open file\n"); continue; }
                char pline[MAX_LINE];
                int idx = 0;
                while (fgets(pline, sizeof(pline), f) && idx < MEM_SIZE) {
                    if (assemble(pline, &program[idx]))
                        idx++;
                }
                fclose(f);
                printf("Loaded %d instructions\n", idx);
            } else {
                printf("Usage: load <file>\n");
            }
        }
        else if (strncmp(line, "run", 3) == 0) {
            pc = 0;
            running = 1;
            while (running && pc < MEM_SIZE) {
                execute_instruction(&program[pc]);
                pc++;
            }
            printf("Program finished.\n");
        }
        else {
            printf("Unknown command. Type 'help'.\n");
        }
    }
}

int main() {
    memset(regs, 0, sizeof(regs));
    memset(memory, 0, sizeof(memory));
    memset(program, 0, sizeof(program));
    shell();
    return 0;
}