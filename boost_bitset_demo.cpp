#include <iostream>
#include <fstream>
#include <boost/dynamic_bitset.hpp>
#include <boost/utility/binary.hpp>

#define WRITE_ULONG_SIZE 32
#define BITS_PER_BYTE 8

// 该函数用户bit裁剪, 将 SourceSize 大小的 bitset 转换为 TargetSize 大小的 bitset
// 原始 SourceSize > TargetSize
// truncate head: delete MSB
// truncate tail: delete LSB
// HEAD ...... TAIL
// MSB  ......  LSB
enum class TruncateDirection {
    HEAD,  // 删除头部多余位
    TAIL   // 删除尾部多余位
};

template <size_t SourceSize, size_t TargetSize>
boost::dynamic_bitset<> convertBitset(const boost::dynamic_bitset<>& sourceBits, TruncateDirection direction) {
    boost::dynamic_bitset<> targetBits;
    if (direction == TruncateDirection::HEAD) {
        for (size_t i = 0; i < TargetSize; i++) {
            if (i < SourceSize) {
                // targetBits[i] = sourceBits[i];
                targetBits.push_back(sourceBits[i]);
            }
        }
    } else if (direction == TruncateDirection::TAIL) {
        for (size_t i = 0; i < TargetSize; i++) {
            if (i < SourceSize) {
                // targetBits[i] = sourceBits[SourceSize - TargetSize + i];
                targetBits.push_back(sourceBits[SourceSize - TargetSize + i]);
            }
        }
    }
    return targetBits;
}

// static boost::dynamic_bitset<> writeBits2File(const boost::dynamic_bitset<>& bitset, const std::string& filename) {
static boost::dynamic_bitset<> writeBits2File(const boost::dynamic_bitset<>& bitset, std::ofstream& outputFile) {
    // std::ofstream outputFile(filename, std::ios::binary | std::ios::out);
    // if (!outputFile.is_open()) {
    //     std::cerr << "Error opening file for writing." << std::endl;
    //     return boost::dynamic_bitset<>();
    // }

    std::size_t numFullBlocks = bitset.size() / WRITE_ULONG_SIZE;
    std::cout << "bitset.size: " << bitset.size() << "\nnumFullBlocks: " << numFullBlocks << std::endl;
    for (std::size_t i = 0; i < numFullBlocks; ++i) {
        boost::dynamic_bitset<> block(WRITE_ULONG_SIZE);
        for (std::size_t j = 0; j < WRITE_ULONG_SIZE; ++j) {
            block[j] = bitset[i * WRITE_ULONG_SIZE + j];
        }
        std::cout << "Write Block: " << block << std::endl;
        unsigned long ulongValue = static_cast<unsigned long>(block.to_ulong());
        std::cout << "" << ulongValue << std::endl;
        std::printf("ulongValue: %ld %#lx\n", ulongValue, ulongValue);
        outputFile.write(reinterpret_cast<const char*>(&ulongValue), WRITE_ULONG_SIZE / BITS_PER_BYTE); // sizeof(ulongValue)
    }

    std::size_t remainingBits = bitset.size() % WRITE_ULONG_SIZE;
    std::printf("remainingBits: %ld\n", remainingBits);
    boost::dynamic_bitset<> remainingBitsSet;

    // boost::dynamic_bitset<> remainingBitsSet(remainingBits);
    for (std::size_t i = 0; i < remainingBits; ++i) {
        // remainingBitsSet[i] = bitset[numFullBlocks * 32 + i];
        remainingBitsSet.push_back(bitset[numFullBlocks * 32 + i]);
    }
    std::cout << remainingBitsSet << std::endl;
    // unsigned long ulongValue = static_cast<unsigned long>(remainingBitsSet.to_ulong());
    // outputFile.write(reinterpret_cast<const char*>(&ulongValue), (remainingBits + 7) / 8); // 写入不足8位的部分

    // outputFile.close();

    return remainingBitsSet;
}

int main() {
    // std::string bitString = "100100110011001100110011001100110011";
    // boost::dynamic_bitset<> inputBits(bitString);
    // std::cout << inputBits << std::endl;

    // boost::dynamic_bitset<> ret = writeBits2File(inputBits, "output.bin");
    // std::cout << ret << std::endl;
    std::ifstream inputFile("input.bin", std::ios::binary);
    std::ofstream outputFile("output.bin", std::ios::binary);

    static boost::dynamic_bitset<> WriteBitSet;

    if (!inputFile || !outputFile) {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }

    const int bufferSize = 16; // 128 bits / 8 bits per byte
    char buffer[bufferSize];

    while (inputFile.read(buffer, bufferSize)) {
        boost::dynamic_bitset<> bitset128;

        for (int i = 0; i < bufferSize; i++) {
            printf("%02X ", static_cast<unsigned char>(buffer[i]));
            for (int j = 0; j < 8; j++) {
                // bitset128[i * 8 + j] = (buffer[i] >> (7 - j)) & 1;
                // bitset128[i * 8 + j] = (buffer[i] >> (j)) & 1;
                bitset128.push_back((buffer[i] >> (j)) & 1);
            }
        }

        boost::dynamic_bitset<> convertedBits = convertBitset<128, 126>(bitset128, TruncateDirection::TAIL);

        std::cout << "\nOriginal  128-bit: " << bitset128 << std::endl;
        std::cout << "Converted 126-bit: " << convertedBits << std::endl;

        for (size_t i = 0; i < convertedBits.size(); ++i) {
            WriteBitSet.push_back(convertedBits[i]);
        }

        std::cout << "WriteBitSet size: " << WriteBitSet.size() << std::endl;

        boost::dynamic_bitset<> tmp;
        if (WriteBitSet.size() >= WRITE_ULONG_SIZE) {
            // 待写入 bitset, 返回剩余bits
            tmp = writeBits2File(WriteBitSet, outputFile);
        }
        WriteBitSet.reset();
        WriteBitSet = tmp;

        // if ((WriteBitSet.size() % 8) == 0) {
        //     for (size_t i = 0; i < WriteBitSet.size(); i += 32) {
        //         uint32_t value = static_cast<uint32_t>(WriteBitSet.to_ulong());
        //         outputFile.write(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
        //         WriteBitSet >>= 32;
        //     }

        //     WriteBitSet.clear();
        // }
    }
    inputFile.close();
    outputFile.close();
    return 0;
}
