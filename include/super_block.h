#ifndef SUPER_BLOCK_H_
#define SUPER_BLOCK_H_
#include "block.h"

class SuperBlock: public Block
{
public:
    int* const maxDataBlockNumbers;
    int* const inodeNumbers;
    int* const inodeBitmapBeginBlock;
    int* const inodeBitmapEndBlock;
    int* const dataBitmapBeginBlock;
    int* const dataBitmapEndBlock;
    int* const inodeBeginBlock;
    int* const inodeEndBlock;
    int* const dataBeginBlock;
    int* const dataEndBlock;
    int* const nextFreeInodeBitmapByte;
    int* const nextFreeDataBitmapByte;
    int* const nextFreeDataBitmapBlock;
    SuperBlock(bool create);
};

#endif //SUPER_BLOCK_H_