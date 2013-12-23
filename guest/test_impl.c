#include "tests/test.h"
#include <stdio.h>
#include <stdlib.h>

void test_start(struct Test *test)
{
    fprintf(test->file, "[%s] Test started\n", test->name);
    if (test->test_func) (*test->test_func)(test);
    fprintf(test->file, "[%s] Test finished\n", test->name);
    fprintf(test->file, "[%s] Check started\n", test->name);
    if (test->check_func) (*test->check_func)(test);
    fprintf(test->file, "[%s] Check finished\n", test->name);
    fprintf(test->file, "[%s] TEST SUCCESSFUL\n", test->name);
}

void test_checkpoint(struct Test *test)
{
    sigsetjmp(test->jmp_buffer, 1);
}

void test_error(struct Test *test, int in_sig_handler)
{
    if (test->func_str && test->op_str && test->context_str) {
        fprintf(test->file, "[%s] Error[%s]: %s %s, got: %d (%s)\n",
                test->name, test->context_str, test->func_str, test->op_str, 
                test->value, strerror(errno));
    }

    if (test->retries >= test->max_retries) {
        fprintf(test->file, "[%s] TEST FAILED\n", test->name);
        siglongjmp(g_jmp_buffer, 1);
        return;
    }

    test->retries++;
    fprintf(test->file, "[%s] Retrying... %d/%d\n", test->name, 
            test->retries, test->max_retries);

    if (in_sig_handler) {
        fprintf(test->file, "[%s] TEST FAILED\n", test->name);
        siglongjmp(g_jmp_buffer, 1);
    } else
        siglongjmp(test->jmp_buffer, 1);
}
