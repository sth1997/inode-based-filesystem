#include "../include/super_block.h"
#include "../include/const.h"
#include <cstdlib>

SuperBlock::SuperBlock(bool create):
Block(0, create), maxDataBlockNumbers((int*) &data[0]), inodeNumbers((int*) &data[4]),
inodeBitmapBeginBlock((int*) &data[8]), inodeBitmapEndBlock((int*) &data[12]),
dataBitmapBeginBlock((int*) &data[16]), dataBitmapEndBlock((int*) &data[20]),
inodeBeginBlock((int*) &data[24]), inodeEndBlock((int*) &data[28]),
dataBeginBlock((int*) &data[32]), dataEndBlock((int*) &data[36]),
nextFreeInodeBlockNumber((int*) &data[40]), nextFreeDataBlockNumber((int*) &data[44])
{
    if (create)
    {
        *maxDataBlockNumbers = MAX_DATA_BLOCK_NUMBERS;
        *inodeNumbers = MAX_INODE_NUMBERS;
        *inodeBitmapBeginBlock = 1; // [1, 2)
        *inodeBitmapEndBlock = 2;
        *dataBitmapBeginBlock = 2;
        *dataBitmapEndBlock = *dataBitmapBeginBlock + MAX_DATA_BLOCK_NUMBERS / 8 / BLOCK_SIZE;
        *inodeBeginBlock = *dataBitmapEndBlock;
        *inodeEndBlock = *inodeBeginBlock + MAX_INODE_NUMBERS; // one inode use one block
        *dataBeginBlock = *inodeEndBlock;
        *dataEndBlock = *dataBeginBlock + MAX_DATA_BLOCK_NUMBERS;
        *nextFreeInodeBlockNumber = *inodeBeginBlock;
        *nextFreeDataBlockNumber = *dataBeginBlock;
        setChanged();
        system("rm -rf /tmp/fsdata/");
        system("mkdir -p /tmp/fsdata/");
    }
}