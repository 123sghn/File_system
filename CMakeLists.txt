cmake_minimum_required(VERSION 3.16)
project(FileSystem)

set(CMAKE_CXX_STANDARD 17)

add_executable(FileSystem
        main.cpp

        src/DiskDriver.cpp src/DiskDriver.h
        src/FileSystem.cpp src/FileSystem.h
        src/UserInterface.cpp src/UserInterface.h
        src/Shell.cpp src/Shell.h

        src/Constraints.h
        src/Tools.h

        src/entity/FreeBlockStack.h src/entity/FreeBlockStack.cpp
        src/entity/Vim.h src/entity/Vim.cpp

        src/entity/Directory.h
        src/entity/FileSystemInfo.h
        src/entity/FileIndex.h
        src/entity/INode.h
        src/entity/User.h)
