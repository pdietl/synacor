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

#ifdef DEBUG
    #define dprintf(f, ...) printf("*** DEBUG (%s): " f, __func__, ## __VA_ARGS__)
    #define dpf() dprintf("\n")
#else
    #define dprintf(f, ...)
    #define dpf()
#endif

#define READ1(var) \
    uint16_t var; \
    if (readU16(&var) == -1) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

#define READ2(var1, var2) \
    uint16_t var1, var2; \
    if (readU16(&var1) == -1 || readU16(&var2) == -1) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

#define READ3(var1, var2, var3) \
    uint16_t var1, var2, var3; \
    if (readU16(&var1) == -1 || readU16(&var2) == -1 || readU16(&var3) == -1 ) { \
        fprintf(stderr, "Not enough arguments to op '%s'!", __func__); \
        exit(1); \
    }

uint16_t memory[MAX_ADDR + 1];
uint16_t mem_offset = 0;

STACK_GENERATE(s, stack, /* func modifier */, uint16_t)

static stack *prog_stack;
static uint16_t regs[REG_NUM] = {0};

void halt(void);
void set(void);
void push(void);
void pop(void);
void eq(void);
void gt(void);
void jmp(void);
void jt(void);
void jf(void);
void add(void);
void mult(void);
void mod(void);
void and(void);
void or(void);
void not(void);
void rmem(void);
void wmem(void);
void call(void);
void ret(void);
void out(void);
void in(void);
void noop(void);

void (*op_functions[NUM_OP_CODES])() = {
    halt,
    set,
    push,
    pop,
    eq,
    gt,
    jmp,
    jt,
    jf,
    add,
    mult,
    mod,
    and,
    or,
    not,
    rmem,
    wmem,
    call,
    ret,
    out,
    in,
    noop
};

void execute_file(void);
void not_implemented(enum opcode);

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <exe>\n", argv[0]);
        exit(1);
    }

    FILE *fp;

    if (!(fp = fopen(argv[1], "r"))) {
        perror("fopen");
        exit(1);
    }

    clearerr(fp);
    fread(memory, sizeof *memory, sizeof memory / sizeof *memory, fp);

    if (ferror(fp)) {
        perror("");
        exit(1);
    }

    fclose(fp);

    if (!(prog_stack = s_new(128))) {
        perror("stack");
        exit(1);
    }

    execute_file();

    return 0;
}

/* Helper functions */

uint16_t get_reg_val(uint16_t reg)
{
    if (!is_reg(reg)) {
        fprintf(stderr, "INTERNAL ERROR: Attempting to read a register which doesn't exist!\n");
        exit(1);
    }
    return regs[reg - MIN_REG];
}

void verify_int_or_die(uint16_t i)
{
    if (i > MAX_INT) {
        fprintf(stderr, "ERROR: Number is out of range: %u\n"
            "Numbers are from 0 through %u\n", i, MAX_INT);
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

void verify_reg_or_int_and_get_val_or_die(uint16_t *i)
{
    if (is_reg(*i))
        *i = get_reg_val(*i);
    else
        verify_int_or_die(*i);
}

void execute_file()
{
    uint16_t op;
    while (readU16(&op) != -1) {
        if (op >= NUM_OP_CODES) {
            fprintf(stderr, "ERROR: Op code out of range! Valid codes are from "
            "0 through %u. Offending op code: %u\n", NUM_OP_CODES - 1, op);
            exit(1);
        }
        if (op_functions[op] == 0) {
            not_implemented(op);
        }
        op_functions[op]();
    }

    if (mem_offset >= MAX_INT) {
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

void halt()
{
    dpf();
    exit(0);
}

void set()
{
    READ2(reg, val)

    verify_reg_or_die(reg);
    verify_reg_or_int_and_get_val_or_die(&val);

    dprintf("Setting register %d to 0x%02x |%c|\n", reg - MIN_REG,
        val, isprint(val) ? val : '.');

    regs[reg - MIN_REG] = val;
}

void push()
{
    READ1(val);
    verify_reg_or_int_and_get_val_or_die(&val);
    s_push(prog_stack, val);

}

void pop()
{
    READ1(dest_reg);
    verify_reg_or_die(dest_reg);

    if (s_empty(prog_stack)) {
        fprintf(stderr, "ERROR: Stack underflow!\n");
        exit(1);
    }

    regs[dest_reg - MIN_REG] = s_top(prog_stack);
    s_pop(prog_stack);
}

void eq()
{
    READ3(dest_reg, val1, val2)
    
    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    verify_reg_or_int_and_get_val_or_die(&val2);

    dprintf("val1: 0x%02x, val2: 0x%02x\n", val1, val2);

    if (val1 == val2) {
        dprintf("Values equal. Setting reg %d to 1\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 1;
    } else {
        dprintf("Values NOT equal. Setting reg %d to 0\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 0;
    }
}

void gt()
{
    READ3(dest_reg, val1, val2)
    
    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    verify_reg_or_int_and_get_val_or_die(&val2);

    dprintf("val1: 0x%02x, val2: 0x%02x\n", val1, val2);

    if (val1 > val2) {
        dprintf("val 1 > val2. Setting reg %d to 1\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 1;
    } else {
        dprintf("val 1 is <= val2. Setting reg %d to 0\n", dest_reg - MIN_REG);
        regs[dest_reg - MIN_REG] = 0;
    }
}

void jmp()
{
    READ1(addr)
    verify_reg_or_int_and_get_val_or_die(&addr);
    
    dprintf("current word-wise file offset: %u\n", mem_offset - 1);
    dprintf("jump addr: %u\n", addr);
    
    mem_offset = addr;
}

void jt()
{
    READ2(boolean, addr)
    
    verify_reg_or_int_and_get_val_or_die(&boolean);
    verify_reg_or_int_and_get_val_or_die(&addr);

    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %u\n", mem_offset - 3);

    if (boolean)
        mem_offset = addr;
}

void jf()
{
    READ2(boolean, addr)
    
    verify_reg_or_int_and_get_val_or_die(&boolean);
    verify_reg_or_int_and_get_val_or_die(&addr);
    
    dprintf("boolean val: '%c'\tjump addr: %u\n", boolean != 0 ? 'T' : 'F', addr);
    dprintf("current word-wise file offset: %u\n", mem_offset - 3);

    if (!boolean)
        mem_offset = addr;
}

void add()
{
    READ3(dest_reg, addend1, addend2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&addend1);
    verify_reg_or_int_and_get_val_or_die(&addend2);
    
    uint16_t sum = (addend1 + addend2) % (MAX_INT + 1);

    dprintf("Setting register %d to (0x%02x + 0x%02x) %% MAX_INT+1 = 0x%02x\n", dest_reg - MIN_REG,
        addend1, addend2, sum);

    regs[dest_reg - MIN_REG] = sum;
}

void mult()
{
    READ3(dest_reg, factor1, factor2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&factor1);
    verify_reg_or_int_and_get_val_or_die(&factor2);
    
    uint16_t product = (factor1 * factor2) % (MAX_INT + 1);

    dprintf("Setting register %d to (0x%02x * 0x%02x) %% MAX_INT+1 = 0x%02x\n", dest_reg - MIN_REG,
        factor1, factor2, product);

    regs[dest_reg - MIN_REG] = product;
}

void mod()
{
    READ3(dest_reg, val1, val2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    verify_reg_or_int_and_get_val_or_die(&val2);
    
    uint16_t res = val1 % val2;

    dprintf("Setting register %d to (0x%02x %% 0x%02x) = 0x%02x\n", dest_reg - MIN_REG,
        val1, val2, res);

    regs[dest_reg - MIN_REG] = res;
}

void and()
{
    READ3(dest_reg, val1, val2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    verify_reg_or_int_and_get_val_or_die(&val2);
    
    uint16_t res = val1 & val2;

    dprintf("Setting register %d to (0x%02x & 0x%02x) = 0x%02x\n", dest_reg - MIN_REG,
        val1, val2, res);

    regs[dest_reg - MIN_REG] = res;
}

void or()
{
    READ3(dest_reg, val1, val2)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    verify_reg_or_int_and_get_val_or_die(&val2);
    
    uint16_t res = val1 | val2;

    dprintf("Setting register %d to (0x%02x | 0x%02x) = 0x%02x\n", dest_reg - MIN_REG,
        val1, val2, res);

    regs[dest_reg - MIN_REG] = res;
}

void not()
{
    READ2(dest_reg, val1)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&val1);
    
    uint16_t res = ~val1 & MAX_INT;

    dprintf("Setting register %d to (~0x%02x %% MAX_INT+1) = 0x%02x\n", dest_reg - MIN_REG,
        val1, res);

    regs[dest_reg - MIN_REG] = res;
}

void rmem()
{
    READ2(dest_reg, addr)

    verify_reg_or_die(dest_reg);
    verify_reg_or_int_and_get_val_or_die(&addr);
    
    uint16_t res = memory[addr];

    dprintf("Setting register %d to value of mem location (0x%02x) = 0x%02x\n", dest_reg - MIN_REG,
        addr, res);

    regs[dest_reg - MIN_REG] = res;
}

void wmem()
{
    READ2(addr, val)

    verify_reg_or_int_and_get_val_or_die(&addr);
    verify_reg_or_int_and_get_val_or_die(&val);
    
    memory[addr] = val;

    dprintf("Setting mem loc %u to value of %02x\n", addr, memory[addr]);
}

void call()
{
    READ1(addr)
    verify_reg_or_int_and_get_val_or_die(&addr);
    
    dprintf("current word-wise file offset: %u\n", mem_offset - 2);
    dprintf("jump addr: %u\n", addr);
    
    s_push(prog_stack, mem_offset);
    mem_offset = addr;
}

void ret()
{
    if (s_empty(prog_stack)) {
        fprintf(stderr, "ERROR: Stack underflow!\n");
        exit(1);
    }

    mem_offset = s_top(prog_stack);
    s_pop(prog_stack);
    
    dprintf("current word-wise file offset: %u\n", mem_offset - 1);
    dprintf("jump addr: %u\n", mem_offset);
}

void out()
{
    READ1(ch)
    verify_reg_or_int_and_get_val_or_die(&ch);
    putchar(ch);
}

void in()
{
    READ1(reg)
    verify_reg_or_die(reg);

    regs[reg - MIN_REG] = getchar();
}

void noop()
{
    dpf();
}

