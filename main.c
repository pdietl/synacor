#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <endian.h>
#include <inttypes.h>
#include <ctype.h>
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

uint16_t regs[REG_NUM] = {0};

void (*op_functions[NUM_OP_CODES])(FILE *) = {0};

void execute_file(FILE *fp);
void not_implemented(enum opcode);

void halt(FILE *fp);
void set(FILE *fp);
void jmp(FILE *fp);
void jt(FILE *fp);
void jf(FILE *fp);
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
        perror("");
    }

    op_functions[HALT] = halt;
    op_functions[SET] = set;
    op_functions[JMP] = jmp;
    op_functions[JT] = jt;
    op_functions[JF] = jf;
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

void verify_reg_mem_or_die_and_get_val(uint16_t *addr, FILE *fp)
{
    if (is_reg(*addr)) {
        *addr = get_reg_val(*addr);
    } else if (!is_valid_addr(*addr)) {
        fprintf(stderr, "ERROR: Address is out of range: %u\n"
            "Address are from 0 through %u\n", *addr, MAX_ADDR);
        exit(1);
    } else if (*addr * 2 >= get_file_num_bytes(fp)) {
        fprintf(stderr, "Invalid Address! Address is beyond file!\n");
        exit (1);
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
    printf("*** DEBUG: Opcode number %d (%s) not implemented.\n", code, op_to_string(code));
    exit(0);
}

void halt(FILE *fp)
{
    dpf();
    exit(0);
}

void set(FILE *fp)
{
    uint16_t reg, val;
    if (readU16(fp, &reg) == -1 || readU16(fp, &val) == -1) {
        fprintf(stderr, "No arguments to 'set'!");
        exit(1);
    }

    verify_reg_or_die(reg);
    verify_int_or_die(val);

    dprintf("Setting register %d to 0x%02x |%c|\n", reg - MIN_REG,
        val, isprint(val) ? val : '.');

    regs[reg - MIN_REG] = val;
}

void jmp(FILE *fp)
{
    uint16_t addr;
    if (readU16(fp, &addr) == -1) {
        fprintf(stderr, "No argument to 'jmp'!");
        exit(1);
    }
    verify_reg_mem_or_die_and_get_val(&addr, fp);
    fseek(fp, addr * 2, SEEK_SET);
}

void jt(FILE *fp)
{
    uint16_t boolean, addr;
    if (readU16(fp, &boolean) == -1 || readU16(fp, &addr) == -1) {
        fprintf(stderr, "No arguments to 'jt'!");
        exit(1);
    }
    verify_reg_mem_or_die_and_get_val(&boolean, fp);
    verify_reg_mem_or_die_and_get_val(&addr, fp);

    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %ld\n", ftell(fp) / 2 - 3);

    if (boolean)
        fseek(fp, addr * 2, SEEK_SET);
}

void jf(FILE *fp)
{
    uint16_t boolean, addr;
    if (readU16(fp, &boolean) == -1 || readU16(fp, &addr) == -1) {
        fprintf(stderr, "No arguments to 'jf'!");
        exit(1);
    }
    verify_reg_mem_or_die_and_get_val(&boolean, fp);
    verify_reg_mem_or_die_and_get_val(&addr, fp);
    
    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %ld\n", ftell(fp) / 2 - 3);

    if (!boolean)
        fseek(fp, addr * 2, SEEK_SET);
}

void out(FILE *fp)
{
    uint16_t ch;
    if (readU16(fp, &ch) == -1) {
        fprintf(stderr, "No argument to 'out'!");
        exit(1);
    }
    putchar(ch);
}

void noop(FILE *fp)
{
    dpf();
}

