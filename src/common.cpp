#include "../include/common.h"
#include "../include/const.h"

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

void deleteSuperBlockAndBitmaps(SuperBlock*& superBlock, Bitmap*& inodeBitmap, BitmapMultiBlocks*& dataBitmap)
{
    delete dataBitmap;
    delete inodeBitmap;
    delete superBlock;
    dataBitmap = NULL;
    inodeBitmap = NULL;
    superBlock = NULL;
}

bool myStrcmp(const char* a, const char* b)
{
    for (int i = 0; i < FILE_NAME_LEN; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

int stringMatch(const std::string& fileName, Block* b, int size)
{
    if (size > BLOCK_SIZE)
        size = BLOCK_SIZE;
    const char* str_c = fileName.c_str();
    for (int offset = 0; offset < size; offset += 16)
    {
        if (myStrcmp(str_c, b->data + offset))
        {
            return *((int*)(b->data + offset + FILE_NAME_LEN));
        }
    }
    return -1;
}