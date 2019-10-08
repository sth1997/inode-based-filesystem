#include "../include/common.h"

Block* blockNumberToBlock(int _number)
{
    Block* b = new Block(_number, false);
    return b;
}

void createSuperBlockAndBitmaps(SuperBlock*& superBlock, Bitmap*& inodeBitmap, BitmapMultiBlocks*& dataBitmap)
{
    superBlock = new SuperBlock(true);
    inodeBitmap = new Bitmap(*superBlock->inodeBitmapBeginBlock, true, *superBlock->inodeBeginBlock, MAX_INODE_NUMBERS / 8);
    dataBitmap = new BitmapMultiBlocks(*superBlock->dataBitmapBeginBlock, *superBlock->dataBitmapEndBlock, true, *superBlock->dataBeginBlock);
}