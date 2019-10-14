#include <gtest/gtest.h>
#include "../include/super_block.h"
#include "../include/const.h"
#include "../include/bitmap.h"
#include "../include/common.h"
#include "../include/inode.h"

using std::string;

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
    deleteSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
    delete[] buf;
}

TEST(inode_test, directory)
{
    SuperBlock* superBlock;
    Bitmap* inodeBitmap;
    BitmapMultiBlocks* dataBitmap;
    createSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
    int rootBlockNum = Inode::createRootInode("/", superBlock, inodeBitmap, dataBitmap);
    Inode* rootInode = new Inode(rootBlockNum, false);
    int dir0BlockNum = rootInode->createInode("12345678dir0", superBlock, inodeBitmap, dataBitmap, directory);
    int dir1BlockNum = rootInode->createInode("12345678dir1", superBlock, inodeBitmap, dataBitmap, directory);
    int data0BlockNum = rootInode->createInode("1234567data0", superBlock, inodeBitmap, dataBitmap, file);
    ASSERT_EQ(*rootInode->size, 16 * 3);
    Inode* dir1Inode = new Inode(dir1BlockNum, false);
    int dir2BlockNum = dir1Inode->createInode("12345678dir2", superBlock, inodeBitmap, dataBitmap, directory);
    ASSERT_EQ(*superBlock->inodeNumbers, 5);
    ASSERT_EQ(dir2BlockNum, *superBlock->inodeBeginBlock + 4);
    int data1BlockNum = dir1Inode->createInode("1234567data1", superBlock, inodeBitmap, dataBitmap, file);
    delete rootInode;
    delete dir1Inode;
    ASSERT_EQ(Inode::nameToInodeNumber("1234567data0", rootBlockNum - *superBlock->inodeBeginBlock, superBlock, inodeBitmap), 3);
    ASSERT_EQ(Inode::nameToInodeBlockNumber("12345678dir2", rootBlockNum - *superBlock->inodeBeginBlock, superBlock, inodeBitmap), -1);
    ASSERT_EQ(Inode::nameToInodeBlockNumber("12345678dir2", dir1BlockNum - *superBlock->inodeBeginBlock, superBlock, inodeBitmap), 4 + *superBlock->inodeBeginBlock);

    ASSERT_EQ(Inode::absolutePathToInodeNumber("/", superBlock, inodeBitmap), 0);
    ASSERT_EQ(Inode::pathToInodeNumber("12345678dir1/12345678dir2", 0, superBlock, inodeBitmap), dir2BlockNum - *superBlock->inodeBeginBlock);
    ASSERT_EQ(Inode::absolutePathToInodeNumber("/12345678dir1/12345678dir2", superBlock, inodeBitmap), dir2BlockNum - *superBlock->inodeBeginBlock);
    ASSERT_EQ(Inode::pathToInodeNumber("12345678dir1/1234567data1", 0, superBlock, inodeBitmap), data1BlockNum - *superBlock->inodeBeginBlock);
    ASSERT_EQ(Inode::pathToInodeNumber("12345678dir0/12345678dir2", 0, superBlock, inodeBitmap), -1);
    Inode::link("12345678dir1/1234567data1", "12345678dir1/12345678dir2/1234567data2", 0, superBlock, inodeBitmap, dataBitmap);
    int data2InodeNumber = Inode::pathToInodeNumber("12345678dir1/12345678dir2/1234567data2", 0, superBlock, inodeBitmap);
    ASSERT_EQ(data2InodeNumber, data1BlockNum - *superBlock->inodeBeginBlock);
    Inode* data2Inode = Inode::inodeNumberToInode(data2InodeNumber, superBlock, inodeBitmap);
    ASSERT_EQ(*data2Inode->refcnt, 2);
    ASSERT_EQ(*data2Inode->type, file);
    delete data2Inode;
    dir1Inode = new Inode(dir1BlockNum, false);
    dir1Inode->deleteInode(string("1234567data1"), superBlock, inodeBitmap, dataBitmap);
    ASSERT_EQ(dir1Inode->nameToInodeNumber("1234567data1", superBlock, inodeBitmap), -1);
    delete dir1Inode;
    ASSERT_EQ(Inode::pathToInodeNumber("1234567data1", dir1BlockNum - *superBlock->inodeBeginBlock, superBlock, inodeBitmap), -1);
    data2Inode = Inode::inodeNumberToInode(data2InodeNumber, superBlock, inodeBitmap);
    ASSERT_EQ(*data2Inode->refcnt, 1);
    deleteSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
}