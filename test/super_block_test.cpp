#include <gtest/gtest.h>
#include <../include/super_block.h>
#include <../include/const.h>

TEST(super_block_test, create)
{
    SuperBlock* b1 = new SuperBlock(true);
    ASSERT_TRUE(b1->getChanged());
    delete b1;

    SuperBlock* b2 = new SuperBlock(false);
    ASSERT_EQ(*b2->maxDataBlockNumbers, MAX_DATA_BLOCK_NUMBERS);
    ASSERT_EQ(*b2->inodeBitmapBeginBlock, 1);
    ASSERT_EQ(*b2->dataBitmapBeginBlock, 2);
    ASSERT_EQ(*b2->nextFreeDataBitmapBlock, 2);
    delete b2;
}