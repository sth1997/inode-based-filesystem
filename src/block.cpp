#include "../include/block.h"
#include "const.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <cstdio>

Block::Block(int _blockNum, bool _newBlock)
{
    this->blockNum = _blockNum;
    changed = false;
    data = new char[BLOCK_SIZE];
    if (!_newBlock)
    {
        char fileName[64];
        sprintf(fileName, "%s%d", DATA_PATH, this->blockNum);
        int fd = open(fileName, O_RDONLY);
        ssize_t bytes = read(fd, data, BLOCK_SIZE);
        assert(bytes == BLOCK_SIZE);
        close(fd);
    }
    else
    {
        setChanged();
        system("rm -rf /tmp/fsdata/");
        system("mkdir -p /tmp/fsdata/");
    }
    
}

Block::~Block()
{
    if (changed)
    {
        char fileName[64];
        sprintf(fileName, "%s%d", DATA_PATH, this->blockNum);
        int fd = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0777);
        assert(fd != -1);
        ssize_t bytes = write(fd, data, BLOCK_SIZE);
        assert(bytes == BLOCK_SIZE);
        close(fd);
    }
    delete[] this->data;
}