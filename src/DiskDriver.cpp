#include "DiskDriver.h"

std::string DiskDriver::diskName = "./disk.dsk";

DiskDriver::DiskDriver() {
    isOpen = false;
}

DiskDriver::~DiskDriver() {
    if (isOpen) {
        disk.close();
    }
}

bool DiskDriver::open() {
    if (isOpen) {
        return true;
    }
    std::ifstream t(diskName); //使用读入流来判断文件是否存在
    if (t.is_open()) {
        t.close();
        disk.open(diskName, std::ios::in | std::ios::out | std::ios::binary); //以读、写、二进制形式打开文件流对象
        isOpen = true;
        return true;
    }
    return false;
}

bool DiskDriver::close() {
    if (!isOpen) {
        return true;
    }
    disk.close();
    isOpen = false;
    return true;
}

bool DiskDriver::init(uint32_t sz) {
    if (isOpen) {
        return false;
    }
    std::ofstream c(diskName, std::ios::binary | std::ios::out); //以二进制和写入（输出）的形式创建输出文件流对象
    //填充虚拟磁盘
    c.seekp(0, std::ios::beg); //将写入位置设置为文件开头
    for (uint32_t i = 0; i < sz; ++i) {
        c.write("\0", 1); //写入一个字节的空数据，重复 sz 次
    }
    c.seekp(0, std::ios::beg); //将写入位置重新设置为文件开头
    c.write(reinterpret_cast<char*>(&sz), sizeof(sz)); //写入文件大小信息
    int8_t ok = -1; //未格式化标记
    c.write(reinterpret_cast<char*>(&ok), sizeof(ok)); //写入未格式化标记信息
    c.close();
    return true;
}

void DiskDriver::seekStart(uint32_t sz) {
    disk.seekp(sz, std::ios::beg);
    //std::cout<<cursor<<std::endl;
}

void DiskDriver::seekCurrent(uint32_t sz) {
    disk.seekp(sz, std::ios::cur);
    //std::cout<<cursor<<std::endl;
}

void DiskDriver::read(char* buf, uint32_t sz) {
    disk.read(buf, sz);
    disk.flush(); //刷新输出流，实时显示
    disk.clear(); //清除文件流的错误状态和文件尾标记
}

void DiskDriver::write(const char* buf, uint32_t sz) {
    disk.write(buf, sz);
    disk.flush();
    disk.clear();
}
