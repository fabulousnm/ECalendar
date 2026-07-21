/*
@file Hash.h
@brief SHA256 哈希函数声明
创建基于 FIPS 180-4 标准的 SHA256 哈希计算接口。
用于用户密码的安全存储与验证。
*/
#ifndef ECALENDER_HASH_H
#define ECALENDER_HASH_H
#include <string>

/*
函数说明：
sha256 - 计算输入字符串的 SHA256 哈希值
input 待哈希的输入字符串（原始密码等）
 返回64 字符的十六进制哈希字符串（小写）
使用纯 C++ 实现 SHA256 算法，不依赖外部库。
*/
std::string sha256(const std::string& input);

#endif // ECALENDER_HASH_H

