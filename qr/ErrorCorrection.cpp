#include "ErrorCorrection.h"

#include <cassert>
#include <cstdint>
#include <deque>

#include "DataTables.h"

namespace qrEc
{
    // Number of error code blocks per group
    constexpr std::array<int, 160> ERROR_CODEWORD_COUNTS {
        7,10,13,17,  10,16,22,28, 15,26,18,22, 20,18,26,16,
        26,24,18,22, 18,16,24,28, 20,18,18,26, 24,22,22,26,
        30,22,20,24, 18,26,24,28, 20,30,28,24, 24,22,26,28,
        26,22,24,22, 30,24,20,24, 22,24,30,24, 24,28,24,30,
        28,28,28,28, 30,26,28,28, 28,26,26,26, 28,26,30,28,
        28,26,28,30, 28,28,30,24, 30,28,30,30, 30,28,30,30,
        26,28,30,30, 28,28,28,30, 30,28,30,30, 30,28,30,30,
        30,28,30,30, 30,28,30,30, 30,28,30,30, 30,28,30,30,
        30,28,30,30, 30,28,30,30, 30,28,30,30, 30,28,30,30,
        30,28,30,30, 30,28,30,30, 30,28,30,30, 30,28,30,30
    };

    std::vector<std::vector<std::byte>> SubdivideCodewords(const std::vector<std::byte> &codewords, int version, qr::EC_LEVEL ecLevel)
    {
        int codewordGroupIndex = ((version - 1) * 4) + ecLevel;
        std::vector<std::pair<uint8_t,uint8_t>> ecGroups = qrData::EC_DATA_CODEWORD_GROUPS[codewordGroupIndex];

        std::vector<std::vector<std::byte>> dataCodewordBlocks{};

        auto it = codewords.begin();
        for (int groupIdx{0}; groupIdx < ecGroups.size(); groupIdx++) {
            std::pair<int,int> group = ecGroups[groupIdx];
            
             for (int i{0}; i < group.first; i++) {
                dataCodewordBlocks.push_back({ it, it + group.second });
                it += group.second;
             }
        }

        return dataCodewordBlocks;
    }

    std::vector<std::byte> ConstructFinalMessage(const std::vector<std::byte> &codewords, int version, qr::EC_LEVEL ecLevel)
    {   
        // Generate error codewords for each data block and combine
        // them to create the final message

        auto dataCodewordBlocks = SubdivideCodewords(codewords, version, ecLevel);

        size_t totalDataCodewords = 0;
        size_t totalErrorCodewords = 0;

        // Calculate the generator polynomial used for generating
        // the error codewords
        int numErrorCodewords = ERROR_CODEWORD_COUNTS[((version - 1) * 4) + ecLevel];
        Polynomial generatorPolynomial = CalcGeneratorPolynomial(numErrorCodewords);
        
        
        // Generate error codewords for each data codeword block
        // and count the total number of data and error codewords
        std::vector<std::vector<std::byte>> errorCodewordBlocks;

        for (const std::vector<std::byte>& block : dataCodewordBlocks) {
            auto errorCodewordBlock = GenerateErrorCodewords(block, numErrorCodewords, generatorPolynomial);
            errorCodewordBlocks.push_back(errorCodewordBlock);

            totalDataCodewords += block.size();
            totalErrorCodewords += errorCodewordBlock.size();
        }

        // Structure final message by either appending the
        // error codeword block to the data codeword block,
        // or interleaving the data and error codewords

        // If there is one data codeword block, simply append the error
        // codeword block to the end of it 
        if (dataCodewordBlocks.size() == 1) {
            std::vector<std::byte> codewordBlock = dataCodewordBlocks[0];
            std::vector<std::byte> errorBlock = errorCodewordBlocks[0];

            codewordBlock.reserve(codewordBlock.size() + errorBlock.size());
            codewordBlock.insert(codewordBlock.end(), errorBlock.begin(), errorBlock.end());
            
            return codewordBlock;
        }
        
        // If there are multiple codeword blocks, they are interleaved
        else if (dataCodewordBlocks.size() > 1) {

            // Blocks are interleaved by taking codewords from each block sequentially
            // e.g. b1[0],b2[0],b3[0],b1[1],b2[1],b3[1]...
            auto InterleaveCodewordBlocks = [](
                const std::vector<std::vector<std::byte>>& codewordBlocks,
                size_t totalCodewords
            ) {
                std::vector<std::byte> res(totalCodewords);
    
                int byteIdx = 0;
                for (int i{0}; i < totalCodewords; i+=codewordBlocks.size()) {
                    for (int blockIdx{0}; blockIdx < codewordBlocks.size(); blockIdx++) {
                        // When blocks are not the same size, skip groups that have expended
                        // all of their values and offset i so the remaining values will be placed
                        // in the proper index of res
                        while (byteIdx > codewordBlocks[blockIdx].size()-1) {
                            blockIdx++;
                            i--;
                        }
                        
                        res[i+blockIdx] = codewordBlocks[blockIdx][byteIdx];
                    }
                    byteIdx++;
                }

                return res;
            };


            auto interleavedCodewords = InterleaveCodewordBlocks(dataCodewordBlocks, totalDataCodewords);
            auto interleavedErrorCodewords = InterleaveCodewordBlocks(errorCodewordBlocks, totalErrorCodewords);

            // Append interleaved error codewords to interleaved data codewords
            interleavedCodewords.reserve(totalDataCodewords + totalErrorCodewords);

            interleavedCodewords.insert(interleavedCodewords.end(),
                interleavedErrorCodewords.begin(),
                interleavedErrorCodewords.end());

            return interleavedCodewords;
        }

        return {};
    }

    std::vector<std::byte> GenerateErrorCodewords(const std::vector<std::byte> &codewordBlock, int nErrorCodewords, Polynomial generatorPolynomial)
    {
        Polynomial messagePolynomial = CreateMessagePoly(codewordBlock);

        // Multiply message polynomial by x^n, where n is the number of error correction
        // codewords, to prevent the lead term from becoming too small during division
        messagePolynomial.MultiplyExponent(nErrorCodewords);

        // The lead term of the generator polynomial should also have the same
        // exponent as the prepared message
        int generatorLeadTermExp = generatorPolynomial.GetLeadTerm().exp;
        int messageLeadTermExp = messagePolynomial.GetLeadTerm().exp;
        generatorPolynomial.MultiplyExponent(messageLeadTermExp - generatorLeadTermExp);

        // Yield the error correction codewords by
        // dividing the message polynomial by the generator polynomial
        Polynomial prevPolynomial = messagePolynomial;
        Term prevLeadTerm = prevPolynomial.GetLeadTerm();
        Polynomial multResult;

        for (int i{0}; i < messagePolynomial.poly.size(); i++)
        {
            if (prevLeadTerm.coef == 0) {
                prevPolynomial.poly.erase(prevPolynomial.poly.begin());
                prevLeadTerm = prevPolynomial.GetLeadTerm();
                continue;
            }
            
            multResult = MultiplyTerm(generatorPolynomial, prevLeadTerm);
            generatorPolynomial.MultiplyExponent(-1);
            
            prevPolynomial = Xor(prevPolynomial, multResult);

            prevLeadTerm = prevPolynomial.GetLeadTerm();
        }
        
        std::vector<std::byte> res(prevPolynomial.poly.size());
        for (int i{0}; i < res.size(); i++) {
            res[i] = std::byte(prevPolynomial.poly[i].coef);
        }

        return res;
    }


    Polynomial CalcGeneratorPolynomial(int nCodewords)
    {
        assert(nCodewords >= 7);

        Polynomial currGenerator = { { {0,7}, {87,6}, {229,5}, {146,4}, {149,3}, {238,2}, {102,1}, {21,0} } };

        for (int generatorIdx{7}; generatorIdx < nCodewords; generatorIdx++)
        {
            currGenerator = CreateGeneratorPoly(currGenerator);
        }

        return currGenerator;
    }

    // Divide bs1 by bs2 until bs1 is desiredLength bits. Modifies values
    static void DivideBitStrings(std::deque<uint8_t>& bs1, std::deque<uint8_t>& bs2, int desiredLength)
    {
        while (bs1.size() > desiredLength)
        {            
            // Decrease the size of bs2 until it is the 
            // same size as the bs1
            while (bs2.size() > bs1.size()) {
                bs2.pop_back();
            }
            
            // XOR bs1 and bs2
            for (int i{0}; i < bs1.size(); i++) {
                bs1[i] ^= bs2[i];
            }

            // Remove any 0s from the left of the bs1
            while (bs1.front() == 0) { 
                bs1.pop_front();
            }
        }

        // If bs1 is shorter than the desired length, pad the left with 0s
        // until it is long enough
        while (bs1.size() < desiredLength) {
            bs1.push_front(0);
        }
    }

    std::vector<uint8_t> GenerateErrorCorrectedFormatStr(uint8_t formatString)
    {
        std::deque<uint8_t> generatorPolynomialBits{1,0,1,0,0,1,1,0,1,1,1};
        std::deque<uint8_t> ecBits(10, 0);

        // Insert the 5-bit format string the beginning of the error correction string
        for (int i{0}; i < 5; i++) {
            ecBits.push_front((formatString >> i) & 1);
        }

        // Remove any 0s from the left of the error correction string
        // The string must be at least 10 bits
        while ((ecBits.front() == 0) && ecBits.size() > 10) { 
            ecBits.pop_front();
        }

        // Pad generator with 0s until it is the same length as the error correction string
        while (generatorPolynomialBits.size() < ecBits.size()) {
            generatorPolynomialBits.push_back(0);
        }

        DivideBitStrings(ecBits, generatorPolynomialBits, 10);

        // Prepend the error correction string with the 5-bit format string
        for (int i{0}; i < 5; i++) {
            ecBits.push_front((formatString >> i) & 1);
        }

        // Mask format string
        std::vector<uint8_t> maskBits{1,0,1,0,1,0,0,0,0,0,1,0,0,1,0};

        for (int i{0}; i < 15; i++) {
            ecBits[i] ^= maskBits[i];
        }

        auto vec = std::vector<uint8_t>(15);
        for (int i{0}; i < 15; i++) {
            vec[i] = ecBits[i];
        }
        
        return vec;
    }


    std::vector<uint8_t> GenerateVersionInformationString(int version)
    {
        std::deque<uint8_t> generatorPolynomialBits{1,1,1,1,1,0,0,1,0,0,1,0,1};
        std::deque<uint8_t> versionStr(12, 0);

        // Add the 6-bit version number to the beginning of versionStr to
        // make an 18-bit bit string
        for (int i{0}; i < 6; i++) {
            versionStr.push_front((version >> i) & 1);
        }

        // Remove 0s from the left of versionStr
        // The version string must be at least 12 bits
        while (versionStr.front() == 0 && versionStr.size() > 12) {
            versionStr.pop_front();
        }

        // Add 0s until the generator string is the same length as the version string
        while (generatorPolynomialBits.size() < versionStr.size()) {
            generatorPolynomialBits.push_back(0);
        }

        DivideBitStrings(versionStr, generatorPolynomialBits, 12);

        // Prepend the version string with the original 6-bit version string
        for (int i{0}; i < 6; i++) {
            versionStr.push_front((version >> i) & 1);
        }

        auto vec = std::vector<uint8_t>(versionStr.size());
        for (int i{0}; i < versionStr.size(); i++) {
            vec[i] = versionStr[i];
        }

        return vec;
    }
}