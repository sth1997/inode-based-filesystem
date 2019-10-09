#ifndef COMMON_H_
#define COMMON_H_

#include "const.h"
#include "block.h"
#include "super_block.h"
#include "bitmap.h"
#include <string>

Block* blockNumberToBlock(int _number);
void createSuperBlockAndBitmaps(SuperBlock*& superBlock, Bitmap*& inodeBitmap, BitmapMultiBlocks*& dataBitmap);
void deleteSuperBlockAndBitmaps(SuperBlock*& superBlock, Bitmap*& inodeBitmap, BitmapMultiBlocks*& dataBitmap);
int stringMatch(const std::string& fileName, Block* b, int size);
#endif //COMMON_H_