#include "../include/common.h"

Block* blockNumberToBlock(int _number)
{
    Block* b = new Block(_number, false);
    return b;
}