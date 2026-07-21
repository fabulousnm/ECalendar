#include "core/User.h"
#include "util/Hash.h"

User::User(const std::string& username, const std::string& rawPassword)
    : username(username), passwordHash(sha256(rawPassword)) {}
// checkPassword - 验证密码

bool User::checkPassword(const std::string& rawPassword) const {
    return passwordHash == sha256(rawPassword);
}

