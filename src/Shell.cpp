#include "Shell.h"

Shell::Shell() {
    user.uid = 0;
    isExit = false;

    //helpInform["Command"] = "Usage               Interpret";
    helpInform["cat"] = "cat <FILE>                    concatenate and display files";
    helpInform["cd"] = "cd <DIR>                      change the shell working directory";
    helpInform["chmod"] = "chmod <FILE> -a|t|o [r][w][x] change file modes or Access Control Lists";
    helpInform["clear"] = "clear                         clear the terminal screen";
    helpInform["cp"] = "cp <SOURCE> <DEST>            copy files and directories";
    helpInform["exit"] = "exit                          exit the shell";
    helpInform["format"] = "format                        format disks or tapes";
    helpInform["help"] = "help                          display information about builtin commands";
    helpInform["logout"] = "logout                        exit a login shell";
    helpInform["ls"] = "ls [-a] [<DIR>]               list directory contents";
    helpInform["mkdir"] = "mkdir <DIR>                   make directories";
    helpInform["mv"] = "mv <SOURCE> <DEST>            move (rename) files";
    helpInform["passwd"] = "passwd                        update user's authentication tokens";
    helpInform["rm"] = "rm <FILE>                     remove files or directories";
    helpInform["rmdir"] = "rmdir <DIR>                   remove empty directories";
    helpInform["sudo"] = "sudo <COMMAND>                execute a command as another user";
    helpInform["touch"] = "touch <FILE>                  change file timestamps";
    helpInform["useradd"] = "useradd <USERNAME>            create a new user or update default new user information";
    helpInform["userdel"] = "userdel <USERNAME>            delete a user account and related files";
    helpInform["vim"] = "vim <FILE>                    a programmer's file editor";
    helpInform["userlist"] = "userlist                      display a list of system users";
    helpInform["trust"] = "trust <USERNAME>              add a user to the trusted list";
    helpInform["distrust"] = "distrust <USERNAME>           remove a user from the trusted list";
}

std::vector<std::string> Shell::split_path(std::string path) {
    if (path == "~") {
        return std::vector<std::string>{""};
    }

    std::vector<std::string> paths;
    path += '/'; //方便程序处理
    std::string item = "";
    bool occur = false; //判断是否出现路径符号/，多个斜杠算作一个
    for (char ch : path) {
        if (ch == '/') {
            if (!occur) {
                paths.push_back(item);
                item = "";
                occur = true;
            }
            else {
                continue;
            }
        }
        else {
            occur = false;
            item += ch;
        }
    }
    return paths;
}

std::pair<bool, std::vector<std::string>> Shell::split_cmd(std::string& cmd) {
    std::vector<std::string> cmds;
    bool isQuote = false;
    std::string item = "";
    for (auto ch : cmd) {
        if (isQuote) {
            if (ch == '\"') {
                cmds.emplace_back(item);
                isQuote = false;
                item = "";
            }
            else {
                item += ch;
            }
        }
        else {
            if (ch == '\"') {
                if (!item.empty()) {
                    cmds.emplace_back(item);
                }
                isQuote = true;
                item = "";
            }
            else if (ch != ' ' && ch != '\t') {
                item += ch;
            }
            else if (!item.empty()) {
                cmds.emplace_back(item);
                item = "";
            }
        }
    }
    if (isQuote) {
        cmds.clear();
        return std::make_pair(false, cmds);
    }

    if (!item.empty()) {
        cmds.emplace_back(item);
    }
    return std::make_pair(true, cmds);
}

void Shell::exec() {
    //init();
    userInterface.initialize();
    std::string input, word;
    bool valid;

    while (!isExit) {
        if (user.uid == 0) {
            cmd_login();
            std::getchar();
            curPath.clear();
        }
        userInterface.updateDirNow();
        outputPrefix();
        std::getline(std::cin, input);
        //cmd.clear();
        //std::istringstream ss(input);
        //while (ss >> word) {
        //    cmd.push_back(word);
        //}

        std::tie(valid, cmd) = split_cmd(input);

        //std::cerr << valid << std::endl;
        //for (int i = 0; i < cmd.size(); i++) {
        //    std::cerr << cmd[i] << " \n"[i == cmd.size() - 1];
        //}

        if (!valid) {
            std::cout << RED << "syntax error:" << RESET << " missing terminating \" character" << std::endl;
            continue;
        }
        if (cmd.empty()) {
            continue;
        }

        std::string cmdType = cmd[0];

        if (cmdType == "sudo") {
            isSudo = true;
            cmd.erase(cmd.begin());
            if (cmd.empty()) {
                cmdType = "";
                std::cout << "sudo: missing command operand" << std::endl;
                continue;
            }
            else {
                cmdType = cmd[0];
            }
            if (!cmd_sudo()) {
                continue;
            }
        }
        else {
            isSudo = false;
        }
        userInterface.setSudoMode(isSudo);
        userInterface.setCurrentCmd(cmdType);

        if (cmdType == "logout") {
            cmd_logout();
        }
        else if (cmdType == "exit") {
            cmd_exit();
        }
        else if (cmdType == "cd") {
            cmd_cd();
        }
        else if (cmdType == "ls") {
            cmd_ls();
        }
        else if (cmdType == "touch") {
            cmd_touch();
        }
        else if (cmdType == "cat") {
            cmd_cat();
        }
        else if (cmdType == "vim") {
            cmd_vim();
        }
        else if (cmdType == "mv") {
            cmd_mv();
        }
        else if (cmdType == "cp") {
            cmd_cp();
        }
        else if (cmdType == "rm") {
            cmd_rm();
        }
        else if (cmdType == "mkdir") {
            cmd_mkdir();
        }
        else if (cmdType == "rmdir") {
            cmd_rmdir();
        }
        else if (cmdType == "format") {
            cmd_format();
        }
        else if (cmdType == "chmod") {
            cmd_chmod();
        }
        else if (cmdType == "useradd") {
            cmd_useradd();
        }
        else if (cmdType == "userdel") {
            cmd_userdel();
        }
        else if (cmdType == "userlist") {
            cmd_userlist();
        }
        else if (cmdType == "passwd") {
            cmd_passwd();
        }
        else if (cmdType == "trust") {
            cmd_trust();
        }
        else if (cmdType == "distrust") {
            cmd_distrust();
        }
        else if (cmdType == "help") {
            cmd_help();
        }
        else if (cmdType == "clear") {
            cmd_clear();
        }
        else {
            std::cout << cmdType << ": command not found" << std::endl;
        }
    }
}

void Shell::cmd_login() {
    std::string userName;
    std::string password;
    cmd_clear();
    while (1) {
        std::cout << "Login as: " << std::flush;
        std::cin >> userName;
        //userName = "root";
        std::cout << "Password: " << std::flush;
        std::cin >> password;
        //password = "123456";

        uint8_t uid = userInterface.userVerify(userName, password);
        if (uid == 0) {
            cmd_clear();
            std::cout << RED << "Access denied. " << RESET << "Please check username or password." << std::endl;
        }
        else {
            userInterface.getUser(uid, &user);
            break;
        }
    }
    cmd_clear();
}

void Shell::cmd_logout() {
    if (cmd.size() > 1) {
        std::cout << "logout: too much arguments" << std::endl;
        return;
    }
    userInterface.logout();
    user.uid = 0;
}

void Shell::cmd_exit() {
    if (cmd.size() > 1) {
        std::cout << "exit: too much arguments" << std::endl;
        return;
    }
    userInterface.logout();
    user.uid = 0;
    isExit = true;
}

void Shell::cmd_cd() {
    if (cmd.size() > 2) {
        std::cout << "cd: too much arguments" << std::endl;
        return;
    }

    if (cmd.size() == 1) {
        return;
    }
    std::vector<std::string> src = split_path(cmd[1]);
    bool ok = true;
    for (std::string& item : src) {
        if (item == "") {
            userInterface.goToRoot();
            curPath.clear();
        }
        else if (item == ".") {
            continue;
        }
        else if (item == "..") {
            if (userInterface.cd(user.uid, item, cmd[1]).first && !curPath.empty()) {
                curPath.pop_back();
            }
            else {
                break;
            }
        }
        else {
            if (userInterface.cd(user.uid, item, cmd[1]).first) {
                curPath.push_back(item);
            }
            else {
                break;
            }
        }
    }
}

void Shell::cmd_ls() {
    if (cmd.size() > 3) {
        std::cout << "ls: too much arguments" << std::endl;
        return;
    }

    if (cmd.size() == 1) {
        userInterface.ls(user.uid, false, std::string());
    }
    else if (cmd.size() == 2) {
        if (cmd[1] == "-a") {
            userInterface.ls(user.uid, true, std::string());
        }
        else {
            std::vector<std::string> src = split_path(cmd[1]);
            if (src.empty()) {
                std::cout << "ls: missing operand" << std::endl;
                return;
            }
            userInterface.ls(user.uid, false, src, cmd[1]);
        }
    }
    else {
        if (cmd[1] == "-a") {
            std::vector<std::string> src = split_path(cmd[2]);
            if (src.empty()) {
                std::cout << "ls: missing operand" << std::endl;
                return;
            }
            userInterface.ls(user.uid, true, src, cmd[2]);
        }
        else if (cmd[2] == "-a") {
            std::vector<std::string> src = split_path(cmd[1]);
            if (src.empty()) {
                std::cout << "ls: missing operand" << std::endl;
                return;
            }
            userInterface.ls(user.uid, true, src, cmd[1]);
        }
        else {
            std::cout << "ls: too much arguments" << std::endl;
        }
    }
}

void Shell::cmd_touch() {
    if (cmd.size() < 2) {
        std::cout << "touch: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "touch: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "touch: missing file operand" << std::endl;
        return;
    }
    std::string fileName = src.back();
    if (fileName.length() >= FILE_NAME_LENGTH) {
        std::cout << "touch: too long file name" << std::endl;
        return;
    }
    src.pop_back();
    if (!src.empty()) userInterface.touch(user.uid, src, fileName, cmd[1]);
    else userInterface.touch(user.uid, fileName, cmd[1]);
}

void Shell::cmd_cat() {
    if (cmd.size() < 2) {
        std::cout << "cat: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "cat: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "cat: missing file operand" << std::endl;
        return;
    }
    std::string fileName = src.back();
    src.pop_back();
    if (!src.empty()) userInterface.cat(user.uid, src, fileName, cmd[1]);
    else userInterface.cat(user.uid, fileName, cmd[1]);
}

void Shell::cmd_vim() {
    if (cmd.size() < 2) {
        std::cout << "vim: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "vim: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "vim: missing file operand" << std::endl;
        return;
    }
    std::string fileName = src.back();
    if (fileName.length() >= FILE_NAME_LENGTH) {
        std::cout << "vim: too long file name" << std::endl;
        return;
    }
    src.pop_back();
    if (!src.empty()) userInterface.vim(user.uid, src, fileName, cmd[1]);
    else userInterface.vim(user.uid, fileName, cmd[1]);
}

void Shell::cmd_mv() {
    if (cmd.size() < 3) {
        std::cout << "mv: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 3) {
        std::cout << "mv: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "mv: missing file operand" << std::endl;
        return;
    }
    std::vector<std::string> des = split_path(cmd[2]);
    if (des.empty()) {
        std::cout << "mv: missing file operand" << std::endl;
        return;
    }
    userInterface.mv(user.uid, src, des, cmd[1], cmd[2]);
}

void Shell::cmd_cp() {
    if (cmd.size() < 3) {
        std::cout << "cp: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 3) {
        std::cout << "cp: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "cp: missing file operand" << std::endl;
        return;
    }
    std::vector<std::string> des = split_path(cmd[2]);
    if (des.empty()) {
        std::cout << "cp: missing file operand" << std::endl;
        return;
    }
    userInterface.cp(user.uid, src, des, cmd[1], cmd[2]);
}

void Shell::cmd_rm() {
    if (cmd.size() < 2) {
        std::cout << "rm: missing file operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "rm: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "rm: missing file operand" << std::endl;
        return;
    }
    std::string fileName = src.back();
    src.pop_back();
    if (!src.empty()) userInterface.rm(user.uid, src, fileName, cmd[1]);
    else userInterface.rm(user.uid, fileName, cmd[1]);
}

void Shell::cmd_mkdir() {
    if (cmd.size() < 2) {
        std::cout << "mkdir: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "mkdir: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "mkdir: missing operand" << std::endl;
        return;
    }
    std::string dirName = src.back();
    if (dirName.length() >= FILE_NAME_LENGTH) {
        std::cout << "mkdir: too long directory name" << std::endl;
        return;
    }
    src.pop_back();
    if (!src.empty()) userInterface.mkdir(user.uid, src, dirName, cmd[1]);
    else userInterface.mkdir(user.uid, dirName, cmd[1]);
}

void Shell::cmd_rmdir() {
    if (cmd.size() < 2) {
        std::cout << "rmdir: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "rmdir: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "rmdir: missing operand" << std::endl;
        return;
    }
    std::string dirName = src.back();
    src.pop_back();
    if (!src.empty()) userInterface.rmdir(user.uid, src, dirName, cmd[1]);
    else userInterface.rmdir(user.uid, dirName, cmd[1]);
}

bool Shell::cmd_sudo() {
    std::cout << "[sudo] password for " << user.name << ": " << std::flush;
    std::string password;
    std::cin >> password;
    std::cin.ignore();

    uint8_t checkUid = userInterface.userVerify(user.name, password);
    if (!checkUid) {
        std::cout << "Password verification " << RED << "failed" << RESET << std::endl;
        return false;
    }
    else if (checkUid != user.uid) {
        std::cout << "sudo: System error" << std::endl;
        return false;
    }
    else {
        std::cout << "Verification successful" << std::endl;
        return true;
    }
}

void Shell::cmd_format() {
    if (cmd.size() > 1) {
        std::cout << "format: too much arguments" << std::endl;
        return;
    }
    userInterface.logout();
    userInterface.format();
    curPath.clear();
    user.uid = 0;
}

void Shell::cmd_chmod() {
    //chmod <file> -ato rwx
    if (cmd.size() < 3) {
        std::cout << "chmod: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 4) {
        std::cout << "chmod: too much arguments" << std::endl;
        return;
    }

    std::vector<std::string> src = split_path(cmd[1]);
    if (src.empty()) {
        std::cout << "chmod: missing operand" << std::endl;
        return;
    }
    if (cmd[2] != "-a" && cmd[2] != "-t" && cmd[2] != "-o") {
        std::cout << "chmod: " << RED << "invalid " << RESET << "mode: '" << cmd[2] << "'" << std::endl;
        return;
    }
    std::string access(3, '-');
    if (cmd.size() == 4) {
        std::string tmp = cmd[3];
        if (tmp.find('r') != std::string::npos) {
            access[0] = 'r';
            tmp.erase(tmp.find('r'), 1);
        }
        if (tmp.find('w') != std::string::npos) {
            access[1] = 'w';
            tmp.erase(tmp.find('w'), 1);
        }
        if (tmp.find('x') != std::string::npos) {
            access[2] = 'x';
            tmp.erase(tmp.find('x'), 1);
        }
        if (!tmp.empty()) {
            std::cout << "chmod: " << RED << "invalid " << RESET << "access: '" << cmd[3] << "'" << std::endl;
            return;
        }
    }

    std::string dirName = src.back();
    src.pop_back();
    if (!src.empty()) userInterface.chmod(user.uid, src, dirName, cmd[2], access, cmd[1]);
    else userInterface.chmod(user.uid, dirName, cmd[2], access, cmd[1]);
}

void Shell::cmd_useradd() {
    if (cmd.size() < 2) {
        std::cout << "useradd: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "useradd: too much arguments" << std::endl;
        return;
    }

    std::string name = cmd[1];
    if (name.length() >= USERNAME_PASWORD_LENGTH) {
        std::cout << "useradd: too long user name" << std::endl;
        return;
    }
    userInterface.useradd(user.uid, name);
}

void Shell::cmd_userdel() {
    if (cmd.size() < 2) {
        std::cout << "userdel: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "userdel: too much arguments" << std::endl;
        return;
    }

    std::string name = cmd[1];
    userInterface.userdel(user.uid, name);
}

void Shell::cmd_userlist() {
    if (cmd.size() > 1) {
        std::cout << "userlist: too much arguments" << std::endl;
        return;
    }
    userInterface.userlist();
}

void Shell::cmd_passwd() {
    if (cmd.size() > 1) {
        std::cout << "passwd: too much arguments" << std::endl;
        return;
    }
    userInterface.passwd(user.uid, user.name);
}

void Shell::cmd_trust() {
    if (cmd.size() < 2) {
        std::cout << "trust: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "trust: too much arguments" << std::endl;
        return;
    }

    std::string name = cmd[1];
    userInterface.trust(user.uid, user.name, name);
}

void Shell::cmd_distrust() {
    if (cmd.size() < 2) {
        std::cout << "distrust: missing operand" << std::endl;
        return;
    }
    if (cmd.size() > 2) {
        std::cout << "distrust: too much arguments" << std::endl;
        return;
    }

    std::string name = cmd[1];
    userInterface.distrust(user.uid, user.name, name);
}

void Shell::cmd_help() {
    if (cmd.size() > 2) {
        std::cout << "cd: too much arguments" << std::endl;
        return;
    }

    if (cmd.size() == 1) { //单独执行help，表示输出所有命令
        std::cout << "Usage                         Interpret" << std::endl;
        for (auto [_, inform] : helpInform) {
            std::cout << inform << std::endl;
        }
    }
    else if (helpInform.count(cmd[1])) {
        std::cout << "Usage                         Interpret" << std::endl;
        std::cout << helpInform[cmd[1]] << std::endl;
    }
    else {
        std::cout << "help: " << cmd[1] << ": command not found" << std::endl;
    }
}

void Shell::cmd_clear() {
    system("cls");
}

const std::vector<std::string>& Shell::getCmd() const {
    return cmd;
}

void Shell::setCmd(const std::vector<std::string>& cmd) {
    Shell::cmd = cmd;
}

void Shell::outputPrefix() {
    std::cout << YELLOW << "FileSystem@" << user.name << ":" << GREEN << "~";
    for (const auto& s : curPath) {
        std::cout << "/" << s;
    }
    std::cout << RESET << "$ ";
}