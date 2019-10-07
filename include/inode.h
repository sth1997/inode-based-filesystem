#ifndef INODE_H_
#define INODE_H_
#include "block.h"

enum InodeType
{
    file,
    directory
};

class Inode: public Block
{
private:
    void openIndirectBlock();
    void closeIndirectBlock();
    int indexToBlockNumber(int index);
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
};

#endif //INODE_H_