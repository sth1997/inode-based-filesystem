#ifndef CONST_H_
#define CONST_H_

const int BLOCK_SIZE = 4096;
const static char* DATA_PATH = "/tmp/fsdata/";
const int MAX_DATA_BLOCK_NUMBERS = 1024 * 1024;
const int MAX_INODE_NUMBERS = 1024; // only use one block to save inode bitmap

#endif //CONST_H_