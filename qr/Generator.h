#pragma once

#include <string>
#include <bitset>
#include <cstddef>
#include <vector>
#include <deque>
#include <cstdint>

#include "helpers.h"

namespace qr
{
    class QRGenerator
    {
    public:
        explicit QRGenerator(EC_LEVEL ecLevel = EC_LEVEL::L, int version = -1);
        void Init(const std::string& input);

        MODE DetermineMode(const std::string& input);
        std::vector<std::byte> Encode(const std::string& input);
        std::vector<std::byte> EncodeNumeric(const std::string& input);
        std::vector<std::byte> EncodeAlphanumeric(const std::string& input);
        std::vector<std::byte> EncodeByte(const std::string& input);

        int GetVersion() const { return m_Version; }
        EC_LEVEL GetECLevel() const { return m_EcLevel; }

    private:
        std::deque<uint8_t> ToNumericBitString(const std::string& input);
        std::deque<uint8_t> ToAlphanumericBitString(const std::string& input);
        std::deque<uint8_t> ToByteModeBitString(const std::string& input);

        int EstimateQRVersion(size_t inputSize);

        std::vector<std::byte> AppendTerminatingSequence(std::deque<uint8_t>& bitStr);
    private:
        EC_LEVEL m_EcLevel = EC_LEVEL::L;
        MODE m_Mode = MODE::NONE;
        int m_Version = -1;
    };

}