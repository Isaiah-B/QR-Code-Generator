#pragma once

#include <string>
#include <bitset>
#include <unordered_set>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <array>
#include <deque>

namespace qr
{
    enum EC_LEVEL
    {
        L, M, Q, H
    };

    enum class MODE
    {
        NONE,
        NUMERIC,
        ALPHANUMERIC,
        BYTE,
        KANJI
    };

    extern std::unordered_set<char> AlphanumericSymbols;

    extern bool IsAlphanumeric(char c);
    extern std::string IntStringToBinaryString(const std::string& str);
    extern std::string AlphanumericStringtoBinaryString(const std::string& str);
    extern int GetAlphanumericCode(char c);

    extern std::vector<std::byte> BinaryStrToByteArr(std::deque<uint8_t>& bitStr);

    extern int GetCharacterCountBits(int version, MODE mode);
    extern int GetNumCodewords(int version, EC_LEVEL ecLevel);
}