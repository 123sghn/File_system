#include "FileSystem.h"

FileSystem::FileSystem() {
    stack = new FreeBlockStack();
    isOpen = false;
}

FileSystem::~FileSystem() {
    //检查超级块的修改标记
    if (systemInfo.flag) {
        //将文件流指针移动到开始位置，并跳过容量、未格式化标志和块大小的长度
        disk.seekStart(sizeof(capacity) + sizeof(isUnformatted) + sizeof(blockSize));
        //将超级块的修改标志位置为 0，表示系统信息已经被更新
        systemInfo.flag = 0;
        //将更新后的systemInfo写入磁盘
        disk.write(reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo));
    }
    //删除动态分配的内存
    delete stack;
}

bool FileSystem::createDisk(uint32_t sz) {
    bool ok = disk.init(sz);
    return ok;
}

bool FileSystem::format(uint16_t bsize) {
    if (!isOpen) {
        return false;
    }

    //写入格式化标记、块大小
    disk.seekStart(sizeof(capacity)); //移动读写头
    isUnformatted = 0; //将格式化标记设置为已经格式化
    disk.write(reinterpret_cast<char*>(&isUnformatted), sizeof(isUnformatted)); //将格式化标记写入磁盘
    blockSize = bsize; //设置块大小
    disk.write(reinterpret_cast<char*>(&blockSize), sizeof(blockSize)); //将块大小写入磁盘
    systemInfo.flag = 0; //将修改标记设置为已被修改

    systemInfo.freeBlockStackTop = 1; //空闲块栈顶初始位于磁盘块1
    uint32_t totalBlock = capacity / blockSize; //磁盘被划分的块数
    uint32_t blockStackSize = (totalBlock * sizeof(uint32_t) + blockSize - 1) / blockSize; //空闲块栈所占用的磁盘块个数

    systemInfo.rootLocation = blockStackSize + 1; //根目录位于空闲块栈底的下一个块
    systemInfo.avaliableCapacity =
            blockSize * (totalBlock - blockStackSize - 3); //初始可用块个数=总容量-栈大小-引导超级块-根目录i节点-根目录项
    systemInfo.freeBlockNumber = totalBlock - blockStackSize - 3; //空闲块个数=总块数-空闲块栈大小-引导块-根目录项

    //初始化用户，用户名默认root
    systemInfo.users[0].uid = 1;
    strcpy(systemInfo.users[0].name, "root");
    strcpy(systemInfo.users[0].password, "123456");

    for (uint8_t i = 1; i < MAX_USER_NUMS; ++i) {
        systemInfo.users[i].uid = 0;
    }

    //初始化信赖用户矩阵
    for (uint8_t i = 0; i < MAX_USER_NUMS; ++i) {
        for (uint8_t j = 0; j < MAX_USER_NUMS; ++j) {
            systemInfo.trustMatrix[i][j] = 0;
        }
    }
    systemInfo.trustMatrix[0][0] = 1;

    //初始化磁盘中的空闲块栈
    uint32_t ptr = (blockStackSize + 1) * blockSize; //计算初始位置指针
    for (uint32_t b = totalBlock - 1; b >= blockStackSize + 3; --b) {
        ptr -= sizeof(uint32_t); //更新指针位置
        disk.seekStart(ptr); //移动读写头位置
        disk.write(reinterpret_cast<char*>(&b), sizeof(b)); //将当前块号写入磁盘
    }

    //确定空闲块栈顶的偏移量
    disk.seekStart(blockSize);
    bool findStackTop = false;
    auto& i = systemInfo.freeBlockStackTop; //空闲块栈的栈顶
    auto& j = systemInfo.freeBlockStackOffset; //空闲块栈栈顶指针所在的块内偏移
    for (i = 1; i <= blockStackSize; ++i) {
        for (j = 0; j < stack->getMaxSize(); ++j) {
            uint32_t t;
            disk.read(reinterpret_cast<char*>(&t), sizeof(t));
            if (t) {//遍历直到找到不为0的位置，此时表示找到空闲块的栈顶
                findStackTop = true;
                break;
            }
        }
        if (findStackTop) {
            break;
        }
    }

    //创建根目录，配置根目录i节点和目录列表的信息
    INode rootINode{};
    Directory dir{};
    rootINode.uid = 0; //0表示系统
    rootINode.bno = systemInfo.rootLocation + 1; //i节点磁盘块为根结点所在磁盘块的下一位
    rootINode.flag = 0x7f; //01111111，目录，所有用户都有rwx权限
    strcpy(rootINode.creationTime, INode::getCurTime().c_str());
    strcpy(rootINode.modifiedTime, rootINode.creationTime);
    //前2位表示类型，中3位表示信赖者的访问权限，后三位表示其余用户的访问权限
    dir.item[0].inodeIndex = systemInfo.rootLocation;
    strcpy(dir.item[0].name, "."); //当前目录指向自己，根目录没有上级目录，而且不需要存储其他信息，所以i结点号指向自己即可
    dir.item[1].inodeIndex = systemInfo.rootLocation;
    strcpy(dir.item[1].name, ".."); //根目录没有上级目录，所以指向自己
    dir.item[2].inodeIndex = 0;

    disk.seekStart(systemInfo.rootLocation * blockSize); //将根目录的 INode 写入磁盘
    disk.write(reinterpret_cast<char*>(&rootINode), sizeof(rootINode));
    disk.seekStart(rootINode.bno * blockSize); //将目录项写入根目录所在的块
    disk.write(reinterpret_cast<char*>(&dir), sizeof(dir));

    //将超级块信息写入磁盘0号块指定区域
    disk.seekStart(sizeof(capacity) + sizeof(isUnformatted) + sizeof(blockSize));
    disk.write(reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo));

    //写入空闲块栈
    auto blocks = stack->getBlocks();
    disk.seekStart(systemInfo.freeBlockStackTop * blockSize);
    disk.read(reinterpret_cast<char*>(blocks), sizeof(blocks[0]) * stack->getMaxSize()); //将磁盘上的空闲块栈数据加载到内存中
    stack->setStackTop(systemInfo.freeBlockStackOffset); //更新栈顶指针

    return true;
}

//挂载的目的是确保该磁盘已经被格式化，以便操作系统可以访问其中的文件和目录
//挂载文件系统，读取超级块信息和空闲块栈信息
//确保磁盘被正确打开并且已经格式化，然后读取必要的信息（如块大小、超级块信息和空闲块栈）以初始化文件系统的内部状态
bool FileSystem::mount() {
    if (!disk.open()) { //打开磁盘
        return false; //如果磁盘打开失败，返回失败
    }
    disk.seekStart(0);
    disk.read(reinterpret_cast<char*>(&capacity), sizeof(capacity)); //读取磁盘容量
    disk.read(reinterpret_cast<char*>(&isUnformatted), sizeof(isUnformatted)); //读取是否格式化标记
    isOpen = true; //设置文件系统为打开状态
    if (!isUnformatted) { //如果文件系统已格式化
        disk.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize)); //读取块大小
        disk.read(reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo)); //读取超级块信息
        auto blocks = stack->getBlocks(); //获取空闲块栈
        disk.seekStart(systemInfo.freeBlockStackTop * blockSize);
        disk.read(reinterpret_cast<char*>(blocks), sizeof(blocks[0]) * stack->getMaxSize());
        stack->setStackTop(systemInfo.freeBlockStackOffset); //设置空闲块栈顶
        disk.seekStart(0);
        return true; //返回成功
    } else {
        return false; //如果文件系统未格式化，返回失败
    }
}

//分配块，从空闲块栈中获取一个空闲块
uint32_t FileSystem::blockAllocate() {
    bool isStackEmpty = stack->empty(); //检查空闲块栈是否为空（为空的含义是当前块栈没有空闲块可以使用）
    if (isStackEmpty) { //如果空闲块栈为空
        auto blocks = stack->getBlocks();
        systemInfo.freeBlockStackTop++; //增加栈顶指针
        systemInfo.freeBlockStackOffset = 0; //重置栈顶偏移
        disk.seekStart(systemInfo.freeBlockStackTop * blockSize); //定位到新的栈顶
        disk.read(reinterpret_cast<char*>(blocks), sizeof(blocks[0]) * stack->getMaxSize()); //读取空闲块栈
        stack->setStackTop(systemInfo.freeBlockStackOffset); //设置空闲块栈顶
    }
    uint32_t ret = stack->getBlock(); //获取一个空闲块
    systemInfo.freeBlockStackOffset++; //增加栈顶偏移
    systemInfo.freeBlockNumber--; //减少空闲块数量
    systemInfo.flag = 1; //设置超级块修改标记
    return ret; //返回分配的块号
}

//释放块，将块归还到空闲块栈
void FileSystem::blockFree(uint32_t bno) {
    bool isStackFull = stack->full(); //检查空闲块栈是否已满
    if (isStackFull) { //如果空闲块栈已满
        auto blocks = stack->getBlocks();
        disk.seekStart(systemInfo.freeBlockStackTop * blockSize); //定位到栈顶
        disk.write(reinterpret_cast<char*>(blocks), sizeof(blocks[0]) * stack->getMaxSize()); //写入空闲块栈
        systemInfo.freeBlockStackTop--; //减少栈顶指针
        systemInfo.freeBlockStackOffset = stack->getMaxSize(); //设置栈顶偏移
        stack->setStackTop(systemInfo.freeBlockStackOffset); //设置空闲块栈顶
    }
    stack->revokeBlock(bno); //归还块
    systemInfo.freeBlockStackOffset--; //减少栈顶偏移
    systemInfo.freeBlockNumber++; //增加空闲块数量
    systemInfo.flag = 1; //设置超级块修改标记
}

//读取块数据
void FileSystem::read(uint32_t bno, uint16_t offset, char* buf, uint16_t sz) {
    uint32_t base = bno * blockSize; //计算块的起始地址
    disk.seekStart(base + offset); //定位到块的具体偏移
    disk.read(buf, sz); //读取数据
}

//写入块数据
void FileSystem::write(uint32_t bno, uint16_t offset, const char* buf, uint16_t sz) {
    uint32_t base = bno * blockSize; //计算块的起始地址
    disk.seekStart(base + offset); //定位到块的具体偏移
    disk.write(buf, sz); //写入数据
}

void FileSystem::readNext(char* buf, uint16_t sz) {
    disk.read(buf, sz); //从磁盘读取指定大小的数据到缓冲区
}

void FileSystem::writeNext(char* buf, uint16_t sz) {
    disk.write(buf, sz); //将缓冲区中的数据写入磁盘
}

void FileSystem::locale(uint32_t bno, uint16_t offset) {
    uint32_t base = bno * blockSize; //计算块号对应的字节偏移量
    disk.seekStart(base + offset); //定位磁盘读写指针到指定位置
}

uint8_t FileSystem::userVerify(std::string userName, std::string password) {
    for (uint8_t i = 0; i < MAX_USER_NUMS; ++i) {
        if (!systemInfo.users[i].uid) {
            continue; //跳过没有用户ID的条目
        }
        if (!strcmp(systemInfo.users[i].name, userName.c_str()) &&
            !strcmp(systemInfo.users[i].password, password.c_str())) {
            return systemInfo.users[i].uid; //如果用户名和密码匹配，返回用户ID
        }
    }
    return 0; //如果没有找到匹配的用户，返回0
}

uint8_t FileSystem::duplicateDetection(std::string userName) {
    for (int8_t i = 0; i < MAX_USER_NUMS; i++) {
        if (!systemInfo.users[i].uid) {
            continue; //跳过没有用户ID的条目
        }
        if (!strcmp(systemInfo.users[i].name, userName.c_str())) {
            return systemInfo.users[i].uid; //如果找到重复的用户名，返回用户ID
        }
    }
    return 0; //如果没有找到重复的用户名，返回0
}

int8_t FileSystem::emptyDetection() {
    for (int8_t i = 0; i < MAX_USER_NUMS; i++) {
        if (!systemInfo.users[i].uid) {
            return i; //返回第一个没有用户ID的条目的索引
        }
    }
    return MAX_USER_NUMS; //如果没有空闲条目，返回最大用户数量
}

void FileSystem::useradd(int8_t idx, std::string name, std::string password) {
    systemInfo.users[idx].uid = idx + 1; //设置用户ID
    strcpy(systemInfo.users[idx].name, name.c_str()); //复制用户名
    strcpy(systemInfo.users[idx].password, password.c_str()); //复制密码
    for (int8_t i = 0; i < MAX_USER_NUMS; i++) {
        systemInfo.trustMatrix[i][idx] = 0; //初始化信任矩阵
        systemInfo.trustMatrix[idx][i] = 0; //初始化信任矩阵
    }
    systemInfo.trustMatrix[idx][idx] = 1; //设置用户对自己的信任
    systemInfo.flag = 1; //标记系统信息已修改
}

void FileSystem::userdel(uint8_t uid) {
    uint8_t idx = uid - 1; //根据用户ID计算索引
    systemInfo.users[idx].uid = 0; //清除用户ID
    for (int8_t i = 0; i < MAX_USER_NUMS; i++) {
        systemInfo.trustMatrix[i][idx] = 0; //清除信任矩阵
        systemInfo.trustMatrix[idx][i] = 0; //清除信任矩阵
    }
    systemInfo.flag = 1; //标记系统信息已修改
}

void FileSystem::passwd(uint8_t uid, std::string password) {
    int8_t idx = uid - 1; //根据用户ID计算索引
    strcpy(systemInfo.users[idx].password, password.c_str()); //复制新的密码
    systemInfo.flag = 1; //标记系统信息已修改
}

bool FileSystem::grantTrustUser(std::string currentUser, std::string targetUser) {
    int8_t curIdx = -1, tarIdx = -1;
    for (int8_t i = 0; i < MAX_USER_NUMS; ++i) {
        if (!systemInfo.users[i].uid) {
            continue; //跳过没有用户ID的条目
        }
        if (!strcmp(systemInfo.users[i].name, currentUser.c_str())) {
            curIdx = i; //找到当前用户的索引
        }
        if (!strcmp(systemInfo.users[i].name, targetUser.c_str())) {
            tarIdx = i; //找到目标用户的索引
        }
    }
    if (curIdx == -1 || tarIdx == -1) {
        return false; //如果当前用户或目标用户不存在，返回false
    } else {
        systemInfo.trustMatrix[curIdx][tarIdx] = 1; //设置信任关系
        systemInfo.flag = 1; //标记系统信息已修改
        return true; //返回true表示操作成功
    }
}

bool FileSystem::revokeTrustUser(std::string currentUser, std::string targetUser) {
    int8_t curIdx = -1, tarIdx = -1;
    for (int8_t i = 0; i < MAX_USER_NUMS; ++i) {
        if (!systemInfo.users[i].uid) {
            continue; //跳过没有用户ID的条目
        }
        if (!strcmp(systemInfo.users[i].name, currentUser.c_str())) {
            curIdx = i; //找到当前用户的索引
        }
        if (!strcmp(systemInfo.users[i].name, targetUser.c_str())) {
            tarIdx = i; //找到目标用户的索引
        }
    }
    if (curIdx == -1 || tarIdx == -1) {
        return false; //如果当前用户或目标用户不存在，返回false
    } else {
        systemInfo.trustMatrix[curIdx][tarIdx] = 0; //取消信任关系
        systemInfo.flag = 1; //标记系统信息已修改
        return true; //返回true表示操作成功
    }
}

uint8_t FileSystem::verifyTrustUser(std::string currentUser, std::string targetUser) {
    int8_t curIdx = -1, tarIdx = -1;
    for (int8_t i = 0; i < MAX_USER_NUMS; ++i) {
        if (!systemInfo.users[i].uid) {
            continue; //跳过没有用户ID的条目
        }
        if (!strcmp(systemInfo.users[i].name, currentUser.c_str())) {
            curIdx = i; //找到当前用户的索引
        }
        if (!strcmp(systemInfo.users[i].name, targetUser.c_str())) {
            tarIdx = i; //找到目标用户的索引
        }
    }
    if (curIdx == -1 || tarIdx == -1) {
        return 0; //如果当前用户或目标用户不存在，返回0
    } else {
        return systemInfo.trustMatrix[curIdx][tarIdx]; //返回信任关系的状态
    }
}

uint8_t FileSystem::verifyTrustUser(uint8_t currentUserUid, uint8_t targetUserUid) {
    int8_t curIdx = currentUserUid - 1, tarIdx = targetUserUid - 1; //计算用户索引
    return systemInfo.trustMatrix[curIdx][tarIdx]; //返回信任关系的状态
}

void FileSystem::getUser(uint8_t uid, User* user) {
    int8_t idx = uid - 1; //根据用户ID计算索引
    user->uid = systemInfo.users[idx].uid; //获取用户ID
    strcpy(user->name, systemInfo.users[idx].name); //复制用户名
    strcpy(user->password, systemInfo.users[idx].password); //复制密码
}

uint32_t FileSystem::getRootLocation() {
    return systemInfo.rootLocation; //返回根目录的位置
}

//上级模块可以设置在进行 10 次或者其他次数以后执行一次 update函数，将数据持久化，以避免频繁的磁盘写入操作。
void FileSystem::update() {
    if (systemInfo.flag) {
        systemInfo.flag = 0;
        //写入基础信息
        disk.seekStart(sizeof(capacity));
        disk.write(reinterpret_cast<char*>(&isUnformatted), sizeof(isUnformatted));
        disk.write(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
        disk.write(reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo));
        //写入空闲块栈信息
        auto blocks = stack->getBlocks();
        disk.seekStart(systemInfo.freeBlockStackTop * blockSize);
        disk.write(reinterpret_cast<char*>(blocks), sizeof(blocks[0]) * stack->getMaxSize());
    }
}

