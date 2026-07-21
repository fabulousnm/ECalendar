/* @file test_hash.cpp 测试hash函数效果的程序
 */

#include "Hash.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string input;
      input = argv[1];
    
    // 计算并输出哈希值
    std::string hash = sha256(input);
    
    std::cout << "\n输入: " << input << std::endl;
    std::cout << "SHA256: " << hash << std::endl;
    std::cout << "长度: " << hash.length() << " 字符" << std::endl;
    
    return 0;
}
