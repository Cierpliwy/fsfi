#include "test.h"

TEST_FUNC()
{
    OUTPUT("Doing test func\n");
    *(char*)(12312) = 12;
}

CHECK_FUNC()
{
    OUTPUT("Doing check func\n");
}
