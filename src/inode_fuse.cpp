/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <string>
#include "../include/inode.h"
#include "../include/super_block.h"
#include "../include/common.h"
#include <assert.h>
#include <fstream>
using namespace std;

static ofstream log;
static SuperBlock* superBlock;
static Bitmap* inodeBitmap;
static BitmapMultiBlocks* dataBitmap;
static int rootInodeNumber;

static string getFileNameFromPath(const string& pathStr)
{
    int pos = pathStr.rfind("/");
    assert(pos != pathStr.npos);
    return pathStr.substr(pos + 1);
}

static string getParentPath(const string& pathStr)
{
    int pos = pathStr.rfind("/");
    assert(pos != pathStr.npos);
    if (pos == 0) // "/"
        return string("/");
    return pathStr.substr(0, pos);
}

static int inode_opendir(const char* path, struct fuse_file_info *fi)
{
    log << "opendir " << path << endl;
    log.flush();
    string cwdStr = string(path);
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    log << "inodeNum = " << inodeNum << endl;
    log.flush();
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    assert(*cwdInode->type == directory);
    log << "opendir successful" << endl;
    log.flush();
    delete cwdInode;
    return 0;
}

static int inode_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi)
{
    log << "readdir " << path << endl;
    log.flush();
    struct stat st;
    st.st_mode = S_IFDIR;
    filler(buf, ".", &st, 0);
    filler(buf, "..", &st, 0);
    string cwdStr = string(path);
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    for (int offset = 0; offset < *cwdInode->size; offset += BLOCK_SIZE)
    {
        Block* b = cwdInode->inodeToBlock(offset);
        int len = BLOCK_SIZE;
        if (*cwdInode->size - offset < BLOCK_SIZE)
            len = *cwdInode->size - offset;
        log << "len = " << len << endl;
        log.flush();
        for (int i = 0; i < len; i += 16)
        {
            if (b->data[i] == '\0') // file has been deleted
                continue;
            char fileName[FILE_NAME_LEN + 1];
            memcpy(fileName, b->data + i, FILE_NAME_LEN);
            fileName[FILE_NAME_LEN] = '\0';
            int blockNum = *((int*)(b->data + i + FILE_NAME_LEN));
            log << "fileName = " << fileName << endl;
            log.flush();
            Inode* newFile = new Inode(blockNum, false);
            if (*newFile->type == directory)
                st.st_mode = S_IFDIR | 0666;
            else if (*newFile->type == file)
                st.st_mode = S_IFREG | 0666;
            else
                st.st_mode = S_IFLNK | 0666;
            delete newFile;
            log << "st_mode = " << st.st_mode << endl;
            log.flush();
            filler(buf, fileName, &st, 0);
        }
        delete b;
    }
    log << "readdir successful" << endl;
    log.flush();
    delete cwdInode;
    return 0;
}

static int inode_mkdir(const char *path, mode_t mode)
{
    log << "mkdir " << path << endl;
    log.flush();
    string fileName = getFileNameFromPath(string(path));
    string cwdStr = getParentPath(string(path));
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    log << "filename = " << fileName << endl;
    log << "inode Block num = " << cwdInode->getBlockNum() << "   " << *superBlock->inodeBeginBlock << endl;
    log.flush();
    cwdInode->createInode(fileName, superBlock, inodeBitmap, dataBitmap, directory);
    log << "mkdirdir successful" << endl;
    log.flush();
    delete cwdInode;
    return 0;
}

static int inode_rmdir(const char *path)
{
    log << "rmdir " << path << endl;
    log.flush();
    string fileName = getFileNameFromPath(string(path));
    string cwdStr = getParentPath(string(path));
    log << "fileName = " << fileName << "parentPath = " << cwdStr << endl;
    log.flush();
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    cwdInode->deleteInode(fileName, superBlock, inodeBitmap, dataBitmap);
    delete cwdInode;
    return 0;
}

static int inode_unlink(const char *path)
{
    string fileName = getFileNameFromPath(string(path));
    string cwdStr = getParentPath(string(path));
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    cwdInode->deleteInode(fileName, superBlock, inodeBitmap, dataBitmap);
    delete cwdInode;
    return 0;
}

static int inode_getattr(const char *path, struct stat *stbuf)
{
    log << "getattr " << path << endl;
    log.flush();
    Inode* inode = NULL;
    memset(stbuf, 0, sizeof(struct stat));
    int inodeNum = Inode::absolutePathToInodeNumber(path, superBlock, inodeBitmap);
    if (inodeNum == -1)
        return -ENOENT;
    inode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    stbuf->st_ino = inode->getBlockNum() - *superBlock->inodeBeginBlock;
    if (*inode->type == directory)
        stbuf->st_mode = S_IFDIR | 0666;
    else if(*inode->type == file)
        stbuf->st_mode = S_IFREG | 0666;
    else
        stbuf->st_mode = S_IFLNK | 0666;
    
    stbuf->st_nlink = *inode->refcnt;
    stbuf->st_size = *inode->size;
    stbuf->st_blocks = (*inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    stbuf->st_blksize = BLOCK_SIZE;

    delete inode;
    return 0;
}

static int inode_release(const char *path, struct fuse_file_info *fi)
{
    // do nothing
    return 0;
}

static int inode_open(const char *path, struct fuse_file_info *fi)
{
    // do nothing
    return 0;
}

static int inode_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    log << "create " << path << endl;
    log.flush();
    string fileName = getFileNameFromPath(string(path));
    string cwdStr = getParentPath(string(path));
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    cwdInode->createInode(fileName, superBlock, inodeBitmap, dataBitmap, file);
    delete cwdInode;
    return 0;
}

static int inode_truncate(const char *path, off_t size)
{
    log << "truncate " << path << "  size = " << size << endl;
    log.flush();
    int inodeNum = Inode::absolutePathToInodeNumber(string(path), superBlock, inodeBitmap);
    if (inodeNum == -1)
    {
        log << "File " << path << " not found." << endl;
        log.flush();
        return -ENOENT;
    }
    Inode* inode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    if (*inode->type != file)
    {
        log << "truncate type != file" << endl;
        log.flush();
        assert(0);
    }
    inode->truncate(size, superBlock, dataBitmap);
    delete inode;
    return 0;
}

static int inode_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    log << "read " << path << "  size = " << size << "  offset = " << offset << endl;
    log.flush();
    int inodeNum = Inode::absolutePathToInodeNumber(string(path), superBlock, inodeBitmap);
    if (inodeNum == -1)
    {
        log << "File " << path << " not found." << endl;
        log.flush();
        return -ENOENT;
    }
    Inode* inode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    if (*inode->type != file)
    {
        log << "read type != file" << endl;
        log.flush();
        assert(0);
    }
    if (*inode->size <= offset)
    {
        delete inode;
        return 0;
    }

    if (*inode->size - offset < size)
        size = *inode->size - offset;
    int restBlockSize = 0;
    int readBytes = 0;
    for (; (offset < *inode->size) && (size != 0); offset += restBlockSize, size -= restBlockSize, buf += restBlockSize, readBytes += restBlockSize)
    {
        Block* b = inode->inodeToBlock(offset);
        restBlockSize = BLOCK_SIZE - offset % BLOCK_SIZE;
        if (restBlockSize > size)
            restBlockSize = size;
        memcpy(buf, b->data + (offset % BLOCK_SIZE), restBlockSize);
        delete b;
    }
    delete inode;
    log << "Read " << readBytes << " bytes." << endl;
    log.flush();
    return readBytes;
}

static int inode_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    log << "write " << path << "  size = " << size << "  offset = " << offset << endl;
    log.flush();
    int inodeNum = Inode::absolutePathToInodeNumber(string(path), superBlock, inodeBitmap);
    if (inodeNum == -1)
    {
        log << "File " << path << " not found." << endl;
        log.flush();
        return -ENOENT;
    }
    Inode* inode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    if (*inode->type != file)
    {
        log << "write type != file" << endl;
        log.flush();
        assert(0);
    }
    if (*inode->size < offset)
    {
        log << "Write offset > inode size" << endl;
        log.flush();
        assert(0);
    }

    int writeBytes = 0;
    int restBlockSize = 0;
    for (; (offset < *inode->size) && (size != 0); offset += restBlockSize, size -= restBlockSize, buf += restBlockSize, writeBytes += restBlockSize)
    {
        Block* b = inode->inodeToBlock(offset);
        restBlockSize = BLOCK_SIZE - offset % BLOCK_SIZE;
        if (restBlockSize > size)
            restBlockSize = size;
        if (restBlockSize > (*inode->size - offset))
            restBlockSize = *inode->size - offset;
        memcpy(b->data + (offset % BLOCK_SIZE), buf, restBlockSize);
        delete b;
    }
    if (size != 0)
    {
        inode->append(buf, size, superBlock, dataBitmap);
        writeBytes += size;
    }
    delete inode;
    log << "Write " << writeBytes << " bytes." << endl;
    log.flush();
    return writeBytes;
}

static int inode_link(const char *from, const char *to)
{
    log << "link " << from << "  " << to << endl;
    log.flush();
    Inode::link(string(from + 1), string(to + 1), 0, superBlock, inodeBitmap, dataBitmap);
    return 0;
}


static int inode_rename(const char *from, const char *to)
{
    log << "rename" << from << "  " << to << endl;
    log.flush();
    string fileName = getFileNameFromPath(string(from));
    string cwdStr = getParentPath(string(from));
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    int offset = -1;
    int oldInodeNum = cwdInode->nameToInodeNumber(fileName, superBlock, inodeBitmap, &offset);
    Block* b = cwdInode->inodeToBlock(offset);
    memset(b->data + (offset % BLOCK_SIZE), 0, 16);
    b->setChanged();
    delete b;
    delete cwdInode;
    Inode* oldInode = Inode::inodeNumberToInode(oldInodeNum, superBlock, inodeBitmap);
    InodeType oldInodeType = *oldInode->type;
    delete oldInode;

    fileName = getFileNameFromPath(string(to));
    cwdStr = getParentPath(string(to));
    inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    cwdInode->createInode(fileName, superBlock, inodeBitmap, dataBitmap, oldInodeType, oldInodeNum);
    delete cwdInode;
    return 0;
}

static int inode_readlink(const char *path, char *buf, size_t size)
{
    int inodeNum = Inode::absolutePathToInodeNumber(string(path), superBlock, inodeBitmap);
    if (inodeNum == -1)
    {
        log << "File " << path << " not found." << endl;
        log.flush();
        return -ENOENT;
    }
    Inode* inode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    if (*inode->type != slink)
    {
        log << "readlink type != symlink" << endl;
        log.flush();
        assert(0);
    }

    size = *inode->size;
    Block* b = inode->inodeToBlock(0);
    memcpy(buf, b->data, size);
    delete b;
    delete inode;

    buf[size] = '\0';
    return 0;
}

static int inode_symlink(const char *from, const char *to)
{
    log << "symlink " << from << "  " << to << endl;
    log.flush();
    string fileName = getFileNameFromPath(string(to));
    string cwdStr = getParentPath(string(to));
    int inodeNum = Inode::absolutePathToInodeNumber(cwdStr, superBlock, inodeBitmap);
    Inode* cwdInode = Inode::inodeNumberToInode(inodeNum, superBlock, inodeBitmap);
    int newBlockNum = cwdInode->createInode(fileName, superBlock, inodeBitmap, dataBitmap, slink);
    Inode* newInode = Inode::inodeNumberToInode(newBlockNum - *superBlock->inodeBeginBlock, superBlock, inodeBitmap);
    newInode->append(from, strlen(from), superBlock, dataBitmap);
    delete newInode;
    delete cwdInode;
    return 0;
}

static struct fuse_operations inode_oper; 

int main(int argc, char *argv[])
{
    inode_oper.getattr    = inode_getattr;
    inode_oper.opendir = inode_opendir;
    inode_oper.readdir = inode_readdir;
    inode_oper.mkdir = inode_mkdir;
    inode_oper.rmdir = inode_rmdir;
    inode_oper.unlink = inode_unlink;
    inode_oper.release = inode_release;
    inode_oper.open = inode_open;
    inode_oper.create = inode_create;
    inode_oper.truncate = inode_truncate;
    inode_oper.read = inode_read;
    inode_oper.write = inode_write;
    inode_oper.rename = inode_rename;
    inode_oper.link = inode_link;
    inode_oper.readlink    = inode_readlink;
    inode_oper.symlink    = inode_symlink;

    log.open("inode_fuse.log");

    createSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
    rootInodeNumber = Inode::createRootInode("/", superBlock, inodeBitmap, dataBitmap);
    umask(0);
    int ret = fuse_main(argc, argv, &inode_oper, NULL);
    deleteSuperBlockAndBitmaps(superBlock, inodeBitmap, dataBitmap);
    return ret;
}
