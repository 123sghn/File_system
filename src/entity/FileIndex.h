#ifndef FILESYSTEM_FILEINDEX_H
#define FILESYSTEM_FILEINDEX_H

#include <cstdint>
#include "../Constraints.h"

//文件索引表
class FileIndex {
public:
    uint32_t index[FILE_INDEX_SIZE]; //数据存放的磁盘块号，4092
    uint32_t next; //下一个索引的磁盘块号，支持大文件，没有为0
};

#endif //FILESYSTEM_FILEINDEX_H
