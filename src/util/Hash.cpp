/* Hash.cpp
按照 FIPS 180-4 标准实现 SHA256 算法。
包含 64 个常量的 K 表、6 个逻辑函数以及核心的 64 轮压缩函数 sha256_transform，
最终输出的 sha256() 函数完成填充、分块处理和摘要拼接。
 */

#include "Hash.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <vector>
/*
 K[64] - SHA256 算法使用的 64 个常量
 这些常量是前 64 个质数（2,3,5,...,311）的立方根的小数部分的
 前 32 位二进制表示。每个质数的立方根取小数部分 × 2^32 取整。
 */
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/*
rotr - 循环右移，将 32 位无符号整数向右循环移动 n 位。
 */
static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}
/*
  ch - 选择函数，根据 x 的每个位选择 y 或 z 的对应位：当 x 位为 1 时选 y，为 0 时选 z。
 */
static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

/*maj - 多数函数，对 x、y、z 的每个位取多数值（至少两个为 1 则结果为 1）。
 */
static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

/*
 sigma0 - 大 Σ0 函数
 用于压缩函数中变量 a 的更新，对 x 做 2、13、22 位的循环右移后再异或。
 */
static inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

/* sigma1 - 大 Σ1 函数
用于压缩函数中变量 e 的更新，对 x 做 6、11、25 位的循环右移后再异或。
 */
static inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

/*
 gamma0 - 小 σ0 函数， 用于消息调度（扩展 48 个字），对 x 做 7、18 位的循环右移和 3 位右移后异或。
 */
static inline uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

/*
gamma1 - 小 σ1 函数，用于消息调度（扩展 48 个字），对 x 做 17、19 位的循环右移和 10 位右移后异或。
 */
static inline uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

/*sha256_transform - SHA256 核心压缩函数

对每个 512 位（64 字节）的数据块执行 64 轮压缩。
处理过程：
  1. 将 16 个大端字复制到 W[0..15]
  2. 通过 gamma 函数扩展出 W[16..63]
  3. 初始化 8 个工作变量 a~h 为当前状态
  4. 执行 64 轮压缩（使用 K 常量、sigma/ch/maj 函数）
  5. 将结果累加到 state 中
 state  当前 256 位摘要状态（8 × 32 位），会被更新
 block  512 位（64 字节）的数据块
 */
static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    // 第 1 步：将 64 字节的块拆分为 16 个大端序的 32 位字 W[0..15]
    uint32_t W[64];
    for (int i = 0; i < 16; i++) {
        W[i] = ((uint32_t)block[i * 4]     << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] << 8)
             | ((uint32_t)block[i * 4 + 3]);
    }

    // 第 2 步：将 W 扩展到 64 个字 W[16..63]
    for (int i = 16; i < 64; i++) {
        W[i] = gamma1(W[i - 2]) + W[i - 7] + gamma0(W[i - 15]) + W[i - 16];
    }

    // 第 3 步：初始化工作变量为当前状态
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    // 第 4 步：执行 64 轮压缩
    for (int i = 0; i < 64; i++) {
        // T1 = h + Σ1(e) + Ch(e,f,g) + K[i] + W[i]
        uint32_t T1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
        // T2 = Σ0(a) + Maj(a,b,c)
        uint32_t T2 = sigma0(a) + maj(a, b, c);
        // 更新 8 个工作变量（右移一个位置）
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    // 第 5 步：将本轮结果累加到状态中
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/*
sha256 - 计算输入字符串的 SHA256 哈希值
   完整的 SHA256 计算流程：
   1. 初始化 8 个状态向量（前 8 个质数平方根小数部分的前 32 位）
    2. 计算填充后的总块数（追加 0x80、填充 0、追加原始长度）
   3. 对每个 512 位块调用 sha256_transform
   4. 将最终的 8 个 32 位状态拼接为 64 字符的十六进制字符串
input 待哈希的字符串
return 小写 64 字符的十六进制哈希摘要
 */
std::string sha256(const std::string& input) {
    // 初始化状态向量：前 8 个质数（2,3,5,7,11,13,17,19）平方根小数部分 × 2^32
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // 原始数据的位长度（用于填充的最后 8 字节）
    uint64_t bitlen = input.size() * 8;
    // 计算需要的总块数：原始数据 + 0x80 填充字节 + 8 字节长度，向上取整到 64 的倍数
    size_t block_count = (input.size() + 9 + 63) / 64;

    // 分配缓冲区并初始化为 0（填充 0 自动完成）
    std::vector<uint8_t> buf(block_count * 64, 0);
    std::memcpy(buf.data(), input.data(), input.size());
    // 在数据末尾追加 0x80 作为填充起始标记
    buf[input.size()] = 0x80;

    // 将原始位长度以大端序写入最后 8 个字节
    for (int i = 0; i < 8; i++) {
        buf[buf.size() - 8 + i] = (bitlen >> (56 - i * 8)) & 0xff;
    }

    // 逐块执行 SHA256 压缩
    for (size_t i = 0; i < block_count; i++) {
        sha256_transform(state, buf.data() + i * 64);
    }

    // 将 8 个 32 位状态值拼接为 64 个字符的十六进制字符串
    std::ostringstream oss;
    for (int i = 0; i < 8; i++) {
        oss << std::hex << std::setw(8) << std::setfill('0') << state[i];
    }
    return oss.str();
}

