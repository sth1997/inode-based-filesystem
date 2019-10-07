#include "../include/bitmap.h"
#include "../include/const.h"
#include "../include/common.h"
#include <assert.h>
#include <cstring>

Bitmap::Bitmap(int _blockNumber, bool _create, int _baseBlockNumber, int _sizeBytes):
Block(_blockNumber, _create), baseBlockNumber(_baseBlockNumber)
{
    if (_sizeBytes == -1)
        sizeBytes = BLOCK_SIZE;
    else
        sizeBytes = _sizeBytes;
    if (_create)
        setAll(true);
}

void Bitmap::set(int _blockNumber, bool free)
{
    int index = _blockNumber - baseBlockNumber;
    assert(index / 8 < sizeBytes);
    assert(index >= 0);
    char& tmp = data[index / 8];
    char allFree = (char) (1 << 8) - 1;
    int posInByte = index & 7;
    if (free)
        tmp |= (1 << posInByte);
    else
        tmp &= allFree - (1 << posInByte);
    setChanged();
}

void Bitmap::setFree(int _blockNumber)
{
    set(_blockNumber, true);
}

void Bitmap::setUsed(int _blockNumber)
{
    set(_blockNumber, false);
}

void Bitmap::setAll(bool free)
{
    if (free)
        memset(data, -1, sizeBytes);
    else
        memset(data, 0, sizeBytes);
    setChanged();
}

//return -1 means there is no free position in this bitmap/block
int Bitmap::getNextFree(int &startByte) const
{
    for (int i = startByte; i < sizeBytes; ++i)
        if (data[i])
        {
            unsigned char tmp = (unsigned char) (data[i] & (-data[i]));
            int res = 0;
            for (; tmp; tmp >>= 1)
                ++res;
            int ret = i * 8 + res - 1;
            startByte = i;
            return ret + baseBlockNumber;
        }
    for (int i = 0; i < startByte; ++i)
        if (data[i])
        {
            unsigned char tmp = (unsigned char) (data[i] & (-data[i]));
            int res = 0;
            for (; tmp; tmp >>= 1)
                ++res;
            int ret = i * 8 + res - 1;
            startByte = i;
            return ret + baseBlockNumber;
        }
    startByte = 0;
    return -1;
}

BitmapMultiBlocks::BitmapMultiBlocks(int _startBlockNumber, int _endBlockNumber, bool _create, int _baseBlockNumber):
startBlockNumber(_startBlockNumber), endBlockNumber(_endBlockNumber), baseBlockNumber(_baseBlockNumber)
{
    if (_create)
    {
        for (int i = startBlockNumber; i < endBlockNumber; ++i)
        {
            Bitmap b(i, true, baseBlockNumber);
            b.setAll(true);
        }
    }
}

void BitmapMultiBlocks::setAll(bool free)
{
    for (int i = startBlockNumber; i < endBlockNumber; ++i)
    {
        Bitmap b(i, false, baseBlockNumber);
        b.setAll(free);
    }
}

void BitmapMultiBlocks::set(int _blockNumber, bool free)
{
    _blockNumber -= baseBlockNumber;
    int blocksPerBitmap = BLOCK_SIZE * 8;
    // TODO: do not read block every time.
    Bitmap b(startBlockNumber + _blockNumber / blocksPerBitmap, false, 0);
    b.set(_blockNumber % blocksPerBitmap, free);
}

int BitmapMultiBlocks::getNextFree(int &startBlock, int &startByte)
{
    for (int i = startBlock; i < endBlockNumber; ++i)
    {
        Bitmap b(i, false, 0);
        int blockNum = b.getNextFree(startByte);
        if (blockNum != -1)
        {
            startBlock = i;
            return blockNum + (i - startBlockNumber) * BLOCK_SIZE * 8 + baseBlockNumber;
        }
        else
            startByte = 0;
    }

    for (int i = 0; i < startBlock; ++i)
    {
        Bitmap b(i, false, 0);
        int blockNum = b.getNextFree(startByte);
        if (blockNum != -1)
        {
            startBlock = i;
            return blockNum + (i - startBlockNumber) * BLOCK_SIZE * 8 + baseBlockNumber;
        }
        else
            startByte = 0;
    }
    return -1;
}