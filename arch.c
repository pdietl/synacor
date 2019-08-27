#include <stdio.h>
#include <stdlib.h>
#include <byteswap.h>
#include <stdint.h>
#include "arch.h"

const char *op_to_string(enum opcode op)
{
    switch (op) {
        case HALT: return "halt";
        case SET: return "set";
        case PUSH: return "push";
        case POP: return "pop";
        case EQ: return "eq";
        case GT: return "gt";
        case JMP: return "jmp";
        case JT: return "jt";
        case JF: return "jf";
        case ADD: return "add";
        case MULT: return "mult";
        case MOD: return "mod";
        case AND: return "and";
        case OR: return "or";
        case NOT: return "not";
        case RMEM: return "rmem";
        case WMEM: return "wmem";
        case CALL: return "call";
        case RET: return "ret";
        case OUT: return "out";
        case IN: return "in";
        case NOOP: return "noop";
        default:
            fprintf(stderr, "INTERNAL ERROR in %s! Unrecognized opcode: %d\n",
                __func__, op);
            exit(1);
    }
}

bool is_valid_int(uint16_t n)
{
    return n <= MAX_INT;
}

bool is_reg(uint16_t addr)
{
    return addr >= MIN_REG && addr <= MAX_REG;
}


int readU16(uint16_t *ret)
{
    extern uint16_t memory[];
    extern uint16_t mem_offset;

    if (mem_offset >= MAX_INT)
        return -1;

    *ret = le16toh(memory[mem_offset++]);
    return 0;
}

int addr_to_reg_num(uint16_t addr)
{
    if (!is_reg(addr))
        return -1;
    return addr - MIN_REG;
}
