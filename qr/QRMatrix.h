#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "ErrorCorrection.h"

struct QRMatrixOptions
{
    int version;
    qr::EC_LEVEL ecLevel = qr::EC_LEVEL::M;
    std::shared_ptr<std::vector<std::byte>> pDataBits = nullptr;
    int scalingFac = 8;
    int mask = -1;
};

class QRMatrix
{
public:
    QRMatrix(const QRMatrixOptions& options);

    void Init();
    void WriteImagePPM(const char* filepath);
    void WriteImageJPEG(const char* filepath);
    void WriteImagePNG(const char* filepath);
    
private:
    void AddFinderPatterns();
    void AddAlignmentPatterns();
    void AddTimingPatterns();
    void AddDarkModule();
    void AddTempRegions();
    void AddDataBits();
    void SelectMask();
    void RestoreModuleColors();
    void AddQuietZone();
    
    void ScaleMatrix();
    std::vector<unsigned char> ConvertMatrixToU8();

    void ApplyMask0();
    void ApplyMask1();
    void ApplyMask2();
    void ApplyMask3();
    void ApplyMask4();
    void ApplyMask5();
    void ApplyMask6();
    void ApplyMask7();

    int EvaluateMask();
    int EvalCondition1(bool dir);
    int EvalCondition2();
    int EvalCondition3();
    int EvalCondition4();
    int TestPattern(int col, int row, bool direction);
    
    void InsertFormatString();
    void InsertVersionInformation();

    void InsertByte(std::byte byte);
    void GetNextPos();

    void DrawSquare(int x, int y, int width, uint32_t color);
    void DrawLine(int x, int y, int xStep, int yStep, int length, uint32_t color);
    void DrawModule(int x, int y, uint32_t color);
    void ToggleModule(int x, int y);

    uint32_t GetModule(int x, int y);
    void UnpackColor(const uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b);

private:
    int m_ScaledDimension;
    int m_OriginalDimension;
    int m_ModuleSize;
    int m_Version;
    int m_MaskPattern;
    qr::EC_LEVEL m_EcLevel;

    uint8_t m_ScalingFac = 1;
    
    std::vector<uint32_t> m_Matrix{};
    std::vector<uint32_t> m_TempMatrix{};
    std::shared_ptr<std::vector<std::byte>> m_DataBits = nullptr;

    struct DataInserter
    {
        uint8_t rowStep = -1;
        bool movingUp = true;
        bool currCol = 1;
        int posX, posY;
    } m_DataInserter;
};