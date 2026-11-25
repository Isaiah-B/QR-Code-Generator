#include "helpers.h"

#include <cassert>
#include <stdexcept>

#include "DataTables.h"

namespace qr
{
    std::unordered_set<char> AlphanumericSymbols = {0x20,0x24,0x25,0x2A,0x2B,0x2D,0x2E,0x2F,0x3A};

    bool IsAlphanumeric(char c)
    {
        // Return true if c is
        // A-Z, 0-9, space, or a symbol: $, %, *, +, -, ., /, :
        return isupper(c) || isdigit(c) || AlphanumericSymbols.contains(c);
    }

    std::string IntStringToBinaryString(const std::string& str)
    {
        size_t numLen = str.size();
        assert(numLen >= 1 && numLen <= 3);

        // Convert each group into 10-bit binary equivalent
        int num = atoi(str.c_str());

        if (numLen == 3) {
            std::bitset<10> groupBinary(num);
            return groupBinary.to_string();
        }
        else if (numLen == 2) {
            std::bitset<7> groupBinary(num);
            return groupBinary.to_string();
        }    
        else {
            std::bitset<4> groupBinary(num);
            return groupBinary.to_string();
        }
    }

    std::string AlphanumericStringtoBinaryString(const std::string& str)
    {
        assert(str.size() == 2 || str.size() == 1);

        if (str.size() == 2) {
            int n1 = GetAlphanumericCode(str[0]);
            int n2 = GetAlphanumericCode(str[1]);

            int val = (n1*45) + n2;
            std::bitset<11> binaryResult(val);
            return binaryResult.to_string();
        }
        else {
            int val = GetAlphanumericCode(str[0]);
            std::bitset<6> binaryResult(val);
            return binaryResult.to_string();
        }
    }

    int GetAlphanumericCode(char c)
    {
        // 0-9
        if (c >= 48 && c <= 57) {
            return c - 48;
        }

        // A-Z
        else if (c >= 65 && c <= 90) {
            return c - 55;
        }

        // alphanumeric symbol
        else if (AlphanumericSymbols.contains(c)) {
            switch (c) {
                case 0x20: return 36;
                case 0x24: return 37;
                case 0x25: return 38;
                case 0x2A: return 39;
                case 0x2B: return 40;
                case 0x2D: return 41;
                case 0x2E: return 42;
                case 0x2F: return 43;
                case 0x3A: return 44;
            }
        }

        return -1;
    }


    std::vector<std::byte> BinaryStrToByteArr(std::deque<uint8_t>& bitStr)
    {
        assert(bitStr.size() % 8 == 0);

        int nBytes = bitStr.size() / 8;
        
        std::vector<std::byte> res(nBytes);

        uint8_t currByte;
        for (int i{0}; i < nBytes; i++) {
            currByte = 0;
            for (int bitSh{7}; bitSh >= 0; bitSh--) {
                currByte |= (bitStr.front() << bitSh);
                bitStr.pop_front();
            }

            res[i] = std::byte{currByte};
        }

        return res;
    }

    // Returns number of bits in the character count indicator determined
    // by the version and mode or -1 on error
    int GetCharacterCountBits(int version, MODE mode)
    {
        if (version >= 1 && version <= 9) {
            switch (mode) {
                case MODE::NUMERIC:      return 10;
                case MODE::ALPHANUMERIC: return 9;
                case MODE::BYTE:         return 8;
                case MODE::KANJI:        return 8;
            }
        }

        else if (version >= 10 && version <= 26) {
            switch (mode) {
                case MODE::NUMERIC:      return 12;
                case MODE::ALPHANUMERIC: return 11;
                case MODE::BYTE:         return 16;
                case MODE::KANJI:        return 10;
            }
        }

        else if (version >= 27 && version <= 40) {
            switch (mode) {
                case MODE::NUMERIC:      return 14;
                case MODE::ALPHANUMERIC: return 13;
                case MODE::BYTE:         return 16;
                case MODE::KANJI:        return 12;
            }
        }

        return -1;
    }

    int GetNumCodewords(int version, EC_LEVEL ecLevel)
    {
        assert(version >= 1 && version <= 40);

        switch (ecLevel) {
            case EC_LEVEL::L:
                return qrData::EC_L_DATA_CODEWORDS[version];
            case EC_LEVEL::M:
                return qrData::EC_M_DATA_CODEWORDS[version];
            case EC_LEVEL::Q:
                return qrData::EC_Q_DATA_CODEWORDS[version];
            case EC_LEVEL::H:
                return qrData::EC_H_DATA_CODEWORDS[version];
        }

        throw std::runtime_error("GetNumCodewords(): Invalid error correction level: " + ecLevel);
    }
}