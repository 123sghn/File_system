#ifndef FILESYSTEM_FREEBLOCKSTACK_H
#define FILESYSTEM_FREEBLOCKSTACK_H

#include <cstdint>
#include <iostream>
#include "../Constraints.h"

class FreeBlockStack {
public:
    FreeBlockStack();
    uint32_t getBlock(); //获取空闲块
    void revokeBlock(uint32_t block); //回收空闲块，置于栈顶
    uint32_t* getBlocks(); //返回指向栈的指针
    uint32_t getMaxSize(); //返回栈的最大大小
    bool empty(); //判断栈是否为空
    bool full(); //判断是否已满
    void setStackTop(uint32_t st); //设置栈顶指针的值

private:
    const uint32_t maxSize; //栈大小
    uint32_t stackTop; //栈顶指针
    uint32_t blocks[BLOCK_SIZE / (8 * sizeof(uint32_t))]; //栈本体，占一个块大小
};

#endif //FILESYSTEM_FREEBLOCKSTACK_H
