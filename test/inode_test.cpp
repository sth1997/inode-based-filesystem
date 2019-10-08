#include <gtest/gtest.h>
#include "../include/super_block.h"
#include "../include/const.h"
#include "../include/bitmap.h"
#include "../include/common.h"
#include "../include/inode.h"

TEST(inode_test, largeFile)
{
    SuperBlock* superBlock;
    Bitmap* inodeBitmap;
    BitmapMultiBlocks* dataBitmap;
    createSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
    ASSERT_EQ(*superBlock->inodeNumbers, 0);
    int inodeBlockNum = Inode::newInodeBlockNum(superBlock, inodeBitmap);
    ASSERT_EQ(inodeBlockNum, *superBlock->inodeBeginBlock);
    ASSERT_EQ(*superBlock->inodeNumbers, 1);
    inodeBlockNum = Inode::newInodeBlockNum(superBlock, inodeBitmap);
    ASSERT_EQ(inodeBlockNum, *superBlock->inodeBeginBlock + 1);
    ASSERT_EQ(*superBlock->inodeNumbers, 2);
    Inode* inode = new Inode(inodeBlockNum, true, file);
    int bufSize = 1024 * 1024 * 16;
    int* buf = new int[bufSize / 4];
    for (int i = 0; i < bufSize / 4; ++i)
        buf[i] = i;
    inode->append((char*)buf, 1024 * 1024 * 5, superBlock, dataBitmap);
    ASSERT_EQ(*inode->singleIndirectBlockNumber, *superBlock->dataBeginBlock + inode->maxDirectBlockNumber + 1);
    Block* b = inode->inodeToBlock(1024* 1024 * 2);
    for (int i = 0; i < 1024; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), 1024 * 1024 * 2 / 4 + i);
    delete b;
    b = inode->inodeToBlock(1024 * 1024 * 4 + 4096 * 10);
    for (int i = 0; i < 1024; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), (1024 * 1024 * 4 + 4096 * 10) / 4 + i);
    delete b;

    inode->truncate(1000000, superBlock, dataBitmap);
    ASSERT_EQ(*inode->singleIndirectBlockNumber, -1);
    inode->append((char*)buf, 1024 * 1024 * 5, superBlock, dataBitmap);
    ASSERT_NE(*inode->singleIndirectBlockNumber, -1);
    b = inode->inodeToBlock(1000000);
    for (int i = 0; i < (1000000 % BLOCK_SIZE)/4; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), (1000000 - 1000000 % BLOCK_SIZE) / 4 + i);
    for (int i = (1000000 % BLOCK_SIZE)/4; i < 1024; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), i - (1000000 % BLOCK_SIZE)/4);
    delete b;
    b = inode->inodeToBlock(1024* 1024 * 2);
    for (int i = 0; i < 1024; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), 1024 * 1024 * 2 / 4 + i - 1000000 / 4);
    delete b;
    b = inode->inodeToBlock(1024 * 1024 * 4 + 4096 * 10);
    for (int i = 0; i < 1024; ++i)
        ASSERT_EQ(*((int*)(&b->data[i * 4])), (1024 * 1024 * 4 + 4096 * 10) / 4 + i - 1000000 / 4);
    delete b;
    
    delete[] buf;
}