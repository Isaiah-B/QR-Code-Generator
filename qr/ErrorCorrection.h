#pragma once

#include <vector>
#include <cstddef>

#include "helpers.h"
#include "Polynomial.h"

using namespace polynomial;

namespace qrEc
{
    extern const std::array<int, 160> ERROR_CODEWORD_COUNTS;

    std::vector<std::vector<std::byte>> SubdivideCodewords(const std::vector<std::byte>& codewords,
                                                            int version,
                                                            qr::EC_LEVEL ecLevel);

    Polynomial CalcGeneratorPolynomial(int nCodewords);
    std::vector<std::byte> GenerateErrorCodewords(const std::vector<std::byte>& codewords, int nErrorCodewords, Polynomial generator);
    std::vector<std::byte> ConstructFinalMessage(const std::vector<std::byte>& codewords, int version, qr::EC_LEVEL ecLevel);

    std::vector<uint8_t> GenerateErrorCorrectedFormatStr(uint8_t formatString);
    std::vector<uint8_t> GenerateVersionInformationString(int version);
}