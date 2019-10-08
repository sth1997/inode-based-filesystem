#include "../include/inode.h"
#include "../include/const.h"
#include <assert.h>
#include "../include/common.h"
#include <cstring>

Inode::Inode(int _blockNumber, bool _create, InodeType _type):
Block(_blockNumber, _create), blockNumberList((int*) &data[16]),
size((int*) &data[0]), type((int*) &data[4]), refcnt((int*) &data[8]),
singleIndirectBlockNumber((int*) &data[12]), maxDirectBlockNumber(BLOCK_SIZE / 4 - 4)
{
    indirectBlock = NULL;
    if (_create)
    {
        *size = 0;
        *type = _type;
        *refcnt = 1; //TODO : 0 or 1?
        *singleIndirectBlockNumber = -1;
        setChanged();
    }
}

Inode::~Inode()
{
    if (indirectBlock != NULL)
    {
        closeIndirectBlock();
    }
}

int Inode::createIndirectBlock(SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int blockNum = dataBitmap->getNextFree(*superBlock->nextFreeDataBitmapBlock, *superBlock->nextFreeDataBitmapByte);
    assert(blockNum != -1);
    dataBitmap->set(blockNum, false);
    setChanged();
    return blockNum;
}

void Inode::openIndirectBlock(SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    assert(indirectBlock == NULL);
    if (*singleIndirectBlockNumber == -1)
    {
        assert(superBlock != NULL);
        assert(dataBitmap != NULL);
        *singleIndirectBlockNumber = createIndirectBlock(superBlock, dataBitmap);
        indirectBlock = new Block(*singleIndirectBlockNumber, true);
    }
    else
        indirectBlock = new Block(*singleIndirectBlockNumber, false);
}

void Inode::closeIndirectBlock()
{
    assert(indirectBlock != NULL);
    delete indirectBlock;
    indirectBlock = NULL;
}

int Inode::indexToBlockNumber(int index)
{
    assert(index < (*size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    assert(index < maxDirectBlockNumber + BLOCK_SIZE / 4);
    if (index < maxDirectBlockNumber) // direct
    {
        return blockNumberList[index];
    }
    else
    {
        if (indirectBlock == NULL)
        {
            openIndirectBlock();
        }
        int* intData = (int*) indirectBlock->data;
        return intData[index - maxDirectBlockNumber];
    }
}

Block* Inode::inodeToBlock(int offset)
{
    int index = offset / BLOCK_SIZE;
    int blockNumber = indexToBlockNumber(index);
    return blockNumberToBlock(blockNumber);
}

void Inode::setBlockNumber(int offset, int _blockNum, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int index = offset / BLOCK_SIZE;
    assert(index < maxDirectBlockNumber + BLOCK_SIZE / 4);
    if (index < maxDirectBlockNumber) // direct
    {
        blockNumberList[index] = _blockNum;
        setChanged();
    }
    else
    {
        if (indirectBlock == NULL)
        {
            openIndirectBlock(superBlock, dataBitmap);
        }
        int* intData = (int*) indirectBlock->data;
        intData[index - maxDirectBlockNumber] = _blockNum;
        indirectBlock->setChanged();
    }
}

void Inode::append(char* buf, int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int restSize = (maxDirectBlockNumber + BLOCK_SIZE / 4) * BLOCK_SIZE - *size;
    assert(len <= restSize);
    while (len > 0)
    {
        int appendLen = BLOCK_SIZE - *size % BLOCK_SIZE;
        if (appendLen == BLOCK_SIZE)
        {
            if (appendLen > len)
                appendLen = len;
            int newBlockNumber = dataBitmap->getNextFree(*superBlock->nextFreeDataBitmapBlock, *superBlock->nextFreeDataBitmapByte);
            assert(newBlockNumber != -1);
            dataBitmap->set(newBlockNumber, false);
            setBlockNumber(*size, newBlockNumber, superBlock, dataBitmap);
            Block* b = new Block(newBlockNumber, true);
            memcpy(b->data, buf, appendLen);
            b->setChanged();
            delete b;
        }
        else
        {
            if (appendLen > len)
                appendLen = len;
            Block* b = inodeToBlock(*size);
            memcpy(b->data + (*size % BLOCK_SIZE), buf, appendLen);
            b->setChanged();
            delete b;
        }
        buf += appendLen;
        *size += appendLen;
        len -= appendLen;
    }
    setChanged();
}

void Inode::deleteIndirectBlock(SuperBlock* superBLock, BitmapMultiBlocks* dataBitmap)
{
    if (indirectBlock != NULL)
        closeIndirectBlock();
    dataBitmap->set(*singleIndirectBlockNumber, true);
    *singleIndirectBlockNumber = -1;
}

void Inode::truncate(int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    if (len == *size)
        return;
    if (len > *size)
    {
        char* buf = new char[len - *size];
        append(buf, len - *size, superBlock, dataBitmap);
    }
    else
    {
        while (len < *size)
        {
            int restBlockLen = *size % BLOCK_SIZE;
            if (restBlockLen == 0)
                restBlockLen = BLOCK_SIZE;
            int reduceLen = restBlockLen;
            if (reduceLen > *size - len)
                reduceLen = *size - len;
            if (reduceLen == restBlockLen)
            {
                int blockNum = indexToBlockNumber((*size - 1) / BLOCK_SIZE);
                dataBitmap->set(blockNum, true);
                setBlockNumber(*size - 1, -1, superBlock, dataBitmap);
            }
            else
            {
                //do nothing
            }
            *size -= reduceLen;
        }
        int index = (*size - 1) / BLOCK_SIZE;
        if ((index < maxDirectBlockNumber) && (*singleIndirectBlockNumber != -1))
            deleteIndirectBlock(superBlock, dataBitmap);
    }
    assert(len == *size);
    setChanged();
}

int Inode::newInodeBlockNum(SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    assert(*superBlock->inodeNumbers < MAX_INODE_NUMBERS);
    int blockNum = inodeBitmap->getNextFree(*superBlock->nextFreeInodeBitmapByte);
    assert(blockNum != -1);
    inodeBitmap->set(blockNum, false);
    (*superBlock->inodeNumbers)++;
    return blockNum;
}