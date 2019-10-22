#ifndef INODE_H_
#define INODE_H_
#include "block.h"
#include "super_block.h"
#include "bitmap.h"
#include <string>

enum InodeType
{
    file,
    directory,
    slink
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
    int* const size;
    InodeType* const type;
    int* const refcnt;
    int* const singleIndirectBlockNumber;
    const int maxDirectBlockNumber;
    Block* indirectBlock;
    Inode(int _blockNumber, bool _create, InodeType _type = file);
    ~Inode();
    Block* inodeToBlock(int offset);
    void append(const char* buf, int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    void truncate(int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap);
    static int newInodeBlockNum(SuperBlock* superBlock, Bitmap* inodeBitmap);
    static Inode* inodeNumberToInode(int inodeNumber, SuperBlock* SuperBlock, Bitmap* inodeBitmap);
    static Block* inodeNumberToBlock(int offset, int inodeNum, SuperBlock* superBlock, Bitmap* inodeBitmap);
    int nameToInodeNumber(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, int* offsetPtr = NULL);
    static int nameToInodeNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap);
    static int nameToInodeBlockNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap);
    int createInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap, const InodeType inodeType, int inodeNumber = -1);
    void deleteInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap);
    static int createRootInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap);
    static int pathToInodeNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap);
    static int absolutePathToInodeNumber(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap);
    static void link(const std::string& srcName, const std::string& dstName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap);
};

#endif //INODE_H_