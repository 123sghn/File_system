#ifndef FILESYSTEM_USERINTERFACE_H
#define FILESYSTEM_USERINTERFACE_H

#include <cstdio>
#include <iostream>
#include <cstring>
#include <vector>

#include "FileSystem.h"
#include "Constraints.h"
#include "./entity/FileIndex.h"
#include "./entity/Vim.h"
#include "Tools.h"

// 为用户提供的接口，支持用户常用的功能
class UserInterface {
public:
    void initialize(); //初始化

    bool logout(); //一个用户退出后的处理

    std::pair<bool, int> cd(uint8_t uid, std::string directoryName, const std::string& initCmd = std::string()); //cd命令接口,进入当前目录的文件夹，返回切换是否成功和错误类型
    bool ls(uint8_t uid, bool all, const std::string& initCmd); //ls命令接口,显示当前目录所有文件信息
    bool ls(uint8_t uid, bool all, std::vector<std::string> src, const std::string& initCmd); //ls命令接口,src指出的目录的所有文件信息

    bool touch(uint8_t uid, std::string fileName, const std::string& initCmd); //touch命令接口,创建文件
    bool touch(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd); //touch命令接口,根据src路径创建文件
    bool cat(uint8_t uid, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* returnContent = nullptr); //cat命令接口,打印文件
    bool cat(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* returnContent = nullptr); //cat命令接口,根据src路径打印文件
    bool vim(uint8_t uid, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* inputContent = nullptr); //vim命令接口,编辑文件
    bool vim(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* inputContent = nullptr); //vim命令接口,根据src路径编辑文件

    bool mv(uint8_t uid, std::vector<std::string> src, std::vector<std::string> des, const std::string& initSrc, const std::string& initDes); //mv命令接口,移动文件
    bool cp(uint8_t uid, std::vector<std::string> src, std::vector<std::string> des, const std::string& initSrc, const std::string& initDes); //cp命令接口,复制文件
    bool rm(uint8_t uid, std::string fileName, const std::string& initCmd); //rm命令接口,删除文件
    bool rm(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd); //rm命令接口,根据src路径删除文件

    bool mkdir(uint8_t uid, std::string directoryName, const std::string& initCmd); //mkdir命令接口,创建目录
    bool mkdir(uint8_t uid, std::vector<std::string> src, std::string dirName, const std::string& initCmd); //mkdir命令接口,根据src指出的路径创建目录
    bool rmdir(uint8_t uid, std::string dirName, const std::string& initCmd); //rmdir命令接口,删除文件夹
    bool rmdir(uint8_t uid, std::vector<std::string> src, std::string dirName, const std::string& initCmd); //rmdir命令接口,根据src路径删除文件夹

    bool format(); //format命令接口,格式化整个文件系统
    bool chmod(uint8_t uid, std::string name, std::string who, std::string access, const std::string& initCmd); //chmod命令接口,修改文件或目录权限
    bool chmod(uint8_t uid, std::vector<std::string> src, std::string name, std::string who, std::string access, const std::string& initCmd); //chmod命令接口,根据src路径修改文件或目录权限

    bool useradd(uint8_t uid, std::string name); //useradd命令接口，添加用户
    bool userdel(uint8_t uid, std::string name); //userdel命令接口，删除用户
    bool userlist(); //userlist命令接口，显示用户列表
    bool passwd(uint8_t uid, std::string name); //passwd命令接口，修改当前用户密码
    bool trust(uint8_t uid, std::string currentUser, std::string targetUser); //trust命令接口，添加当前用户的信任用户
    bool distrust(uint8_t uid, std::string currentUser, std::string targetUser); //distrust命令接口，删除当前用户的信任用户

    void updateDirNow(); //更新当前目录信息

    void goToRoot(); //进入根目录
    void getUser(uint8_t uid, User* user); //根据uid提取用户信息
    uint8_t userVerify(std::string username, std::string password); //用户鉴别，鉴别成功返回uid，否则返回0
    void setSudoMode(const bool& sudo); //设置是否是sudo模式
    void setCurrentCmd(const std::string& cmd); //设置当前的命令（用于反馈错误信息）

private:
    Directory directory;//当前目录
    uint32_t nowDiretoryDisk;//当前目录所在磁盘块号
    bool sudoMode;
    std::string currentCmd;
    FileSystem fileSystem;

    //非接口函数设为私有，不让上层调用
    std::pair<uint32_t, int> findDisk(uint8_t uid, std::vector<std::string> src); //从当前目录开始,根据src数组提供的路径,找到对应文件或者目录所在的目录所在的磁盘块号和该文件或者目录的i结点所在的目录项序号

    uint32_t readFileBlock(uint32_t disk, std::string& content); //读取一整个FileIndex的内容
    std::string readFile(uint32_t startDisk); //读取首个FileIndex的链表的所有内容
    uint32_t writeFileBlock(uint32_t disk, const std::string& content); //写一整个FileIndex的内容
    uint32_t writeFile(std::string content); //写一整个文件
    uint32_t freeFileBlock(uint32_t disk); //回收一整个FileIndex的块
    void freeFile(uint32_t startDisk); //回收一整个文件

    void wholeDirItemsMove(int itemLocation); //将从指定位置开始的目录项整体前移
    bool duplicateDetection(std::string name); //重复名检测
    int judge(uint32_t disk); //判断i结点指向的是目录还是文件,目录1,文件0

    bool checkReadAccess(uint8_t uid, INode tar); //判断用户uid对tar的iNode节点是否有读权限
    bool checkWriteAccess(uint8_t uid, INode tar); //判断用户uid对tar的iNode节点是否有写权限
    bool checkOwnerAccess(uint8_t uid, INode tar); //判断用户uid对tar的iNode节点是否有所有者权限

    std::string printFixedLength(const char* str, int maxLen); //将字符转换成maxLen长度，首尾补等量空格

};

#endif //FILESYSTEM_USERINTERFACE_H
