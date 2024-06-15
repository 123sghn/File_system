#ifndef FILESYSTEM_DISKDRIVER_H
#define FILESYSTEM_DISKDRIVER_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

//模拟磁盘，支持挂载磁盘模拟文件、读写头前后移动（以字节为单位）、初始化磁盘功能
class DiskDriver {
public:
    DiskDriver(); //构造函数，将打开标记初始化为未打开
    ~DiskDriver(); //析构函数，退出的时候将打开标记设置为未打开

    bool open(); //打开虚拟磁盘文件，返回是否打开成功
    bool close(); //关闭虚拟磁盘文件，返回是否关闭
    bool init(uint32_t sz); //创建未格式化的指定容量的虚拟磁盘文件，单位为Byte
    void seekStart(uint32_t sz); //将读写头移动到距起始sz字节处
    void seekCurrent(uint32_t sz); //将读写头移动到距当前位置sz字节处
    void read(char* buf, uint32_t sz); //从当前位置读出sz字节到buf缓冲区
    void write(const char* buf, uint32_t sz); //从当前位置将sz字节写入文件

private:
    static std::string diskName; //虚拟磁盘文件名
    std::fstream disk; //C++文件对象模拟磁盘，同时起到读写头的作用
    bool isOpen; //磁盘是否打开标记
};

#endif //FILESYSTEM_DISKDRIVER_H
