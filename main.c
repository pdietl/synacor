#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <endian.h>
#include <inttypes.h>
#include <ctype.h>
#include "cmc/stack.h"
#include "arch.h"

#define MAX_ADDR MAX_INT
#define REG_NUM  8

#define DEBUG

#ifdef DEBUG
    #define dprintf(f, ...) printf("*** DEBUG (%s): " f, __func__, ## __VA_ARGS__)
    #define dpf() dprintf("\n")
#else
    #define dprintf(f, ...)
    #define dpf()
#endif

#define READ1(var) \
    uint16_t var; \
    if (readU16(fp, &var)) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

#define READ2(var1, var2) \
    uint16_t var1, var2; \
    if (readU16(fp, &var1) == -1 || readU16(fp, &var2) == -1) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

#define READ3(var1, var2, var3) \
    uint16_t var1, var2, var3; \
    if (readU16(fp, &var1) == -1 || readU16(fp, &var2) == -1 || \
        readU16(fp, &var3) == -1 ) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

STACK_GENERATE(s, stack, /* func modifier */, uint16_t)

static stack *prog_stack;
static uint16_t regs[REG_NUM] = {0};

void (*op_functions[NUM_OP_CODES])(FILE *) = {0};

void execute_file(FILE *fp);
void not_implemented(enum opcode);

void halt(FILE *fp);
void set(FILE *fp);
void push(FILE *fp);
void pop(FILE *fp);
void eq(FILE *fp);
void jmp(FILE *fp);
void jt(FILE *fp);
void jf(FILE *fp);
void add(FILE *fp);
void out(FILE *fp);
void noop(FILE *fp);

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <exe>\n", argv[0]);
        return 1;
    }

    FILE *fp;

    if (!(fp = fopen(argv[1], "r"))) {
        perror("fopen");
        abort();
    }

    if (!(prog_stack = s_new(128))) {
        perror("stack");
        abort();
    }

    op_functions[HALT] = halt;
    op_functions[SET] = set;
    op_functions[PUSH] = push;
    op_functions[POP] = pop;
    op_functions[EQ] = eq;
    op_functions[JMP] = jmp;
    op_functions[JT] = jt;
    op_functions[JF] = jf;
    op_functions[ADD] = add;
    op_functions[OUT] = out;
    op_functions[NOOP] = noop;

    execute_file(fp);

    return 0;
}

/* Helper functions */

int is_valid_addr(uint16_t addr)
{
    return addr <= MAX_ADDR;
}

uint16_t get_reg_val(uint16_t reg)
{
    if (!is_reg(reg)) {
        fprintf(stderr, "INTERNAL ERROR: Attempting to read a register which doesn't exist!\n");
        exit(1);
    }
    return regs[reg - MIN_REG];
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

void verify_int_or_die(uint16_t i)
{
    if (i > MAX_ADDR) {
        fprintf(stderr, "ERROR: Number is out of range: %u\n"
            "Numbers are from 0 through %u\n", i, MAX_ADDR);
        exit(1);
    }
}

void verify_reg_or_die(uint16_t addr)
{
    if (!is_reg(addr)) {
        fprintf(stderr, "ERROR: Address expected to be a register, "
            "but its value is out of range! The accused: 0x%02x\n", addr);
        exit(1);
    }
}

void verify_reg_or_int_and_get_val_or_die(uint16_t *addr, FILE *fp)
{
    if (is_reg(*addr)) {
        *addr = get_reg_val(*addr);
    } else if (!is_valid_addr(*addr)) {
        fprintf(stderr, "ERROR: Address is out of range: %u\n"
            "Address are from 0 through %u\n", *addr, MAX_ADDR);
        exit(1);
    } else if (*addr * 2 >= get_file_num_bytes(fp)) {
        fprintf(stderr, "Invalid Address! Address is beyond file!\n");
        exit(1);
    }
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

/* op code implementations */

void not_implemented(enum opcode code) 
{
    dprintf("Opcode number %d (%s) not implemented.\n", code, op_to_string(code));
    exit(0);
}

void halt(FILE *fp)
{
    dpf();
    exit(0);
}

void set(FILE *fp)
{
    READ2(reg, val)

    verify_reg_or_die(reg);
    verify_int_or_die(val);

    dprintf("Setting register %d to 0x%02x |%c|\n", reg - MIN_REG,
        val, isprint(val) ? val : '.');

    regs[reg - MIN_REG] = val;
}

void push(FILE *fp)
{
    READ1(val);
    verify_reg_or_int_and_get_val_or_die(&val, fp);
    s_push(prog_stack, val);

}

void pop(FILE *fp)
{
    READ1(dest_reg);
    verify_reg_or_die(dest_reg);

    if (s_empty(prog_stack)) {
        fprintf(stderr, "ERROR: Stack underflow!\n");
        abort();
    }

    regs[dest_reg - MIN_REG] = s_top(prog_stack);
    s_pop(prog_stack);
}

void eq(FILE *fp)
{
    READ3(dest_reg, val1, val2)
    
    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1, fp);
    verify_reg_or_int_and_get_val_or_die(&val2, fp);

    dprintf("val1: 0x%02x, val2: 0x%02x\n", val1, val2);

    if (val1 == val2) {
        dprintf("Values equal. Setting reg %d to 1\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 1;
    } else {
        dprintf("Values NOT equal. Setting reg %d to 0\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 0;
    }
}

void jmp(FILE *fp)
{
    READ1(addr)
    verify_reg_or_int_and_get_val_or_die(&addr, fp);
    fseek(fp, addr * 2, SEEK_SET);
}

void jt(FILE *fp)
{
    READ2(boolean, addr)
    
    verify_reg_or_int_and_get_val_or_die(&boolean, fp);
    verify_reg_or_int_and_get_val_or_die(&addr, fp);

    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %ld\n", ftell(fp) / 2 - 3);

    if (boolean)
        fseek(fp, addr * 2, SEEK_SET);
}

void jf(FILE *fp)
{
    READ2(boolean, addr)
    
    verify_reg_or_int_and_get_val_or_die(&boolean, fp);
    verify_reg_or_int_and_get_val_or_die(&addr, fp);
    
    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %ld\n", ftell(fp) / 2 - 3);

    if (!boolean)
        fseek(fp, addr * 2, SEEK_SET);
}

void add(FILE *fp)
{
    READ3(dest_reg, addend1, addend2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&addend1, fp);
    verify_reg_or_int_and_get_val_or_die(&addend2, fp);
    
    uint16_t sum = (addend1 + addend2) % (MAX_INT + 1);

    dprintf("Setting register %d to (0x%02x + 0x%02x) %% MAX_INT+1 = 0x%02x\n", dest_reg - MIN_REG,
        addend1, addend2, sum);

    regs[dest_reg - MIN_REG] = sum;
}

void out(FILE *fp)
{
    READ1(ch)
    putchar(ch);
}

void noop(FILE *fp)
{
    dpf();
}

