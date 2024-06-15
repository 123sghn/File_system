#include "FreeBlockStack.h"

FreeBlockStack::FreeBlockStack() : maxSize(BLOCK_SIZE / (8 * sizeof(uint32_t))) {
}

uint32_t FreeBlockStack::getBlock() {
    uint32_t ret = blocks[stackTop];
    stackTop++;
    return ret;
}

void FreeBlockStack::revokeBlock(uint32_t block) {
    stackTop--;
    blocks[stackTop] = block;
}

uint32_t* FreeBlockStack::getBlocks() {
    return blocks;
}

uint32_t FreeBlockStack::getMaxSize() {
    return maxSize;
}

bool FreeBlockStack::empty() {
    return stackTop == maxSize;
}

bool FreeBlockStack::full() {
    return stackTop == 0;
}

void FreeBlockStack::setStackTop(uint32_t st) {
    stackTop = st;
}
