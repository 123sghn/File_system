#ifndef FILESYSTEM_FILESYSTEMINFO_H
#define FILESYSTEM_FILESYSTEMINFO_H

#include <cstdint>
#include "User.h"

//超级块对象
class FileSystemInfo {
public:
    uint32_t rootLocation; //根目录所在磁盘块

    uint32_t freeBlockNumber; //空闲块个数
    uint32_t freeBlockStackTop; //空闲块栈的栈顶（栈底根据块大小和磁盘大小可以计算）
    uint16_t freeBlockStackOffset; //空闲块栈栈顶指针所在的块内偏移

    uint32_t avaliableCapacity; //磁盘可用容量

    User users[MAX_USER_NUMS]; //用户列表，最多为8
    uint8_t trustMatrix[MAX_USER_NUMS][MAX_USER_NUMS]; //信赖者矩阵，trustMatrix[i][j]=1代表i信赖j

    uint8_t flag; //超级块修改标记，-1未被修改，0已经被修改
};

#endif //FILESYSTEM_FILESYSTEMINFO_H
