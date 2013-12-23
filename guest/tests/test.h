#ifndef TEST_H
#define TEST_H
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
struct Test;

typedef int (*TestFunctionPtr)(struct Test *test);
typedef int (*CheckFunctionPtr)(struct Test *test);

extern void test_start(struct Test *test);
extern void test_checkpoint(struct Test *test);
extern void test_error(struct Test *test, int in_sig_handler);

extern sigjmp_buf g_jmp_buffer;

// Test structure
struct Test
{
    FILE *file;
    const char *name;

    TestFunctionPtr test_func;
    CheckFunctionPtr check_func;

    sigjmp_buf jmp_buffer;

    unsigned int retries;
    unsigned int max_retries;

    const char *func_str;
    const char *op_str;
    const char *context_str;
    int value;
};

// Test macros
#define TEST_FUNC() extern void test_func(struct Test* test)

#define CHECK_FUNC() extern void check_func(struct Test* test)

#define CHECKPOINT() test_checkpoint(test)

#define OUTPUT(str) fprintf(test->file, "[%s] " str, test->name)
#define OUTPUT_VARS(str, vars) fprintf(test->file, "[%s] " \
                                       str, test->name, vars)

#define ASSERT(name, op, val, context) \
    do{ \
    test->func_str = #name; \
    test->op_str = #op; \
    test->context_str = context; \
    val = name; \
    test->value = val; \
    if (val op) test_error(test, 0); \
    }while(0)

#define ASSERT_CAST(name, op, val, context, cast) \
    do{ \
    test->func_str = #name; \
    test->op_str = #op; \
    test->context_str = context; \
    val = (cast)name; \
    test->value = 0; \
    if (val op) test_error(test, 0); \
    }while(0)

#define CALLFUNC(name, context) \
    do{ \
    test->func_str= #name; \
    test->op_str = ""; \
    test->context_str = context; \
    name; \
    }while(0)

#endif //TEST_H
