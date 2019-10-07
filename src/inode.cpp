#include "../include/inode.h"
#include "../include/const.h"
#include <assert.h>
#include "../include/common.h"

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

void Inode::openIndirectBlock()
{
    assert(indirectBlock == NULL);
    assert(*singleIndirectBlockNumber != -1);
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
    assert(index < *size);
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