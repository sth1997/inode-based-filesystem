#ifndef INODE_H_
#define INODE_H_
#include "block.h"
#include "super_block.h"
#include "bitmap.h"

enum InodeType
{
    file,
    directory
};

class Inode: public Block
{
private:
    void openIndirectBlock(SuperBlock* superBlock = NULL, BitmapMultiBlocks* dataBitmap = NULL);
    void closeIndirectBlock();
    int indexToBlockNumber(int index);
    void setBlockNumber(int offset, int _blockNum, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    int createIndirectBlock(SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    void deleteIndirectBlock(SuperBlock* superBLock, BitmapMultiBlocks* dataBitmap);
public:
    int* const blockNumberList;
    int* size;
    int* type;
    int* refcnt;
    int* singleIndirectBlockNumber;
    const int maxDirectBlockNumber;
    Block* indirectBlock;
    Inode(int _blockNumber, bool _create, InodeType _type = file);
    ~Inode();
    Block* inodeToBlock(int offset);
    void append(char* buf, int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    void truncate(int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    static int newInodeBlockNum(SuperBlock* superBlock, Bitmap* inodeBitmap);
};

#endif //INODE_H_