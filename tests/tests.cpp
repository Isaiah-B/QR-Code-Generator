#include <gtest/gtest.h>
#include <memory>

#include "Generator.h"
#include "helpers.h"

namespace qr
{
    class QRGeneratorFixture : public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_Generator = std::make_unique<QRGenerator>();
        }

        void TearDown() override
        {

        }

        std::unique_ptr<QRGenerator> m_Generator;
    };

    TEST_F(QRGeneratorFixture, DeterminesModeNumeric)
    {
        std::string input{"54319072318691"};
        
        MODE result = m_Generator->DetermineMode(input);
        EXPECT_EQ(result, MODE::NUMERIC);
    }

    TEST_F(QRGeneratorFixture, DeterminesModeAlphanumeric)
    {
        std::string input1{"HELLO CC WORLD"};
        std::string input2{"A6-L$ DH5R%:9+"};
        std::string input3{"G5*/0.a "};

        MODE result1 = m_Generator->DetermineMode(input1);
        MODE result2 = m_Generator->DetermineMode(input2);
        MODE result3 = m_Generator->DetermineMode(input3);

        EXPECT_EQ(result1, MODE::ALPHANUMERIC);
        EXPECT_EQ(result2, MODE::ALPHANUMERIC);
        EXPECT_NE(result3, MODE::ALPHANUMERIC);
    }

    // TEST_F(QRGeneratorFixture, EncodeNumeric)
    // {
    //     std::string input{"01234567"};
    //     std::string expectedStr{"00010000001000000000110001010110011000011000000011101100000100011110110000010001111011000001000111101100000100011110110000010001111011000001000111101100"};
    //     std::vector<uint8_t> expected = qr::BinaryStrToByteArr(expectedStr);

    //     m_Generator->Init(input);
    //     std::vector<std::byte> result = m_Generator->EncodeNumeric(input);

    //     EXPECT_EQ(result.size(), expected.size());
    //     EXPECT_EQ(result, expected);
    // }

    // TEST_F(QRGeneratorFixture, EncodeAlphanumeric)
    // {
    //     std::string input{"AC-42"};
    //     // std::string expected{"00100000001010011100111011100111001000010"};
    //     std::string expectedStr{"00100000001010011100111011100111001000010000000011101100000100011110110000010001111011000001000111101100000100011110110000010001111011000001000111101100"};
    //     std::vector<uint8_t> expected = qr::BinaryStrToByteArr(expectedStr);

    //     m_Generator->Init(input);

    //     std::vector<std::byte> result = m_Generator->EncodeAlphanumeric(input);

    //     EXPECT_EQ(result.size(), expected.size());
    //     EXPECT_EQ(result, expected);
    // }
}
