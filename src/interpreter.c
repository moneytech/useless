#include "ops.h"
#include "interpreter.h"
#include "vm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int tok_start = BP;
static char *tokens[RAM_SZ];
static int tok_cnt = BP;

// ========================================================================
//                     LINE NUMBER FUNCTIONS 
// ========================================================================

// this function adds a new line num struct to the executable
void add_line(executable_t *exec, char *fname, int lno, int start, int end) {
    lnum_t *ln = malloc(sizeof(lnum_t));

    char *fn = strdup(fname);
    ln->fname = fn;
    ln->num = lno;
    ln->start = start;
    ln->stop = end;

    lnum_t *head = exec->lnums;
    
    exec->lnums = ln;
    ln->next = head;
}

// traverses the lnum linked list to determine the addr range of a line
lnum_t *get_lnum(executable_t *exec, char *fn, int lno) {
    lnum_t *current = exec->lnums;

    while (current && !(strcmp(current->fname, fn) == 0 && current->num == lno))
        current = current->next;
    
    return current;
}

// ========================================================================
//                        LABEL RELATED FUNCTIONS 
// ========================================================================

// traverses the label linked list to determine if a word in the asm
// is a label. if it is, the corresponding address is returned, otherwise 0
int is_label(char *word, executable_t *exec) {
    lnode_t *current = exec->labels;

    while (current && strcmp(current->name, word) != 0)
        current = current->next;

    return (current) ? current->address : 0;
}

// this function adds a new lbl to the executable mapped to the provided address
void add_label(executable_t *exec, char *name, int addr) {
    lnode_t *lbl = malloc(sizeof(lnode_t));

    char *lbl_name = strdup(++name);
    lbl->name = lbl_name;
    lbl->address = addr;

    lnode_t *head = exec->labels;
    
    exec->labels = lbl;
    lbl->next = head;
}

// ========================================================================
//                         COMPILATION FUNCTIONS 
// ========================================================================

// goes through a single line and saves tokens delimited by whitespace
// the tokens are stored in a static array of strings on the heap
// NOTE: labels are mapped in this stage, and the entry of the program in found
int tokenize(executable_t *exec, char *line) {
    int in_word = 0;
    char *token;

    while (*line != '\0' && *line != '~') {
        if (!in_word) {
            if (!(isspace(*line))) {
                token = line;
                in_word = 1;
            }
        }
        else {
            if (isspace(*line)) {
                *line = '\0';
                
                if (*token == '$') 
                    add_label(exec, token, tok_cnt + 1);   

                else if (!strcmp(token, "entry"))
                    exec->entry = tok_cnt + 1;

                else 
                    tokens[tok_cnt++] = strdup(token); 

                in_word = 0;
            }
        }

        line++;
    }
    
    return 0;
}

// this function goes through the instructions and translates them
// to op codes depending on the operands (L = literal, R = reference, M = memory)
void process_inst(executable_t *exec, op_t *op, int *index) {
    int i = 0;

    int opcode;
    int i_code = 0;

    char *operand;

    for (i = 0; i < op->argc; i++) {
        operand = tokens[(*index)++];

        int lbl = is_label(operand, exec);
        
        if (operand[0] == '*') {
            i_code |= R << (i * 4);
            opcode = is_label(operand + 1, exec);
        }

        else if (operand[0] == '&') {
            i_code |= L << (i * 4);
            opcode = is_label(operand + 1, exec);
        }

        else if (lbl) {
            i_code |= M << (i * 4);
            opcode = lbl;
        }

        else {
            i_code |= L << (i * 4);
            opcode = atoi(operand);
        }

        exec->code[exec->length + i + 1] = opcode;
        free(operand);
    }

    exec->code[exec->length++] = (i_code << 16) | (int) (op - operations);
    exec->length += i;
}

// this function goes through the token array and either processes instructions
// or translates the token to a literal. the tokens are then free'd
void assemble(executable_t *exec) {
    int index = tok_start;

    while (index < tok_cnt) {
        char *tok = tokens[index++];
    
        op_t *op = is_instruction(tok);

        if (op) 
            process_inst(exec, op, &index);
        
        else 
            exec->code[exec->length++] = (tok[0] == '&') ? is_label(tok + 1, exec) : atoi(tok);

        free(tok);
    }
}

// this function creates an executable struct, adds registers,
// and translates the given files into "byte" code
executable_t *vm_load(char **fnames) {

    executable_t *exec = malloc(sizeof(executable_t));

    exec->code = malloc(RAM_SZ);

    add_label(exec, "$a",  A);
    add_label(exec, "$b",  B);
    add_label(exec, "$c",  C);
    add_label(exec, "$d",  D);
    add_label(exec, "$e",  E);
    add_label(exec, "$sp", SP);
    add_label(exec, "$bp", BP);
    add_label(exec, "$r", RET);

    exec->length = BP + 1;
    exec->entry  = BP + 1;
    
    while(*fnames) {
        FILE *fp;
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        int lno = 1;
        int start;

        fp = fopen(*fnames, "r");
        if (!fp) return NULL;

        while ((read = getline(&line, &len, fp)) != -1) {
            start = tok_cnt;

            if (line[strspn(line, " \t\v\r\n")] != '\0') 
                tokenize(exec, line);

            add_line(exec, *fnames, lno, start, tok_cnt);
        }

        fclose(fp);
        fnames++;
    }

    assemble(exec);

    return exec;
}

// TODO: make sure to free the memory we used!
void vm_unload(executable_t *e) {
    if (e) free(e);
}
