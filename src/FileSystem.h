#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <iostream>
#include <cstdint>
#include <cstring>
#include "DiskDriver.h"
#include "./entity/FileSystemInfo.h" //超级块对象
#include "./entity/INode.h" //i节点:存放文件说明信息
#include "./entity/Directory.h" //目录项和类
#include "./entity/FreeBlockStack.h" //空闲块

//基本文件系统，实现对于文件的管理
class FileSystem {
public:
    FileSystem(); //构造函数，创建空闲块，磁盘打开标记设置为false
    ~FileSystem(); //析构函数，将更新后的systemInfo写入磁盘

    bool createDisk(uint32_t sz); //创建一个指定大小的磁盘，单位为Byte
    bool format(uint16_t bsize); //指定块大小，进行格式化，单位Byte
    bool mount(); //尝试挂载硬盘，若挂载失败则需要格式化

    uint32_t blockAllocate(); //分配空闲磁盘块
    void blockFree(uint32_t bno); //回收磁盘块

    void read(uint32_t bno, uint16_t offset, char* buf, uint16_t sz); //从磁盘块bno偏移offset开始读sz字节到缓冲区buf
    void write(uint32_t bno, uint16_t offset, const char* buf, uint16_t sz); //从磁盘块bno偏移offset开始覆盖写入缓冲区buf开始sz字节
    void readNext(char* buf, uint16_t sz); //从当前位置继续读取数据
    void writeNext(char* buf, uint16_t sz); //从当前位置继续写入数据
    void locale(uint32_t bno, uint16_t offset); //将读写头移动到bno磁盘块的offset偏移

    uint8_t userVerify(std::string userName, std::string password); //用户身份认证,若认证成功返回非0的uid，否则返回0
    uint8_t duplicateDetection(std::string userName); //用户身份认证,若认证成功返回非0的uid，否则返回0
    int8_t emptyDetection(); //查找用户列表0-7中的第一个空位置，没空位置则返回最大用户数，表示用户列表已满
    void useradd(int8_t idx, std::string name, std::string password); //添加用户
    void userdel(uint8_t uid); //删除用户
    void passwd(uint8_t uid, std::string password); //修改某个用户的密码

    bool grantTrustUser(std::string currentUser, std::string targetUser); //添加信任用户组
    bool revokeTrustUser(std::string currentUser, std::string targetUser); //收回信任用户组
    uint8_t verifyTrustUser(std::string currentUser, std::string targetUser); //收回信任用户组
    uint8_t verifyTrustUser(uint8_t currentUserUid, uint8_t targetUserUid); //查询对于current来说target是否为信任用户，1为信任0为不信任
    void getUser(uint8_t uid, User* user); //根据uid读取用户信息

    uint32_t getRootLocation(); //读取根目录所在磁盘块
    void update(); //更新信息

private:
    DiskDriver disk; //虚拟磁盘对象
    uint32_t capacity; //读取到的磁盘容量
    int8_t isUnformatted; //未格式化标记，-1未格式化，0已经格式化
    uint16_t blockSize; //块大小
    bool isOpen; //磁盘是否打开标记
    FileSystemInfo systemInfo; //文件系统超级块
    FreeBlockStack* stack; //空闲块栈，使用指针是为了防止写入硬盘时占用空间

};

#endif //FILESYSTEM_FILESYSTEM_H
