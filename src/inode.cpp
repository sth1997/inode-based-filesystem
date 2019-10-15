#include "../include/inode.h"
#include "../include/const.h"
#include <assert.h>
#include "../include/common.h"
#include <cstring>

Inode::Inode(int _blockNumber, bool _create, InodeType _type):
Block(_blockNumber, _create), blockNumberList((int*) &data[16]),
size((int*) &data[0]), type((int*) &data[4]), refcnt((int*) &data[8]),
singleIndirectBlockNumber((int*) &data[12]), maxDirectBlockNumber(BLOCK_SIZE / 4 - 4)
{
    indirectBlock = NULL;
    if (_create)
    {
        *size = 0;
        *type = _type;
        *refcnt = 1; //TODO : 0 or 1?
        *singleIndirectBlockNumber = -1;
        setChanged();
    }
}

Inode::~Inode()
{
    if (indirectBlock != NULL)
    {
        closeIndirectBlock();
    }
}

int Inode::createIndirectBlock(SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int blockNum = dataBitmap->getNextFree(*superBlock->nextFreeDataBitmapBlock, *superBlock->nextFreeDataBitmapByte);
    assert(blockNum != -1);
    dataBitmap->set(blockNum, false);
    setChanged();
    return blockNum;
}

void Inode::openIndirectBlock(SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    assert(indirectBlock == NULL);
    if (*singleIndirectBlockNumber == -1)
    {
        assert(superBlock != NULL);
        assert(dataBitmap != NULL);
        *singleIndirectBlockNumber = createIndirectBlock(superBlock, dataBitmap);
        indirectBlock = new Block(*singleIndirectBlockNumber, true);
    }
    else
        indirectBlock = new Block(*singleIndirectBlockNumber, false);
}

void Inode::closeIndirectBlock()
{
    assert(indirectBlock != NULL);
    delete indirectBlock;
    indirectBlock = NULL;
}

int Inode::indexToBlockNumber(int index)
{
    assert(index < (*size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    assert(index < maxDirectBlockNumber + BLOCK_SIZE / 4);
    if (index < maxDirectBlockNumber) // direct
    {
        return blockNumberList[index];
    }
    else
    {
        if (indirectBlock == NULL)
        {
            openIndirectBlock();
        }
        int* intData = (int*) indirectBlock->data;
        return intData[index - maxDirectBlockNumber];
    }
}

Block* Inode::inodeToBlock(int offset)
{
    int index = offset / BLOCK_SIZE;
    int blockNumber = indexToBlockNumber(index);
    return blockNumberToBlock(blockNumber);
}

void Inode::setBlockNumber(int offset, int _blockNum, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int index = offset / BLOCK_SIZE;
    assert(index < maxDirectBlockNumber + BLOCK_SIZE / 4);
    if (index < maxDirectBlockNumber) // direct
    {
        blockNumberList[index] = _blockNum;
        setChanged();
    }
    else
    {
        if (indirectBlock == NULL)
        {
            openIndirectBlock(superBlock, dataBitmap);
        }
        int* intData = (int*) indirectBlock->data;
        intData[index - maxDirectBlockNumber] = _blockNum;
        indirectBlock->setChanged();
    }
}

void Inode::append(const char* buf, int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    int restSize = (maxDirectBlockNumber + BLOCK_SIZE / 4) * BLOCK_SIZE - *size;
    assert(len <= restSize);
    while (len > 0)
    {
        int appendLen = BLOCK_SIZE - *size % BLOCK_SIZE;
        if (appendLen == BLOCK_SIZE)
        {
            if (appendLen > len)
                appendLen = len;
            int newBlockNumber = dataBitmap->getNextFree(*superBlock->nextFreeDataBitmapBlock, *superBlock->nextFreeDataBitmapByte);
            assert(newBlockNumber != -1);
            dataBitmap->set(newBlockNumber, false);
            setBlockNumber(*size, newBlockNumber, superBlock, dataBitmap);
            Block* b = new Block(newBlockNumber, true);
            memcpy(b->data, buf, appendLen);
            b->setChanged();
            delete b;
        }
        else
        {
            if (appendLen > len)
                appendLen = len;
            Block* b = inodeToBlock(*size);
            memcpy(b->data + (*size % BLOCK_SIZE), buf, appendLen);
            b->setChanged();
            delete b;
        }
        buf += appendLen;
        *size += appendLen;
        len -= appendLen;
    }
    setChanged();
}

void Inode::deleteIndirectBlock(SuperBlock* superBLock, BitmapMultiBlocks* dataBitmap)
{
    if (indirectBlock != NULL)
        closeIndirectBlock();
    dataBitmap->set(*singleIndirectBlockNumber, true);
    *singleIndirectBlockNumber = -1;
}

void Inode::truncate(int len, SuperBlock* superBlock, BitmapMultiBlocks* dataBitmap)
{
    if (len == *size)
        return;
    if (len > *size)
    {
        char* buf = new char[len - *size];
        append(buf, len - *size, superBlock, dataBitmap);
        delete[] buf;
    }
    else
    {
        while (len < *size)
        {
            int restBlockLen = *size % BLOCK_SIZE;
            if (restBlockLen == 0)
                restBlockLen = BLOCK_SIZE;
            int reduceLen = restBlockLen;
            if (reduceLen > *size - len)
                reduceLen = *size - len;
            if (reduceLen == restBlockLen)
            {
                int blockNum = indexToBlockNumber((*size - 1) / BLOCK_SIZE);
                dataBitmap->set(blockNum, true);
                setBlockNumber(*size - 1, -1, superBlock, dataBitmap);
            }
            else
            {
                //do nothing
            }
            *size -= reduceLen;
        }
        int index = (*size - 1) / BLOCK_SIZE;
        if ((index < maxDirectBlockNumber) && (*singleIndirectBlockNumber != -1))
            deleteIndirectBlock(superBlock, dataBitmap);
    }
    assert(len == *size);
    setChanged();
}

int Inode::newInodeBlockNum(SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    assert(*superBlock->inodeNumbers < MAX_INODE_NUMBERS);
    int blockNum = inodeBitmap->getNextFree(*superBlock->nextFreeInodeBitmapByte);
    assert(blockNum != -1);
    inodeBitmap->set(blockNum, false);
    (*superBlock->inodeNumbers)++;
    return blockNum;
}

Inode* Inode::inodeNumberToInode(int inodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    assert(inodeBitmap->isInodeNumberFree(inodeNumber) == false);
    return new Inode(inodeNumber + *superBlock->inodeBeginBlock, false);
}

Block* Inode::inodeNumberToBlock(int offset, int inodeNum, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    Inode* inode = inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    int index = offset / BLOCK_SIZE;
    int _blockNum = inode->indexToBlockNumber(index);
    delete inode;
    return blockNumberToBlock(_blockNum);
}

int Inode::nameToInodeNumber(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, int* offsetPtr)
{
    assert(fileName.length() > 0);
    if (fileName.length() > FILE_NAME_LEN)
        return -1; // must can not find
    assert(*type == directory);
    for (int offset = 0; offset < *size; offset += BLOCK_SIZE)
    {
        Block* b = inodeToBlock(offset);
        int inodeBlockNum = stringMatch(fileName, b, *size - offset, offsetPtr);
        if (inodeBlockNum != -1)
        {
            if (offsetPtr != NULL)
                *offsetPtr = *offsetPtr + offset;
            return inodeBlockNum - *superBlock->inodeBeginBlock;
        }
        delete b;
    }
    return -1;
}

int Inode::nameToInodeNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    Inode* inode = inodeNumberToInode(dirInodeNumber, superBlock, inodeBitmap);
    int ret = inode->nameToInodeNumber(fileName, superBlock, inodeBitmap);
    delete inode;
    return ret;
}

int Inode::nameToInodeBlockNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    int ret = nameToInodeNumber(fileName, dirInodeNumber, superBlock, inodeBitmap);
    if (ret == -1)
        return -1;
    return ret + *superBlock->inodeBeginBlock;
}

int Inode::createInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap, const InodeType inodeType, int inodeNumber)
{
    assert(*type == directory);
    assert(fileName.length() > 0);
    assert(fileName.length() <= FILE_NAME_LEN);
    int inodeBlockNumber;
    if (inodeNumber != -1) // link
        inodeBlockNumber = inodeNumber + *superBlock->inodeBeginBlock;
    else // new
        inodeBlockNumber = newInodeBlockNum(superBlock, inodeBitmap);
    char* buf = new char[FILE_NAME_LEN + 4];
    memcpy(buf, fileName.c_str(), fileName.length());
    if (fileName.length() < FILE_NAME_LEN)
        buf[fileName.length()] = '\0';
    *((int*)(buf + FILE_NAME_LEN)) = inodeBlockNumber;
    append(buf, FILE_NAME_LEN + 4, superBlock, dataBitmap);
    if (inodeNumber == -1)
    {
        Inode* newInode = new Inode(inodeBlockNumber, true, inodeType);
        delete newInode;
    }
    delete[] buf;
    setChanged();
    return inodeBlockNumber;
}

void Inode::deleteInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap)
{
    assert(*type == directory);
    assert(fileName.length() > 0);
    assert(fileName.length() <= FILE_NAME_LEN);
    int offset = -1;
    int inodeNumber = nameToInodeNumber(fileName, superBlock, inodeBitmap, &offset);
    Inode* inode = inodeNumberToInode(inodeNumber, superBlock, inodeBitmap);
    inode->truncate(0, superBlock, dataBitmap);
    (*inode->refcnt)--;
    inode->setChanged();
    Block* b = inodeToBlock(offset);
    memset(b->data + (offset % BLOCK_SIZE), 0, 16);
    b->setChanged();
    delete b;
    if (*inode->refcnt == 0)
    {
        (*superBlock->inodeNumbers)--;
        inodeBitmap->set(inodeNumber + *superBlock->inodeBeginBlock, true);
    }
    delete inode;
}

int Inode::createRootInode(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap)
{
    int inodeBlockNumber = newInodeBlockNum(superBlock, inodeBitmap);
    assert(inodeBlockNumber == *superBlock->inodeBeginBlock);
    Inode* rootInode = new Inode(inodeBlockNumber, true, directory);
    delete rootInode;
    return inodeBlockNumber;
}

int Inode::pathToInodeNumber(const std::string& fileName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    std::string str = fileName;
    while (true)
    {
        if (str == "/")
            return dirInodeNumber;
        int pos = str.find("/");
        if (pos == str.npos)
            return nameToInodeNumber(str, dirInodeNumber, superBlock, inodeBitmap);
        else
        {
            int nextDirInodeNumber = nameToInodeNumber(str.substr(0, pos), dirInodeNumber, superBlock, inodeBitmap);
            if (nextDirInodeNumber == -1)
                return -1;
            str = str.substr(pos + 1, str.length());
            dirInodeNumber = nextDirInodeNumber;
        }
    }
}

int Inode::absolutePathToInodeNumber(const std::string& fileName, SuperBlock* superBlock, Bitmap* inodeBitmap)
{
    assert(fileName.substr(0,1) == "/");
    if (fileName == "/")
        return 0;
    return pathToInodeNumber(fileName.substr(1), 0, superBlock, inodeBitmap);
}


void Inode::link(const std::string& srcName, const std::string& dstName, int dirInodeNumber, SuperBlock* superBlock, Bitmap* inodeBitmap, BitmapMultiBlocks* dataBitmap)
{
    //TODO : /xxxx or xxx/
    int srcInodeNumber = pathToInodeNumber(srcName, dirInodeNumber, superBlock, inodeBitmap);
    assert(srcInodeNumber != -1);
    Inode* srcInode = inodeNumberToInode(srcInodeNumber, superBlock, inodeBitmap);
    assert(*srcInode->type == file);
    delete srcInode;
    int pos = dstName.rfind("/");
    int dstFatherInodeNumber = dirInodeNumber;
    std::string dstFileName = dstName;
    if (pos != dstName.npos)
    {
        dstFatherInodeNumber = pathToInodeNumber(dstName.substr(0, pos), dirInodeNumber, superBlock, inodeBitmap);
        assert(dstFatherInodeNumber != -1);
        dstFileName = dstName.substr(pos + 1, dstName.length());
    }
    Inode* dstFatherInode = inodeNumberToInode(dstFatherInodeNumber, superBlock, inodeBitmap);
    assert(*dstFatherInode->type == directory);
    dstFatherInode->createInode(dstFileName, superBlock, inodeBitmap, dataBitmap, file, srcInodeNumber);
    delete dstFatherInode;
    srcInode = inodeNumberToInode(srcInodeNumber, superBlock, inodeBitmap);
    *srcInode->refcnt += 1;
    srcInode->setChanged();
    delete srcInode;
}

