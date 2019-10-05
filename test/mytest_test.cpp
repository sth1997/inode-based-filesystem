#include <gtest/gtest.h>
#include <mytest.h>

TEST(test_mytest, printA)
{
    MyTest test(1,2);
    ASSERT_EQ(test.getA(), 1);
}