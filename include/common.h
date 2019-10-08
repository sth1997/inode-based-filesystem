#ifndef COMMON_H_
#define COMMON_H_

#include "const.h"
#include "block.h"
#include "super_block.h"
#include "bitmap.h"

Block* blockNumberToBlock(int _number);
void createSuperBlockAndBitmaps(SuperBlock*& superBlock, Bitmap*& inodeBitmap, BitmapMultiBlocks*& dataBitmap);
#endif //COMMON_H_