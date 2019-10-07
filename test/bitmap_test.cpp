#include <gtest/gtest.h>
#include <../include/bitmap.h>
#include <../include/const.h>

TEST(bitmap_test, bitmap)
{
    int baseBlockNum = 10;
    Bitmap* b = new Bitmap(1, true, baseBlockNum);
    int startByte = 0;
    int blockNum = b->getNextFree(startByte);
    ASSERT_EQ(startByte, 0);
    ASSERT_EQ(blockNum, baseBlockNum);
    b->setUsed(baseBlockNum + 8);
    startByte = 1;
    blockNum = b->getNextFree(startByte);
    ASSERT_EQ(startByte, 1);
    ASSERT_EQ(blockNum, baseBlockNum + 9);
    delete b;

    b = new Bitmap(1, false, baseBlockNum);
    for (int i = 9; i < 16; ++i)
        b->setUsed(baseBlockNum + i);
    blockNum = b->getNextFree(startByte);
    ASSERT_EQ(startByte, 2);
    ASSERT_EQ(blockNum, baseBlockNum + 16);
    delete b;

    b = new Bitmap(1, false, baseBlockNum);
    startByte = 1;
    blockNum = b->getNextFree(startByte);
    ASSERT_EQ(startByte, 2);
    ASSERT_EQ(blockNum, baseBlockNum + 16);
    b->setAll(false);
    blockNum = b->getNextFree(startByte);
    ASSERT_EQ(startByte, 0);
    ASSERT_EQ(blockNum, -1);
    delete b;
}

TEST(bitmap_test, multiBlocks)
{
    int baseBlockNum = 10;
    BitmapMultiBlocks* b = new BitmapMultiBlocks(2, 5, true, baseBlockNum);
    int startBlock = 2;
    int startByte = 0;
    int blockNum = b->getNextFree(startBlock, startByte);
    ASSERT_EQ(startBlock, 2);
    ASSERT_EQ(startByte, 0);
    ASSERT_EQ(blockNum, baseBlockNum);
    delete b;

    Bitmap* b2 = new Bitmap(3, false, baseBlockNum);
    b2->setAll(false);
    delete b2;

    b = new BitmapMultiBlocks(2, 5, false, baseBlockNum);
    startBlock = 3;
    int blocksPerBitmap = BLOCK_SIZE * 8;
    b->set(2 * blocksPerBitmap + baseBlockNum, false);
    blockNum = b->getNextFree(startBlock, startByte);
    ASSERT_EQ(startBlock, 4);
    ASSERT_EQ(startByte, 0);
    ASSERT_EQ(blockNum, 2 * blocksPerBitmap + baseBlockNum + 1);
}