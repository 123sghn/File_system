#include "UserInterface.h"

void UserInterface::initialize() {
    //如果挂载失败,先格式化
    if (!fileSystem.mount()) {
        std::cout << "mount failed!" << std::endl << "begin format!" << std::endl;
        //如果格式化失败,创建新磁盘
        if (!fileSystem.format(BLOCK_BYTE)) {
            std::cout << "format failed because there is no disk.\nStart creating a disk, please input disk size(MB): "
                      << std::flush;
            uint32_t disk_size;
            std::cin >> disk_size;
            fileSystem.createDisk(disk_size * 1024 * 1024);
            std::cout << "disk create success!" << std::endl;
            fileSystem.mount();
            fileSystem.format(BLOCK_BYTE);
        }
        std::cout << "format success!" << std::endl;
    }
    //读入根节点所在磁盘块
    uint32_t root_disk = fileSystem.getRootLocation();
    //根节点
    INode rootInode{};
    //从根节点所在磁盘块读入根节点信息
    fileSystem.read(root_disk, 0, reinterpret_cast<char*>(&rootInode), sizeof(rootInode));
    //将根目录信息写入当前目录
    fileSystem.read(rootInode.bno, 0, reinterpret_cast<char*> (&directory), sizeof(directory));
    nowDiretoryDisk = rootInode.bno;
}

bool UserInterface::logout() {
    uint32_t root_disk = fileSystem.getRootLocation();
    //根节点
    INode rootInode{};
    //从根节点所在磁盘块读入根节点信息
    fileSystem.read(root_disk, 0, reinterpret_cast<char*>(&rootInode), sizeof(rootInode));
    //将根目录信息写入当前目录
    fileSystem.read(rootInode.bno, 0, reinterpret_cast<char*> (&directory), sizeof(directory));
    nowDiretoryDisk = rootInode.bno;

    //更新信息
    fileSystem.update();
    return true;
}

std::pair<bool, int> UserInterface::cd(uint8_t uid, std::string directoryName, const std::string& initCmd) {
    int dirLocation = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, directoryName.c_str())) {
            //确保是目录名
            if (judge(directory.item[i].inodeIndex)) {
                dirLocation = i;
            }
            else {
                dirLocation = -2;
            }
            break;
        }
    }

    if (dirLocation == -1) {
        if (!initCmd.empty()) {
            std::cout << currentCmd << ": " << initCmd << ": No such directory" << std::endl;
        }
        return {false, 0};
    }
    if (dirLocation == -2) {
        if (!initCmd.empty()) {
            std::cout << currentCmd << ": " << initCmd << ": Not a directory" << std::endl;
        }
        return {false, 1};
    }

    //获取指向需要进入的目录的i结点
    uint32_t diretoryInodeDisk = directory.item[dirLocation].inodeIndex;
    INode iNode{};
    fileSystem.read(diretoryInodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));

    if (!checkReadAccess(uid, iNode)) {
        if (!initCmd.empty()) {
            std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        }
        return {false, 2};
    }

    //设置新的当前目录
    nowDiretoryDisk = iNode.bno;
    fileSystem.read(iNode.bno, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    return {true, 0};
}

bool UserInterface::ls(uint8_t uid, bool all, const std::string& initCmd) {
    INode iNode{};
    fileSystem.read(directory.item[0].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (!checkReadAccess(uid, iNode)) {
        if (initCmd.empty()) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory: Permission denied" << std::endl;
        }
        else {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    if (all) {
        std::cout << "  fileName   | uid |              owner               |   access   |    creation time    |    modified time" << std::endl;
    }
    for (int i = 0; i < DIRECTORY_NUMS && directory.item[i].inodeIndex != 0; i++) {
        if (all) {
            if (judge(directory.item[i].inodeIndex)) {
                std::cout << GREEN << printFixedLength(directory.item[i].name, FILE_NAME_LENGTH) << RESET << " |  ";
            }
            else {
                std::cout << printFixedLength(directory.item[i].name, FILE_NAME_LENGTH) << " |  ";
            }
            INode iNode{};
            fileSystem.read(directory.item[i].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
            std::cout << (int)iNode.uid << "  | ";
            if (iNode.uid) {
                User user{};
                fileSystem.getUser(iNode.uid, &user);
                std::cout << printFixedLength(user.name, USERNAME_PASWORD_LENGTH) << " | ";
            }
            else {
                std::cout << printFixedLength("SYSTEM DEFAULT DIRECTORIES", USERNAME_PASWORD_LENGTH) << " | ";
            }
            std::cout << "fd"[iNode.flag >> 6];
            std::cout << "rwx"; //所有者权限
            std::cout << "-r"[iNode.flag >> 5 & 1] << "-w"[iNode.flag >> 4 & 1] << "-x"[iNode.flag >> 3 & 1]; //信赖者权限
            std::cout << "-r"[iNode.flag >> 2 & 1] << "-w"[iNode.flag >> 1 & 1] << "-x"[iNode.flag & 1]; //其余用户权限
            std::cout << " | ";
            std::cout << iNode.creationTime << " | ";
            std::cout << iNode.modifiedTime << std::endl;
        }
        else {
            if (judge(directory.item[i].inodeIndex)) {
                std::cout << GREEN << directory.item[i].name << RESET << "\t";
            }
            else {
                std::cout << directory.item[i].name << "\t";
            }
        }
    }
    if (!all) {
        std::cout << std::endl;
    }
    return true;
}

bool UserInterface::ls(uint8_t uid, bool all, std::vector<std::string> src, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory '" << initCmd << "': No such directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory '" << initCmd << "': Not a directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "open directory '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    ls(uid, all, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::touch(uint8_t uid, std::string fileName, const std::string& initCmd) {
    INode iNode{};
    fileSystem.read(directory.item[0].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (!checkWriteAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }

    int directoryIndex = -1;
    //遍历所有目录项,找到空闲目录项
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            directoryIndex = i;
            break;
        }
    }
    //目录项满了
    if (directoryIndex == -1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': No space left on device" << std::endl;
        return false;
    }

    //重复文件检测
    if (duplicateDetection(fileName)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': File exists" << std::endl;
        return false;
    }

    //给新文件的i结点分配空闲磁盘块
    uint32_t fileInodeDisk = fileSystem.blockAllocate();
    //给文件索引表分配空闲磁盘块
    uint32_t fileIndexDisk = fileSystem.blockAllocate();

    FileIndex fileIndex{};
    //给文件分配空闲磁盘块
    //uint32_t fileDisk = fileSystem.blockAllocate();
    //std::cout << fileDisk << std::endl;
    fileIndex.index[0] = 0;
    fileIndex.next = 0;
    //把文件索引表写入磁盘
    fileSystem.write(fileIndexDisk, 0, reinterpret_cast<char*>(&fileIndex), sizeof(fileIndex));

    //新文件i节点
    INode fileInode{};
    fileInode.bno = fileIndexDisk;
    fileInode.flag = 0x34; //00 110 100b
    //fileInode.flag = 0x20; //00 100 000b
    fileInode.uid = uid;
    strcpy(fileInode.creationTime, INode::getCurTime().c_str());
    strcpy(fileInode.modifiedTime, fileInode.creationTime);
    fileSystem.write(fileInodeDisk, 0, reinterpret_cast<char*>(&fileInode), sizeof(fileInode));

    //更新目录项信息
    strcpy(directory.item[directoryIndex].name, fileName.c_str());
    directory.item[directoryIndex].inodeIndex = fileInodeDisk;
    if (directoryIndex + 1 < DIRECTORY_NUMS) {
        directory.item[directoryIndex + 1].inodeIndex = 0;
    }
    //将更新后的当前目录信息写入磁盘
    fileSystem.write(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));

    //char str[BLOCK_SIZE] = "testjljslkadjflkasjdf";
    //fileSystem.write(fileDisk, 0, reinterpret_cast<char*>(str), BLOCK_BYTE);

    fileSystem.update();
    return true;
}

bool UserInterface::touch(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': No such file or directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': No such file or directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "touch '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    touch(uid, fileName, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::cat(uint8_t uid, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* returnContent) {
    int fileLocation = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, fileName.c_str())) {
            if (!judge(directory.item[i].inodeIndex)) {
                fileLocation = i;
            }
            else {
                fileLocation = -2;
            }
            break;
        }
    }
    if (fileLocation == -1) {
        std::cout << currentCmd << ": " << initCmd << ": No such file" << std::endl;
        return false;
    }
    if (fileLocation == -2) {
        std::cout << currentCmd << ": " << initCmd << ": Not a file" << std::endl;
        return false;
    }

    INode iNode{};
    fileSystem.read(directory.item[fileLocation].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        return false;
    }

    std::string content = readFile(iNode.bno);
    if (returnContent) {
        std::get<0>(*returnContent) = true;
        std::get<1>(*returnContent) = content;
        std::get<2>(*returnContent) = iNode.creationTime;
    }
    else {
        std::cout << content << std::endl;
    }
    return true;
}

bool UserInterface::cat(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* returnContent) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << initCmd << ": No such file or directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << initCmd << ": No such file or directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    cat(uid, fileName, initCmd, returnContent);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::vim(uint8_t uid, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* inputContent) {
    int fileLocation = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, fileName.c_str())) {
            if (!judge(directory.item[i].inodeIndex)) {
                fileLocation = i;
            }
            else {
                fileLocation = -2;
            }
            break;
        }
    }
    if (fileLocation == -1) {
        if (!touch(uid, fileName, initCmd)) {
            return false;
        }
        else {
            for (int i = 0; i < DIRECTORY_NUMS; i++) {
                if (directory.item[i].inodeIndex == 0) {
                    break;
                }
                if (!strcmp(directory.item[i].name, fileName.c_str())) {
                    fileLocation = i;
                    break;
                }
            }
        }
    }
    else if (inputContent) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "move to '" << initCmd << "': File exists" << std::endl;
        return false;
    }
    if (fileLocation == -2) {
        std::cout << currentCmd << ": " << initCmd << ": Not a file" << std::endl;
        return false;
    }

    INode iNode{};
    fileSystem.read(directory.item[fileLocation].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));

    if (!inputContent) {
        if (!checkReadAccess(uid, iNode)) {
            std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
            return false;
        }
        std::string content = readFile(iNode.bno);
        system("cls");
        auto [save, newContent] = Vim(!checkWriteAccess(uid, iNode), content).exec();
        newContent.pop_back();
        system("cls");
        if (save) {
            uint32_t newFileIndexDisk = writeFile(newContent);
            freeFile(iNode.bno);
            iNode.bno = newFileIndexDisk;
            strcpy(iNode.modifiedTime, INode::getCurTime().c_str());
            fileSystem.write(directory.item[fileLocation].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
        }
    }
    else {
        if (!checkWriteAccess(uid, iNode)) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << (currentCmd == "mv" ? "move" : "copy") << " to '" << initCmd << "': Permission denied" << std::endl;
            return false;
        }
        uint32_t newFileIndexDisk = writeFile(std::get<1>(*inputContent));
        freeFile(iNode.bno);
        iNode.bno = newFileIndexDisk;
        std::get<0>(*inputContent) = true;
        strcpy(iNode.creationTime, std::get<2>(*inputContent).c_str());
        strcpy(iNode.modifiedTime, INode::getCurTime().c_str());
        fileSystem.write(directory.item[fileLocation].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    }
    fileSystem.update();
    return true;
}

bool UserInterface::vim(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd, std::tuple<bool, std::string, std::string>* inputContent) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << initCmd << ": No such file or directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << initCmd << ": No such file or directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << initCmd << ": Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    vim(uid, fileName, initCmd, inputContent);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::mv(uint8_t uid, std::vector<std::string> src, std::vector<std::string> des, const std::string& initSrc, const std::string& initDes) {
    if (!cp(uid, src, des, initSrc, initDes)) {
        return false;
    }

    std::string srcName = src.back();
    src.pop_back();
    if (!src.empty()) {
        if (!rm(uid, src, srcName, initSrc)) {
            return false;
        }
    }
    else {
        if (!rm(uid, srcName, initSrc)) {
            return false;
        }
    }
    fileSystem.update();
    return true;
}

bool UserInterface::cp(uint8_t uid, std::vector<std::string> src, std::vector<std::string> des, const std::string& initSrc, const std::string& initDes) {
    std::tuple<bool, std::string, std::string> content;
    std::get<0>(content) = false;
    std::string srcName = src.back();
    src.pop_back();
    if (!src.empty()) {
        cat(uid, src, srcName, initSrc, &content);
    }
    else {
        cat(uid, srcName, initSrc, &content);
    }
    if (!std::get<0>(content)) {
        return false;
    }

    std::get<0>(content) = false;
    std::string desName = des.back();
    des.pop_back();
    if (!des.empty()) {
        vim(uid, des, desName, initDes, &content);
    }
    else {
        vim(uid, desName, initDes, &content);
    }
    if (!std::get<0>(content)) {
        return false;
    }
    fileSystem.update();
    return true;
}

bool UserInterface::rm(uint8_t uid, std::string fileName, const std::string& initCmd) {
    int fileLocation = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, fileName.c_str())) {
            if (!judge(directory.item[i].inodeIndex)) {
                fileLocation = i;
            }
            else {
                fileLocation = -2;
            }
            break;
        }
    }
    if (fileLocation == -1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': No such file" << std::endl;
        return false;
    }
    if (fileLocation == -2) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': Not a file" << std::endl;
        return false;
    }

    //先找到文件索引表,把所有文件和文件索引表的磁盘块回收
    INode fileIndexInode{};
    fileSystem.read(directory.item[fileLocation].inodeIndex, 0, reinterpret_cast<char*>(&fileIndexInode), sizeof(fileIndexInode));
    if (!checkWriteAccess(uid, fileIndexInode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }

    freeFile(fileIndexInode.bno);
    //回收i结点
    fileSystem.blockFree(directory.item[fileLocation].inodeIndex);
    //更新目录项
    wholeDirItemsMove(fileLocation);
    //将新的目录项写入磁盘
    fileSystem.write(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    fileSystem.update();
    return true;
}

bool UserInterface::rm(uint8_t uid, std::vector<std::string> src, std::string fileName, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': No such file or directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': No such file or directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    rm(uid, fileName, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::mkdir(uint8_t uid, std::string directoryName, const std::string& initCmd) {
    INode iNode{};
    fileSystem.read(directory.item[0].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (!checkWriteAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }

    int directoryIndex = -1;
    //遍历所有目录项,找到空闲目录项
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            directoryIndex = i;
            break;
        }
    }
    //目录项满了
    if (directoryIndex == -1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': No space left on device" << std::endl;
        return false;
    }

    //重复文件检测
    if (duplicateDetection(directoryName)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': File exists" << std::endl;
        return false;
    }

    //给新目录的i结点分配空闲磁盘块
    uint32_t directoryInodeDisk = fileSystem.blockAllocate();
    //给新目录的目录项信息分配空闲磁盘块
    uint32_t directoryDisk = fileSystem.blockAllocate();
    //std::cout << directoryInodeDisk << ' ' << directoryDisk << std::endl;
    Directory newDirectory{};
    //目录的第一项为本目录的信息
    newDirectory.item[0].inodeIndex = directoryInodeDisk;
    //设置本目录./
    strcpy(newDirectory.item[0].name, ".");
    //目录的第二项为上级目录的信息
    newDirectory.item[1].inodeIndex = directory.item[0].inodeIndex;
    //设置上级目录../
    strcpy(newDirectory.item[1].name, "..");
    //设置结束标记
    newDirectory.item[2].inodeIndex = 0;
    //将目录项信息写入磁盘
    fileSystem.write(directoryDisk, 0, reinterpret_cast<char*>(&newDirectory), sizeof(newDirectory));

    //新目录i节点
    INode directoryInode{};
    directoryInode.bno = directoryDisk;
    directoryInode.flag = 0x74; //01 110 100b
    //directoryInode.flag = 0x60; //01 100 000b
    directoryInode.uid = uid;
    strcpy(directoryInode.creationTime, INode::getCurTime().c_str());
    strcpy(directoryInode.modifiedTime, directoryInode.creationTime);
    fileSystem.write(directoryInodeDisk, 0, reinterpret_cast<char*>(&directoryInode), sizeof(directoryInode));

    //更新当前目录目录项
    strcpy(directory.item[directoryIndex].name, directoryName.c_str());
    directory.item[directoryIndex].inodeIndex = directoryInodeDisk;
    if (directoryIndex + 1 < DIRECTORY_NUMS) {
        directory.item[directoryIndex + 1].inodeIndex = 0;
    }
    //将更新后的当前目录信息写入磁盘
    fileSystem.write(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));

    //更新已分配磁盘块
    fileSystem.update();
    return true;
}

bool UserInterface::mkdir(uint8_t uid, std::vector<std::string> src, std::string dirName, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': No such directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': Not a directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "create directory '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    mkdir(uid, dirName, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::rmdir(uint8_t uid, std::string dirName, const std::string& initCmd) {
    //先查找对应目录
    int dirLocation = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, dirName.c_str())) {
            if (judge(directory.item[i].inodeIndex)) {
                dirLocation = i;
            }
            else {
                dirLocation = -2;
            }
            break;
        }
    }
    if (dirLocation == -1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': No such directory" << std::endl;
        return false;
    }
    if (dirLocation == -2) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': Not a directory" << std::endl;
        return false;
    }

    //保存当前目录所在磁盘块
    uint32_t nowDisk = nowDiretoryDisk;
    //进入指定目录
    INode dirInode1{};
    fileSystem.read(directory.item[dirLocation].inodeIndex, 0, reinterpret_cast<char*>(&dirInode1), sizeof(dirInode1));
    if (!checkWriteAccess(uid, dirInode1)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }

    fileSystem.read(dirInode1.bno, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    nowDiretoryDisk = dirInode1.bno;

    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (!directory.item[i].inodeIndex) break;
        if (!judge(directory.item[i].inodeIndex)) {
            //文件
            if (!rm(uid, directory.item[i].name, initCmd)) {
                fileSystem.read(nowDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
                nowDiretoryDisk = nowDisk;
                return false;
            }
            i--;
        }
        else if (strcmp(directory.item[i].name, ".") != 0 && strcmp(directory.item[i].name, "..") != 0) {
            //目录
            if (!rmdir(uid, directory.item[i].name, initCmd)) {
                fileSystem.read(nowDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
                nowDiretoryDisk = nowDisk;
                return false;
            }
            i--;
        }
    }

    //重置当前目录
    fileSystem.read(nowDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    nowDiretoryDisk = nowDisk;
    //回收指定目录的i结点所在磁盘块
    //std::cout << dirInode1.bno << ' ' << directory.item[dirLocation].inodeIndex << std::endl;
    fileSystem.blockFree(dirInode1.bno);
    fileSystem.blockFree(directory.item[dirLocation].inodeIndex);
    wholeDirItemsMove(dirLocation);
    //将新的目录项写入磁盘
    fileSystem.write(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    fileSystem.update();
    return true;
}

bool UserInterface::rmdir(uint8_t uid, std::vector<std::string> src, std::string dirName, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': No such directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': Not a directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "remove directory '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    rmdir(uid, dirName, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::format() {
    fileSystem.format(BLOCK_BYTE);
    fileSystem.update();
    INode iNode{};
    fileSystem.read(fileSystem.getRootLocation(), 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    nowDiretoryDisk = iNode.bno;
    fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    return true;
}

bool UserInterface::chmod(uint8_t uid, std::string name, std::string who, std::string access, const std::string& initCmd) {
    int location = -1;
    for (int i = 0; i < DIRECTORY_NUMS; i++) {
        if (directory.item[i].inodeIndex == 0) {
            break;
        }
        if (!strcmp(directory.item[i].name, name.c_str())) {
            location = i;
            break;
        }
    }
    if (location == -1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': No such file or directory" << std::endl;
        return false;
    }

    INode iNode{};
    fileSystem.read(directory.item[location].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (!checkOwnerAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }

    uint8_t mask;
    if (who == "-a") {
        iNode.flag &= 0xc0; //11 000 000
        mask = 0x3f; //00 111 111
    }
    else if (who == "-t") {
        iNode.flag &= 0xc7; //11 000 111
        mask = 0x38; //00 111 000
    }
    else {
        iNode.flag &= 0xf8; //11 111 000
        mask = 0x07; //00 000 111
    }

    uint8_t val = 0;
    if (access[0] == 'r') {
        val |= 0x24; //00 100 100
    }
    if (access[1] == 'w') {
        val |= 0x12; //00 010 010
    }
    if (access[2] == 'x') {
        val |= 0x09; //00 001 001
    }
    val &= mask;
    iNode.flag |= val;
    strcpy(iNode.modifiedTime, INode::getCurTime().c_str());
    fileSystem.write(directory.item[location].inodeIndex, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    return true;
}

bool UserInterface::chmod(uint8_t uid, std::vector<std::string> src, std::string name, std::string who, std::string access, const std::string& initCmd) {
    auto findRes = findDisk(uid, src);
    if (findRes.first == -1) {
        if (findRes.second == 0 || findRes.second == 1) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': No such file or directory" << std::endl;
        }
        else if (findRes.second == 2) {
            std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': Permission denied" << std::endl;
        }
        return false;
    }

    uint32_t tmpDirDisk = findRes.first;
    Directory tmpDir{};
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    uint32_t inodeDisk = tmpDir.item[findRes.second].inodeIndex;
    INode iNode{};
    fileSystem.read(inodeDisk, 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    if (iNode.flag >> 6 != 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': No such file or directory" << std::endl;
        return false;
    }
    if (!checkReadAccess(uid, iNode)) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "access '" << initCmd << "': Permission denied" << std::endl;
        return false;
    }
    tmpDirDisk = iNode.bno;
    fileSystem.read(tmpDirDisk, 0, reinterpret_cast<char*>(&tmpDir), sizeof(tmpDir));
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    chmod(uid, name, who, access, initCmd);
    std::swap(directory, tmpDir);
    std::swap(nowDiretoryDisk, tmpDirDisk);
    return true;
}

bool UserInterface::useradd(uint8_t uid, std::string name) {
    if (!sudoMode || uid != 1) {
        std::cout << currentCmd << ": Permission denied" << std::endl;
        return false;
    }
    if (fileSystem.duplicateDetection(name)) {
        std::cout << currentCmd << ": user '" << name << "' already exists" << std::endl;
        return false;
    }
    uint8_t newUid = fileSystem.emptyDetection();
    if (newUid == MAX_USER_NUMS) {
        std::cout << currentCmd << ": user list is full" << std::endl;
        return false;
    }

    std::string passwd, confirm;
    std::cout << "password: " << std::flush;
    std::cin >> passwd;
    std::cout << "retype password: " << std::flush;
    std::cin >> confirm;
    std::cin.ignore();
    if (passwd != confirm) {
        std::cout << RED << "Sorry, passwords do not match." << RESET << std::endl;
        std::cout << currentCmd << ": " << RED << "failed " << RESET << "preliminary check by password service" << std::endl;
        return false;
    }
    if (passwd.length() >= USERNAME_PASWORD_LENGTH) {
        std::cout << currentCmd << ": too long password" << std::endl;
        return false;
    }

    fileSystem.useradd(newUid, name, passwd);
    fileSystem.update();
    return true;
}

bool UserInterface::userdel(uint8_t uid, std::string name) {
    if (!sudoMode || uid != 1) {
        std::cout << currentCmd << ": Permission denied" << std::endl;
        return false;
    }
    uint8_t delUid = fileSystem.duplicateDetection(name);
    if (!delUid) {
        std::cout << currentCmd << ": user '" << name << "' does not exist" << std::endl;
        return false;
    }
    if (delUid == 1) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "delete user 'root'" << std::endl;
        return false;
    }

    fileSystem.userdel(delUid);
    fileSystem.update();
    return true;
}

bool UserInterface::userlist() {
    User user[MAX_USER_NUMS];
    for (int i = 0; i < MAX_USER_NUMS; i++) {
        fileSystem.getUser(i + 1, user + i);
    }

    std::cout << " uid |              owner               | trust user uids" << std::endl;
    for (int i = 0; i < MAX_USER_NUMS; i++) {
        if (!user[i].uid) {
            continue;
        }
        std::cout << "  " << (int)user[i].uid << "  | ";
        std::cout << printFixedLength(user[i].name, USERNAME_PASWORD_LENGTH) << " | ";
        bool first = false;
        for (int j = 0; j < MAX_USER_NUMS; j++) {
            if (!user[j].uid) {
                continue;
            }
            if (!fileSystem.verifyTrustUser(user[i].name, user[j].name)) {
                continue;
            }
            if (first) {
                std::cout << ", ";
            }
            else {
                first = true;
            }
            if (i == j) {
                std::cout << YELLOW << user[j].name << RESET;
            }
            else {
                std::cout << user[j].name;
            }
        }
        std::cout << std::endl;
    }
    return true;
}

bool UserInterface::passwd(uint8_t uid, std::string name) {
    std::cout << "Changing password for " << name << std::endl;
    std::cout << "current password: " << std::flush;
    std::string old;
    std::cin >> old;
    std::cin.ignore();

    int checkUid = fileSystem.userVerify(name, old);
    if (!checkUid) {
        std::cout << currentCmd << ": Authentication token manipulation error" << std::endl;
        std::cout << currentCmd << ": password unchanged" << std::endl;
        return false;
    }
    else if (checkUid != uid) {
        std::cout << currentCmd << ": System error" << std::endl;
        return false;
    }

    std::string passwd, confirm;
    std::cout << "password: " << std::flush;
    std::cin >> passwd;
    std::cout << "retype password: " << std::flush;
    std::cin >> confirm;
    std::cin.ignore();
    if (passwd != confirm) {
        std::cout << RED << "Sorry, passwords do not match." << RESET << std::endl;
        std::cout << currentCmd << ": " << RED << "failed " << RESET << "preliminary check by password service" << std::endl;
        return false;
    }
    if (passwd.length() >= USERNAME_PASWORD_LENGTH) {
        std::cout << currentCmd << ": too long password" << std::endl;
        return false;
    }

    fileSystem.passwd(uid, passwd);
    fileSystem.update();
    return true;
}

bool UserInterface::trust(uint8_t uid, std::string currentUser, std::string targetUser) {
    if (!sudoMode) {
        std::cout << currentCmd << ": Permission denied" << std::endl;
        return false;
    }
    if (fileSystem.duplicateDetection(targetUser) == uid) {
        std::cout << currentCmd << ": already trust yourself" << std::endl;
        return false;
    }
    if (!fileSystem.grantTrustUser(currentUser, targetUser)) {
        std::cout << currentCmd << ": user '" << targetUser << "' does not exist" << std::endl;
        return false;
    }
    fileSystem.update();
    return true;
}

bool UserInterface::distrust(uint8_t uid, std::string currentUser, std::string targetUser) {
    if (!sudoMode) {
        std::cout << currentCmd << ": Permission denied" << std::endl;
        return false;
    }
    if (fileSystem.duplicateDetection(targetUser) == uid) {
        std::cout << currentCmd << ": " << RED << "cannot " << RESET << "distrust yourself" << std::endl;
        return false;
    }
    if (!fileSystem.revokeTrustUser(currentUser, targetUser)) {
        std::cout << currentCmd << ": user '" << targetUser << "' does not exist" << std::endl;
        return false;
    }
    fileSystem.update();
    return true;
}

void UserInterface::updateDirNow() {
    fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
}

void UserInterface::goToRoot() {
    INode iNode{};
    fileSystem.read(fileSystem.getRootLocation(), 0, reinterpret_cast<char*>(&iNode), sizeof(iNode));
    nowDiretoryDisk = iNode.bno;
    fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
}

void UserInterface::getUser(uint8_t uid, User* user) {
    fileSystem.getUser(uid, user);
}

uint8_t UserInterface::userVerify(std::string username, std::string password) {
    return fileSystem.userVerify(username, password);
}

void UserInterface::setSudoMode(const bool& sudo) {
    sudoMode = sudo;
}

void UserInterface::setCurrentCmd(const std::string& cmd) {
    currentCmd = cmd;
}

std::pair<uint32_t, int> UserInterface::findDisk(uint8_t uid, std::vector<std::string> src) {
    //获取名
    std::string srcName = src.back();
    src.pop_back();
    uint32_t tmpDirectoryDisk = nowDiretoryDisk;
    int error = 0; //能否找到目录
    std::string dirName; //输出错误信息用
    //在此次直接调用cd函数来寻找
    for (std::string& item : src) {
        if (item == "") {
            //空，直接寻找根目录
            goToRoot();
        }
        else if (item == ".") {
            //当前
            continue;
        }
        else if (item == "..") {
            //上级
            error = cd(uid, item).second;
        }
        else {
            error = cd(uid, item).second;
        }
        if (error) {
            dirName = item; //记录哪一步出错了
            break;
        }
    }
    if (error) {
        //std::cout << RED << "failed: " << RESET << "'" << dirName << "' No such directory" << std::endl;
        //还原现场
        nowDiretoryDisk = tmpDirectoryDisk;
        fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
        return std::make_pair(-1, error);
    }
    int location = -1;
    if (srcName != "") {
        //找到对应i结点
        for (int i = 0; i < DIRECTORY_NUMS; i++) {
            if (!directory.item[i].inodeIndex) break;
            if (!strcmp(directory.item[i].name, srcName.c_str())) {
                location = i;
                break;
            }
        }
        if (location == -1) {
            //std::cout << RED << "failed " << RESET << "'" << srcName << "' No such directory or file" << std::endl;
            //还原现场
            nowDiretoryDisk = tmpDirectoryDisk;
            fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
            return std::make_pair(-1, 0);
        }
    }
    else {
        goToRoot();
        location = 0;
    }
    //记录下需要返回的数据
    std::pair<uint32_t, int> ret = std::make_pair(nowDiretoryDisk, location);
    //还原现场
    nowDiretoryDisk = tmpDirectoryDisk;
    fileSystem.read(nowDiretoryDisk, 0, reinterpret_cast<char*>(&directory), sizeof(directory));
    return ret;
}

uint32_t UserInterface::readFileBlock(uint32_t disk, std::string& content) {
    FileIndex fileIndex{};
    fileSystem.read(disk, 0, reinterpret_cast<char*>(&fileIndex), sizeof(fileIndex));
    char buf[BLOCK_BYTE] = {};
    for (int i = 0; i < FILE_INDEX_SIZE && fileIndex.index[i]; i++) {
        fileSystem.read(fileIndex.index[i], 0, reinterpret_cast<char*>(buf), BLOCK_BYTE);
        for (int j = 0; j < BLOCK_BYTE; j++) {
            if (buf[j]) {
                content += buf[j];
            }
        }
    }
    return fileIndex.next;
}

std::string UserInterface::readFile(uint32_t startDisk) {
    std::string content;
    uint32_t next = readFileBlock(startDisk, content);
    while (next) {
        next = readFileBlock(next, content);
    }
    return content;
}

uint32_t UserInterface::writeFileBlock(uint32_t nxtDisk, const std::string& content) {
    FileIndex fileIndex{};
    fileIndex.next = nxtDisk;
    char buf[BLOCK_BYTE] = {};
    int i = 0;
    for (; i < FILE_INDEX_SIZE && i * BLOCK_BYTE < content.length(); i++) {
        uint32_t startPosi = i * BLOCK_BYTE;
        uint32_t endPosi = std::min<uint32_t>(content.length(), (i + 1) * BLOCK_BYTE);
        for (int j = 0; j < BLOCK_BYTE; j++) {
            if (startPosi + j < endPosi) buf[j] = content[startPosi + j];
            else buf[j] = 0;
        }
        fileIndex.index[i] = fileSystem.blockAllocate();
        //std::cout << "writeFileBlock: " << startPosi << ' ' << endPosi << ' ' << fileIndex.index[i] << std::endl;
        fileSystem.write(fileIndex.index[i], 0, reinterpret_cast<char*>(buf), BLOCK_BYTE);
    }
    fileIndex.index[i] = 0;
    uint32_t fileIndexDisk = fileSystem.blockAllocate();
    fileSystem.write(fileIndexDisk, 0, reinterpret_cast<char*>(&fileIndex), sizeof(fileIndex));
    return fileIndexDisk;
}

uint32_t UserInterface::writeFile(std::string content) {
    uint32_t totBlock = (content.length() + BLOCK_BYTE * FILE_INDEX_SIZE - 1) / (BLOCK_BYTE * FILE_INDEX_SIZE);
    uint32_t next = 0;
    for (int i = totBlock - 1; i >= 0; i--) {
        uint32_t startPosi = i * BLOCK_BYTE * FILE_INDEX_SIZE;
        uint32_t endPosi = std::min<uint32_t>(content.length(), (i + 1) * BLOCK_BYTE * FILE_INDEX_SIZE);
        //std::cout << "writeFile: " << startPosi << ' ' << endPosi << std::endl;
        next = writeFileBlock(next, content.substr(startPosi, endPosi - startPosi));
    }
    return next;
}

uint32_t UserInterface::freeFileBlock(uint32_t disk) {
    FileIndex fileIndex{};
    fileSystem.read(disk, 0, reinterpret_cast<char*>(&fileIndex), sizeof(fileIndex));
    for (int i = 0; i < FILE_INDEX_SIZE && fileIndex.index[i]; i++) {
        //回收文件磁盘块
        fileSystem.blockFree(fileIndex.index[i]);
        fileIndex.index[i] = 0;
    }
    uint32_t next = fileIndex.next;
    fileSystem.blockFree(disk);
    return next;
}

void UserInterface::freeFile(uint32_t startDisk) {
    //循环删除,直到没有下一个索引
    uint32_t next = freeFileBlock(startDisk);
    while (next) next = freeFileBlock(next);
}

void UserInterface::wholeDirItemsMove(int itemLocation) {
    //目录项整体前移
    directory.item[itemLocation].inodeIndex = 0;
    for (int i = itemLocation; i + 1 < DIRECTORY_NUMS; i++) {
        if (!directory.item[i + 1].inodeIndex) break;
        std::swap(directory.item[i], directory.item[i + 1]);
    }
}

bool UserInterface::duplicateDetection(std::string name) {
    for (int i = 0; i < DIRECTORY_NUMS; ++i) {
        if (!directory.item[i].inodeIndex) {
            break;
        }
        if (!strcmp(directory.item[i].name, name.c_str())) {
            return true;
        }
    }
    return false;
}

int UserInterface::judge(uint32_t disk) {
    INode iNode{};
    fileSystem.read(disk, 0, reinterpret_cast<char*> (&iNode), sizeof(iNode));
    //是目录，01,xxx,xxx
    //是文件，00,xxx,xxx
    return iNode.flag >> 6;
}

bool UserInterface::checkReadAccess(uint8_t uid, INode tar) {
    if (uid == 0) {
        std::cout << RED << "checkReadAccess: uid = 0" << RESET << std::endl;
    }
    if (!tar.uid || uid == tar.uid || sudoMode) {
        return true;
    }
    if (fileSystem.verifyTrustUser(tar.uid, uid)) {
        //xx 1/0xx xxx
        return (tar.flag >> 5) & 1;
    }
    else {
        //xx xxx 1/0xx
        return (tar.flag >> 2) & 1;
    }
}

bool UserInterface::checkWriteAccess(uint8_t uid, INode tar) {
    if (uid == 0) {
        std::cout << RED << "checkReadAccess: uid = 0" << RESET << std::endl;
    }
    if (!tar.uid || uid == tar.uid || sudoMode) {
        return true;
    }
    if (fileSystem.verifyTrustUser(tar.uid, uid)) {
        //xx x1/0x xxx
        return (tar.flag >> 4) & 1;
    }
    else {
        //xx xxx x1/0x
        return (tar.flag >> 1) & 1;
    }
}

bool UserInterface::checkOwnerAccess(uint8_t uid, INode tar) {
    if (!tar.uid) {
        return false;
    }
    else if (uid == tar.uid || sudoMode) {
        return true;
    }
    else {
        return false;
    }
}

std::string UserInterface::printFixedLength(const char* str, int maxLen) {
    int len = strlen(str);
    int l = maxLen - len >> 1;
    int r = maxLen - len - l;
    return std::string(l, ' ') + str + std::string(r, ' ');
}
