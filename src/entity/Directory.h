#ifndef FILESYSTEM_DIRECTORY_H
#define FILESYSTEM_DIRECTORY_H

//目录项
class DirectoryItem {
public:
    uint32_t inodeIndex; //本目录项指向的i节点所在的磁盘块号
    char name[FILE_NAME_LENGTH]; //文件名/目录名
};

//目录类，一个目录恰好是一个块大小
class Directory {
public:
    DirectoryItem item[DIRECTORY_NUMS]; //32768/128=256
    //从头开始遍历第一个index==0的项为空闲目录项
};

#endif //FILESYSTEM_DIRECTORY_H
