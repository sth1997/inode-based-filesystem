#ifndef BITMAP_H_
#define BITMAP_H_
#include "block.h"

class Bitmap: public Block
{
private:
    int sizeBytes;
    int baseBlockNumber;
public:
    void set(int _blockNumber, bool free);
    Bitmap(int _blockNumber, bool _create, int _baseBLockNumber, int _size = -1);
    void setFree(int _blockNumber);
    void setUsed(int _blockNumber);
    void setAll(bool free);
    int getNextFree(int &startByte) const;
};

class BitmapMultiBlocks
{
private:
    int startBlockNumber; //[startBlockNumber, endBlockNumber)
    int endBlockNumber;
    int baseBlockNumber;
public:
    BitmapMultiBlocks(int _startBlockNumber, int _endBlockNumber, bool _create, int _baseBlockNumber);
    void setAll(bool free);
    void set(int _blockNumber, bool free);
    int getNextFree(int &startBlock, int &startByte);

};

#endif //BITMAP_H_