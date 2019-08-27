#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <endian.h>
#include <inttypes.h>

#ifdef DEBUG
#define dprintf(f, ...) printf("\n>>> DEBUG: "); printf(f, __VA_ARGS__)
#else
#define dprintf(f, ...)
#endif

#define MAX_ADDR 32767
#define MIN_REG 32768
#define MAX_REG 32775

enum opcode {
    HALT = 0,
    SET,
    PUSH,
    POP,
    EQ,
    GT,
    JMP,
    JT,
    JF,
    ADD,
    MULT,
    MOD,
    AND,
    OR,
    NOT,
    RMEM,
    WMEM,
    CALL,
    RET,
    OUT,
    IN,
    NOOP,
    NUM_OP_CODES
};

uint16_t regs[8] = {0};

void (*op_functions[NUM_OP_CODES])(FILE *) = {0};

void execute_file(FILE *fp);
void not_implemented(enum opcode);

void halt(FILE *fp);
void jmp(FILE *fp);
void out(FILE *fp);
void noop(FILE *fp);

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: $0 <exe>\n");
        return -1;
    }

    FILE *fp;

    if (!(fp = fopen(argv[1], "r"))) {
        perror("");
    }

    op_functions[HALT] = halt;
    op_functions[JMP] = jmp;
    op_functions[OUT] = out;
    op_functions[NOOP] = noop;
    
    execute_file(fp);

    return 0;
}

int is_valid_addr(uint16_t addr)
{
    return addr <= MAX_ADDR;
}

int is_reg(uint16_t addr)
{
    return addr >= MIN_REG && addr <= MAX_REG;
}

uint16_t get_reg_val(uint16_t reg)
{
    if (!is_reg(reg)) {
        fprintf(stderr, "INTERNAL ERROR!Attempting to read a register which doesn't exist!\n");
        exit(1);
    }
    return regs[reg - MIN_REG];
}

int readU16(FILE *fp, uint16_t *ret)
{
    uint16_t n;
    size_t i;

    if ((i = fread(&n, sizeof n, 1, fp)) != 1)
        return -1;

    *ret = le16toh(n);
    return 0;
}

void execute_file(FILE *fp)
{
    uint16_t op;
    while (readU16(fp, &op) != -1) {
        if (op >= NUM_OP_CODES) {
            fprintf(stderr, "ERROR: Op code out of range! Valid codes are from "
            "0 through %u. Offending op code: %u\n", NUM_OP_CODES - 1, op);
            exit(1);
        }
        if (op_functions[op] == 0) {
            not_implemented(op);
        }
        op_functions[op](fp);
    }

    if (feof(fp)) {
        exit(0);
    } else {
        perror("ERROR: Error reading!");
        exit(1);
    }
}

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

void halt(FILE *fp)
{
    puts("*** DEBUG: Halting.");
    exit(0);
}

void not_implemented(enum opcode code) 
{
    printf("*** DEBUG: Opcode number %d (%s) not implemented.\n", code, op_to_string(code));
    exit(0);
}

void out(FILE *fp)
{
    uint16_t ch;
    if (readU16(fp, &ch) == -1) {
        fprintf(stderr, "No argument to 'out'!");
        exit(1);
    }
    putchar(ch);
    dprintf("Calling %s! Character: %c\n", __func__, ch);
}

void noop(FILE *fp)
{
    puts("*** DEBUG: noop");
}

long get_file_num_bytes(FILE *fp)
{
    long size;
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size = ftell(fp) + 1;
    fseek(fp, pos, SEEK_SET);
    return size;
}

void jmp(FILE *fp)
{
    uint16_t addr;
    printf("We are at offset %lu\n", ftell(fp));
    if (readU16(fp, &addr) == -1) {
        fprintf(stderr, "No argument to 'jmp'!");
        exit(1);
    }
    if (is_reg(addr)) {
        addr = get_reg_val(addr);
    } else if (!is_valid_addr(addr)) {
        fprintf(stderr, "ERROR: Address is out of range: %u\n"
            "Address are from 0 through %u\n", addr, MAX_ADDR);
        exit(1);
    } else if (addr * 2 >= get_file_num_bytes(fp)) {
        fprintf(stderr, "Invalid JMP! Address is beyond file...\n");
        exit (1);
    }
    printf("JUMPING to %u\n", addr * 2);
    fseek(fp, addr * 2, SEEK_SET);
    printf("%lu\n", ftell(fp));
}
