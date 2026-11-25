#include "Generator.h"

#include <iostream>
#include <algorithm>

#include "helpers.h"
#include "DataTables.h"

namespace qr
{
    QRGenerator::QRGenerator(EC_LEVEL ecLevel, int version)
        : m_EcLevel(ecLevel), m_Version(version)
    {
    }

    void QRGenerator::Init(const std::string &input)
    {
        m_Mode = DetermineMode(input);

        if (m_Version == -1) {
            m_Version = EstimateQRVersion(input.size());
        }
    }

    MODE QRGenerator::DetermineMode(const std::string &input)
    {
        MODE mode = MODE::NONE;

        std::string::const_iterator it = input.begin();
    
        if (isdigit(*it)) {
            mode = MODE::NUMERIC;

            while (isdigit(*it) && it != input.end()) {
                it++;
            }

            if (it == input.end())
                return mode;
        }

        if (IsAlphanumeric(*it) && it != input.end()) {
            mode = MODE::ALPHANUMERIC;

            while (IsAlphanumeric(*it) && it != input.end()) {
                it++;
            }

            if (it == input.end())
                return mode;
        }
        
        // if byte/binary data
        mode = MODE::BYTE;

        return mode;
    }

    std::vector<std::byte> QRGenerator::Encode(const std::string &input)
    {
        switch (m_Mode) {
            case MODE::NUMERIC:
                return EncodeNumeric(input);
            case MODE::ALPHANUMERIC:
                return EncodeAlphanumeric(input);
            case MODE::BYTE:
                return EncodeByte(input);
            default:
                throw std::runtime_error("Invalid encoding mode");
        }
    }

    std::vector<std::byte> QRGenerator::EncodeNumeric(const std::string &input)
    {
        // Binary string consisting of mode indicator, char count indicator,
        // and input data bits 
        std::deque<uint8_t> bitStr = ToNumericBitString(input);

        // Append the terminating sequence and convert the bit string to
        // a vector of bytes
        std::vector<std::byte> byteArr = AppendTerminatingSequence(bitStr);

        return byteArr;
    }

    // Returns bitstream containing numeric mode indicator, character count indicator,
    // and input data bit stream
    std::deque<uint8_t> QRGenerator::ToNumericBitString(const std::string &input)
    {
        size_t inputSize = input.size();

        std::deque<uint8_t> bitString{};
        
        // Concatenate the numeric mode indicator and character count indicator
        uint8_t numericModeIndicator = 1;
        int charCount = GetCharacterCountBits(m_Version, m_Mode);

        for (int i{0}; i < charCount; i++) {
            bitString.push_front((inputSize >> i) & 1);
        }
        for (int i{0}; i < 4; i++) {
            bitString.push_front((numericModeIndicator >> i) & 1);
        }

        
        // Divide input into groups of 3 and add their 10-bit binary
        // values to the result
        // If the group size is 1 or 2, the binary value is 4 or 7 bits respectively
        size_t pos = 0;
        size_t gSize = 3;

        while (pos < inputSize) {
            if ((inputSize - pos) < 3)
                gSize = inputSize - pos;

            auto binaryStr = IntStringToBinaryString(input.substr(pos, gSize));
            for (char c : binaryStr) {
                bitString.push_back(c - '0');
            }

            pos += gSize;
        }

        return bitString;
    }


    std::vector<std::byte> QRGenerator::EncodeAlphanumeric(const std::string &input)
    {
        std::deque<uint8_t> bitStr = ToAlphanumericBitString(input);

        std::vector<std::byte> byteArr = AppendTerminatingSequence(bitStr);

        return byteArr;
    }

    std::deque<uint8_t> QRGenerator::ToAlphanumericBitString(const std::string &input)
    {
        size_t inputSize = input.size();
    
        std::deque<uint8_t> bitString{};

        // Concatenate the alphanumeric mode indicator and character count indicator
        
        uint8_t alphanumModeIndicator = 2;
        int charCount = GetCharacterCountBits(m_Version, m_Mode);

        for (int i{0}; i < charCount; i++) {
            bitString.push_front((inputSize >> i) & 1);
        }
        for (int i{0}; i < 4; i++) {
            bitString.push_front((alphanumModeIndicator >> i) & 1);
        }
        
    
        // Divide input into groups of 2 and add their 11-bit binary
        // values to the result
        // If the group size is 1, the binary value is 6 bits
        size_t pos = 0;
        int gSize = 2;
    
        while (pos < inputSize) {
            if ((inputSize - pos) == 1)
                gSize = 1;
    
            std::string binaryStr = AlphanumericStringtoBinaryString(input.substr(pos, gSize));
            for (char c : binaryStr) {
                bitString.push_back(c - '0');
            }
            
            pos += gSize;
        }
    
        return bitString;
    }


    std::vector<std::byte> QRGenerator::EncodeByte(const std::string &input)
    {
        std::deque<uint8_t> bitStr = ToByteModeBitString(input);

        std::vector<std::byte> byteArr = AppendTerminatingSequence(bitStr);

        return byteArr;
    }

    std::deque<uint8_t> QRGenerator::ToByteModeBitString(const std::string &input)
    {
        size_t inputSize = input.size();

        std::deque<uint8_t> bitString{};

        // Append byte mode indicator and character count indicator binary strings to result
        
        uint8_t byteModeIndicator = 4;
        int charCount = GetCharacterCountBits(m_Version, m_Mode);

        for (int i{0}; i < charCount; i++) {
            bitString.push_front((inputSize >> i) & 1);
        }
        for (int i{0}; i < 4; i++) {
            bitString.push_front((byteModeIndicator >> i) & 1);
        }
        

        for (char byte : input) {
            for (int i{7}; i >= 0; i--) {
                bitString.push_back((byte >> i) & 1);
            }
        }

        return bitString;
    }



    std::vector<std::byte> QRGenerator::AppendTerminatingSequence(std::deque<uint8_t> &bitstr)
    {
        int nCodeWords = GetNumCodewords(m_Version, m_EcLevel);
        int dataCapacity = nCodeWords * 8;

        // If the bitstream is shorter than the data capacity add a terminating sequence
        // by zero padding with at most 4 bits
        if (bitstr.size() < dataCapacity) {
            for (int i{0}; i < 4; i++) {
                bitstr.push_back(0);
                if (bitstr.size() == dataCapacity)
                    break;
            }

            // If the bitstream is still shorter than the data capacity,
            // pad it with zeroes until it ends in a complete byte
            while ((bitstr.size() < dataCapacity) && (bitstr.size() % 8 != 0)) {
                bitstr.push_back(0);
            }
        }

        // Convert the bit string to a byte vector
        std::vector<std::byte> byteVec = BinaryStrToByteArr(bitstr);

        // If the bitstream is still shorter than the qr version's data capacity,
        // pad it with codewords 0xEC and 0x11 until it fills the capacity
        while (byteVec.size() < nCodeWords) {
            byteVec.push_back(std::byte{0xEC});

            if (byteVec.size() == nCodeWords)
                break;

            byteVec.push_back(std::byte{0x11});
        }

        return byteVec;
    }

    int QRGenerator::EstimateQRVersion(size_t inputSize)
    {
        auto findVersionIndex = [&](const qrData::ecDataCapTable& dataCapacityTable) -> int
        {
            int dataCapModeIdx{-1};

            switch (m_Mode) {
                case MODE::NUMERIC: dataCapModeIdx = 0;      break;
                case MODE::ALPHANUMERIC: dataCapModeIdx = 1; break;
                case MODE::BYTE: dataCapModeIdx = 2;         break;
                case MODE::KANJI: dataCapModeIdx = 3;        break;
            }

            auto it = std::find_if(dataCapacityTable.begin(), dataCapacityTable.end(), [&](auto& dataCapArr) {
                return dataCapArr[dataCapModeIdx] >= inputSize;
            });
    
            // Index of found value
            return (it - dataCapacityTable.begin()) + 1;
        };

        int version = -1;
        switch (m_EcLevel) {
            case EC_LEVEL::L:
                version = findVersionIndex(qrData::EC_L_DATA_CAPACITY);
                break;
            case EC_LEVEL::M:
                version = findVersionIndex(qrData::EC_M_DATA_CAPACITY);
                break;
            case EC_LEVEL::Q:
                version = findVersionIndex(qrData::EC_Q_DATA_CAPACITY);
                break;
            case EC_LEVEL::H:
                version = findVersionIndex(qrData::EC_H_DATA_CAPACITY);
                break;
        }
        
        return version;
    }
}