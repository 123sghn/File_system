#ifndef FILESYSTEM_INODE_H
#define FILESYSTEM_INODE_H

#include <cstdint>
#include <string>
#include <ctime>
#include <iomanip>

class INode {
public:
    uint8_t uid; //所属用户ID，默认文件创建者就是文件所有者，拥有该文件所有权限
    uint8_t flag; //高2位00表示文件，01表示目录，10表示软链接，中间3位以rwx格式表示信赖者的访问权限，低3位表示其余用户访问权限
    uint32_t bno; //该文件所在磁盘块号
    char creationTime[25];
    char modifiedTime[25];

    static std::string getCurTime(long long x = std::time(0)) {
        time_t now = x;
        tm* clk = localtime(&now);
        std::stringstream os;
        os << 1900 + clk->tm_year << '-' << std::setw(2) << std::setfill('0') << 1 + clk->tm_mon << '-' << std::setw(2) << std::setfill('0') << clk->tm_mday << ' ';
        os << std::setw(2) << std::setfill('0') << clk->tm_hour << ':' << std::setw(2) << std::setfill('0') << clk->tm_min << ':' << std::setw(2) << std::setfill('0') << clk->tm_sec;
        return os.str();
    }
};

#endif //FILESYSTEM_INODE_H
