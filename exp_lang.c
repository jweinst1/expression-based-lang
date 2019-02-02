#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Prototype for evaluatable expression
// ....... structure .........
// <val> <op> <args> ... <op>
// Or, other form is
// <val> <op> <args>  <term> ... <new exp>

enum exp_type_t {
   exp_type_stop = 0,
   exp_type_set,
   exp_type_if,
   exp_type_else,
   exp_type_plus,
   exp_type_minus,
   exp_type_int
};

typedef unsigned char exp_t;

#define TYPE_INT_SIZE (sizeof(int) + 1)

static const char* STR_TYPE_STOP = "stop";
static const char* STR_TYPE_SET = "set";
static const char* STR_TYPE_IF = "if";
static const char* STR_TYPE_ELSE = "else";
static const char* STR_TYPE_PLUS = "+";
static const char* STR_TYPE_MINUS = "-";
static const char* STR_TYPE_INT = "int";

const char* exp_str_type(enum exp_type_t type)
{
    switch(type)
    {
        case exp_type_stop: return STR_TYPE_STOP;
        case exp_type_set: return STR_TYPE_SET;
        case exp_type_if: return STR_TYPE_IF;
        case exp_type_else: return STR_TYPE_ELSE;
        case exp_type_plus: return STR_TYPE_PLUS;
        case exp_type_minus: return STR_TYPE_MINUS;
        case exp_type_int: return STR_TYPE_INT;
    }
}

static inline void exp_add_ints(exp_t* lfs, exp_t* rfs)
{
    *(int*)(lfs + 1) += *(int*)(rfs + 1);
}

static inline int exp_is_true_int(exp_t* val)
{
    return *(int*)(val + 1);
}

static void exp_copy(exp_t* dest, const exp_t* src)
{
    unsigned i;
    switch(*src)
    {
        case exp_type_int:
            for(i=0;i<5;i++) *dest++ = *src++;
            return;
        default:
            fprintf(stderr, "Copy Error: Expected 'int' for copy buy got %u\n", *src);
            exit(1);
    }
}

unsigned exp_evaluate(exp_t* exp)
{
    unsigned jump_nest;
    const exp_t* start_exp = exp;
    exp_t* current = NULL;
    goto STATE_EMPTY;
STATE_EMPTY:
    switch(*exp)
    {
        case exp_type_set:
             if(current != NULL) {
                // Here, if a value is written over the base, the base
                // should still be accessible in the frame above.
                // in a full implementation, this likely will work
                // with a separate buffer for base.
                exp_copy(current, ++exp);
                goto STATE_BASE;
             }
             current = ++exp;        
             goto STATE_BASE;
        case exp_type_stop:
             ++exp;
             goto STATE_RETURN;
        default:
            fprintf(stderr, "Unexpected byte: Got %u but expected byte '%s'\n", *exp, exp_str_type(exp_type_set));
            exit(1);
    }
STATE_BASE:
    // In this state, we must only have value types.
    // Non value types cannot be used as a base.
    switch(*exp)
    {
        case exp_type_int:
             exp += TYPE_INT_SIZE;
             goto STATE_INT;
        default:
             fprintf(stderr, "Base Error: Got type byte %u as base, expected type '%s'\n",
                              *exp, 
                              exp_str_type(exp_type_int));
             exit(1);
    }
STATE_INT:
    switch(*exp)
    {
        case exp_type_plus:
            ++exp;
            goto STATE_INT_PLUS;
        case exp_type_minus:
            ++exp;
            // not yet implemented
            goto STATE_INT;
        case exp_type_if:
            ++exp;
            goto STATE_IF;
        case exp_type_stop:
            ++exp;
            goto STATE_RETURN;

    }
STATE_IF:
    // this state allows the base to be kept
    // on a condition. for now, it's just int 1
    // or 0.
    switch(*exp)
    {
        case exp_type_int:
            if(exp_is_true_int(exp)) {
                exp += TYPE_INT_SIZE;
                if(*exp++ == exp_type_else) {
                    // else statements need there size prepended,
                    // this greatly increases speed they can be jumped.
                    jump_nest = *(unsigned*)(exp) + sizeof(unsigned);
                    exp += jump_nest;
                    goto STATE_BASE;
                } else {
                    // no else to skip.
                    goto STATE_BASE;
                }
            } else {
                // the if condition fails.
                exp += TYPE_INT_SIZE;
                if(*exp++ == exp_type_else) {
                    // else code will be activated.
                    exp += sizeof(unsigned);
                    if(*exp != exp_type_set) {
                        // else needs a set.
                        fprintf(stderr, "Else Error: Expected 'set', got: %u\n", *exp);
                        exit(1);
                    }
                    goto STATE_EMPTY;
                } else {
                    // the if condition fails but no else is found.
                    // Value is treated as empty because no ins is given
                    // on how to replace it.
                    goto STATE_EMPTY;
                }
            }
        case exp_type_set:
             jump_nest = exp_evaluate(exp);
             if(exp_is_true_int(exp + 1)) {
                exp += jump_nest;
                if(*exp++ == exp_type_else) {
                    // else statements need there size prepended,
                    // this greatly increases speed they can be jumped.
                    jump_nest = *(unsigned*)(exp) + sizeof(unsigned);
                    exp += jump_nest;
                    goto STATE_BASE;
                } else {
                    // no else to skip.
                    goto STATE_BASE;
                }
             } else {
                exp += jump_nest;
                if(*exp++ == exp_type_else) {
                    // else code will be activated.
                    exp += sizeof(unsigned);
                    if(*exp != exp_type_set) {
                        // else needs a set.
                        fprintf(stderr, "Else Error: Expected 'set', got: %u\n", *exp);
                        exit(1);
                    }
                    goto STATE_EMPTY;
                } else {
                    // the if condition fails but no else is found.
                    // Value is treated as empty because no ins is given
                    // on how to replace it.
                    goto STATE_EMPTY;
                }
             }
        default:
             fprintf(stderr, "If Error: Expected 'int' or 'set' got '%u'\n", *exp);
             exit(1);
    }
STATE_INT_PLUS:
    switch(*exp)
    {
        case exp_type_int:
            exp_add_ints(current, exp);
            exp += TYPE_INT_SIZE;
            // the state is recursive until a stop or set byte is found
            goto STATE_INT_PLUS;
        case exp_type_stop:
            // This op is done, but might be others for this base.
            ++exp;
            goto STATE_INT;
        case exp_type_set:
            // this is where the nested call occurs.
            jump_nest = exp_evaluate(exp);
            exp_add_ints(current, exp + 1);
            exp += jump_nest;
            // When nested expression has been evaluated,
            // continue same op, check for stop.
            goto STATE_INT_PLUS;


    }
STATE_RETURN:
    return exp - start_exp;
}

static exp_t test_bin_1[] = {
    exp_type_set,
    exp_type_int,
    3, 0, 0, 0, // little endian 32-bit int.
    exp_type_plus,
    exp_type_set, // nested
    exp_type_int,
    3, 0, 0, 0, // little endian 32-bit int.
    exp_type_plus,
    exp_type_int,
    10, 0, 0, 0, // little endian 32-bit int.
    exp_type_stop,
    exp_type_stop,
    exp_type_stop,
    // Top scope set of stop statements.
    exp_type_stop,
    exp_type_stop,
    exp_type_stop
};

// nested if test case
static exp_t test_bin_2[] = {
    exp_type_set,
    exp_type_int,
    5, 0, 0, 0,
    exp_type_if,
    exp_type_int,
    0, 0, 0, 0,
    exp_type_else,
    13, 0, 0, 0,
    exp_type_set,
    exp_type_int,
    12, 0, 0, 0,
    exp_type_plus,
    exp_type_int,
    1, 0, 0, 0,
    exp_type_stop,
    exp_type_stop
};

int main(void) {
    puts("eval lang test");
    exp_t* tester = test_bin_1;
    puts("evaluating nested integer sum, the result should be 16");
    exp_evaluate(tester);
    if(tester[1] == exp_type_int) {
        puts("result type is int, this passes.");
        printf("the integer result is =  %d\n", *(int*)(tester + 2));
    } else {
        puts("result type is not int, FAILED.");
    }
    puts("evaluating if else instruction test");
    tester = test_bin_2;
    exp_evaluate(tester);
    if(tester[1] == exp_type_int) {
        puts("result is int, pass so far");
        printf("the integer result is =  %d\n", *(int*)(tester + 2));
    } else {
        puts("result type is not int, FAILED.");
    }
    return 0;
}
