#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace qrData
{
    const size_t EC_DATA_CODEWORDS_ARRAY_LEN = 41;
    const size_t EC_DATA_CAPACITY_ARRAY_LEN = 40;

    extern const std::array<int, EC_DATA_CODEWORDS_ARRAY_LEN> EC_L_DATA_CODEWORDS;
    extern const std::array<int, EC_DATA_CODEWORDS_ARRAY_LEN> EC_M_DATA_CODEWORDS;
    extern const std::array<int, EC_DATA_CODEWORDS_ARRAY_LEN> EC_Q_DATA_CODEWORDS;
    extern const std::array<int, EC_DATA_CODEWORDS_ARRAY_LEN> EC_H_DATA_CODEWORDS;

    typedef std::array<std::array<int,4>, EC_DATA_CAPACITY_ARRAY_LEN> ecDataCapTable;
    extern const ecDataCapTable EC_L_DATA_CAPACITY;
    extern const ecDataCapTable EC_M_DATA_CAPACITY;
    extern const ecDataCapTable EC_Q_DATA_CAPACITY;
    extern const ecDataCapTable EC_H_DATA_CAPACITY;

    extern const std::array<std::vector<std::pair<uint8_t,uint8_t>>, 160> EC_DATA_CODEWORD_GROUPS;

    extern const std::array<uint8_t, 256> LOG_ANTILOG_INTEGERS;
    extern const std::array<uint8_t, 256> LOG_ANTILOG_EXPONENTS;
};