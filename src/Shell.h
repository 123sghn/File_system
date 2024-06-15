#ifndef FILESYSTEM_SHELL_H
#define FILESYSTEM_SHELL_H

#include <string>
#include <map>
#include <iostream>
#include <tuple>
#include <sstream>
#include <vector>

#include "UserInterface.h"
#include "entity/User.h"
#include "Tools.h"

//一个命令行界面，实现类似Linux的命令行交互
class Shell {
public:
    Shell();

    //根据part分割str
    std::vector<std::string> split_path(std::string path); //路径划分 data/ztr/sghn->data   ztr    sghn
    std::pair<bool, std::vector<std::string>> split_cmd(std::string& path); //cmd划分  cp /data1/1.txt /data2/1.txt->cp  /data1/1.txt     /data2/1.txt

    //界面主程序
    void exec(); //退出终端的具体实现
    void cmd_login(); //登录
    void cmd_logout(); //logout命令处理程序
    void cmd_exit(); //exit命令处理程序

    void cmd_cd(); //cd命令处理程序
    void cmd_ls(); //ls命令处理程序

    void cmd_touch(); //touch命令处理程序
    void cmd_cat(); //cat命令处理程序
    void cmd_vim(); //vim命令处理程序

    void cmd_mv(); //mv命令处理程序
    void cmd_cp(); //cp命令处理程序
    void cmd_rm(); //rm命令处理程序

    void cmd_mkdir(); //mkdir命令处理程序
    void cmd_rmdir(); //rmdir命令处理程序

    bool cmd_sudo();
    void cmd_format(); //format命令处理程序
    void cmd_chmod(); //chmod命令处理程序

    void cmd_useradd(); //useradd命令处理程序
    void cmd_userdel(); //userdel命令处理程序
    void cmd_userlist(); //userlist命令处理程序
    void cmd_passwd(); //passwd命令处理程序
    void cmd_trust(); //trust命令处理程序
    void cmd_distrust(); //distrust命令处理程序

    void cmd_help(); //help命令处理程序
    void cmd_clear(); //clear命令处理程序

    const std::vector<std::string>& getCmd() const;
    void setCmd(const std::vector<std::string>& cmd);

    void outputPrefix(); //输出当前文件系统的命令提示符

private:
    std::vector<std::string> cmd; //用户输入的整行命令
    User user; //当前登录用户
    bool isExit; //是否退出标记
    bool isSudo; //是否是超管状态
    UserInterface userInterface; //用户接口
    std::vector<std::string> curPath; //当前从根目录开始的路径
    std::map<std::string, std::string> helpInform; //帮助文档
};

#endif //FILESYSTEM_SHELL_H
