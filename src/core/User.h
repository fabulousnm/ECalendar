//User.h定义用户类
 
#ifndef ECALENDER_USER_H
#define ECALENDER_USER_H
#include <string>

class User {
public:
    std::string username;       // 用户名
    std::string passwordHash;   // 密码的哈希值

    User() = default;
    User(const std::string& username, const std::string& rawPassword);

    /*checkPassword 用于验证密码是否正确
    对输入的明文密码计算 SHA256 哈希，与已存储的哈希值比对*/

    bool checkPassword(const std::string& rawPassword) const;
};

#endif // ECALENDER_USER_H

