#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include <stddef.h>

typedef struct lnode_t {
    char *name;
    int address;
    struct lnode_t *next;
} lnode_t;

typedef struct lnum_t {
    char *fname;
    int num;
    int start;
    int stop;
    struct lnum_t *next;
} lnum_t;

typedef struct {
    size_t length;          // length of the executable
    int *code;              // array of op codes
    int entry;              // entry point of the program
    struct lnode_t *labels; // label table
    struct lnum_t *lnums;   // line num table
} executable_t;

executable_t *vm_load(char **fn);
void vm_unload(executable_t *e);

#endif
