#ifndef BLOCK_H_
#define BLOCK_H_

class Block
{
private:
    int blockNum;
    bool changed;
public:
    char* data;
    Block(int _blockNum, bool _newBlock);
    ~Block();
    void setChanged() {changed = true;}
    bool getChanged() {return changed;}
};

#endif //BLOCK_H_