#ifndef FILESYSTEM_USER_H
#define FILESYSTEM_USER_H

#include <cstdint>
#include "../Constraints.h"

class User {
public:
    uint8_t uid;
    char name[USERNAME_PASWORD_LENGTH];
    char password[USERNAME_PASWORD_LENGTH];
};

#endif //FILESYSTEM_USER_H
