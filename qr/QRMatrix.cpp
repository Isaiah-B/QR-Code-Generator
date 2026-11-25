#include "QRMatrix.h"

#include <iostream>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <array>
#include <functional>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define LIGHT 0xFFFFFFF
#define DARK  0X0

#define TEMP_LIGHT 0xFF00FF00
#define TEMP_DARK  0xFF0000FF

#define GREY 0xFF505050
#define RESERVED 0xFFFF0000
#define MASK 0xFFFF00FF

QRMatrix::QRMatrix(const QRMatrixOptions &options)
{
    m_Version       = options.version;
    m_EcLevel       = options.ecLevel;
    m_DataBits      = options.pDataBits;
    m_ScalingFac    = options.scalingFac;
    m_MaskPattern   = options.mask;

    assert(m_Version >= 1 && m_Version <= 40);

    m_OriginalDimension = 21 + (4 * (m_Version - 1));
    
    m_ModuleSize = m_ScalingFac;
    // Original dimension + 8 bits quiet zone * scaling
    m_ScaledDimension = (m_OriginalDimension + 8) * m_ScalingFac;

    Init();
}

void QRMatrix::Init()
{
    m_Matrix.resize(m_OriginalDimension*m_OriginalDimension);
    
    for (size_t i{0}; i < m_Matrix.size(); i++) {
        m_Matrix[i] = GREY;
    }

    AddFinderPatterns();
    AddTempRegions();
    AddAlignmentPatterns();
    AddTimingPatterns();
    AddDarkModule();
    AddDataBits();

    SelectMask();

    InsertFormatString();
    InsertVersionInformation();

    RestoreModuleColors();
    AddQuietZone();
}

void QRMatrix::WriteImagePPM(const char* filepath)
{
    ScaleMatrix();

    int dimension = m_ScaledDimension;

    std::ofstream outFile(filepath, std::ios::binary);
    outFile << "P3\n" << dimension << " " << dimension << "\n255\n";

    uint8_t r, g, b;
    for (size_t i{0}; i < m_Matrix.size(); i++) {
        UnpackColor(m_Matrix[i], r, g, b);
        outFile << static_cast<int>(r) << " " << static_cast<int>(g)  << " " << static_cast<int>(b) << '\n';
    }
}

std::vector<unsigned char> QRMatrix::ConvertMatrixToU8()
{
    ScaleMatrix();
    auto side = m_ScaledDimension * 3;

    std::vector<unsigned char> data(side*side);

    int dataIdx;
    unsigned char r, g, b;
    for (int i{0}; i < m_Matrix.size(); i++) {
        UnpackColor(m_Matrix[i], r, g, b);
        dataIdx = i * 3;
        data[dataIdx]   = r;
        data[dataIdx+1] = g;
        data[dataIdx+2] = b;
    }

    return data;
}

void QRMatrix::WriteImageJPEG(const char *filepath)
{
    int stride = m_ScaledDimension * 3;
    std::vector<unsigned char> imageData = ConvertMatrixToU8();

    stbi_write_jpg_strided(filepath, m_ScaledDimension, m_ScaledDimension, 3, imageData.data(), stride, 100);
}

void QRMatrix::WriteImagePNG(const char *filepath)
{
    int stride = m_ScaledDimension * 3;
    std::vector<unsigned char> imageData = ConvertMatrixToU8();

    stbi_write_png(filepath, m_ScaledDimension, m_ScaledDimension, 3, imageData.data(), stride);
}

void QRMatrix::AddFinderPatterns()
{
    // Top left
    DrawSquare(0, 0, 8, TEMP_LIGHT);
    DrawSquare(0, 0, 7, TEMP_DARK);
    DrawSquare(1, 1, 5, TEMP_LIGHT);
    DrawSquare(2, 2, 3, TEMP_DARK);

    // Top right
    uint8_t tlX = m_OriginalDimension - 8;
    DrawSquare(tlX, 0, 8, TEMP_LIGHT);
    DrawSquare(tlX+1, 0, 7, TEMP_DARK);
    DrawSquare(tlX+2, 1, 5, TEMP_LIGHT);
    DrawSquare(tlX+3, 2, 3, TEMP_DARK);

    // Bottom left
    uint8_t tlY = m_OriginalDimension - 8;
    DrawSquare(0, tlY, 8, TEMP_LIGHT);
    DrawSquare(0, tlY+1, 7, TEMP_DARK);
    DrawSquare(1, tlY+2, 5, TEMP_LIGHT);
    DrawSquare(2, tlY+3, 3, TEMP_DARK);
}

void QRMatrix::AddAlignmentPatterns()
{
    if (m_Version < 2) return;

    std::vector<std::vector<int>> ALIGNMENT_PATTERN_POSITIONS {
        {}, {}, {6,18}, {6,22},
        {6,26}, {6,30}, {6,34}, {6,22,38},
        {6,24,42}, {6,26,46}, {6,28,50}, {6,30,54},
        {6,32,58}, {6,34,62}, {6,26,46,66}, {6,26,48,70},
        {6,26,50,74}, {6,30,54,78}, {6,30,56,82}, {6,30,58,86},
        {6,34,62,90},
    };

    std::vector<int> positions = ALIGNMENT_PATTERN_POSITIONS[m_Version];
    for (int cx : positions) {
        for (int cy : positions) {    
            if (GetModule(cx, cy) == GREY) {
                DrawSquare(cx-2, cy-2, 5, TEMP_DARK);
                DrawSquare(cx-1, cy-1, 3, TEMP_LIGHT);
                DrawSquare(cx, cy, 1, TEMP_DARK);
            }
        }
    }
}

void QRMatrix::AddTimingPatterns()
{
    auto drawTimingPattern = [this](int hStep,int vStep,int startX, int startY)
    {
        int endPos = m_OriginalDimension - 8;
        int moduleCount = endPos - 8;
        int currX = startX, currY = startY;

        for (int i{0}; i < moduleCount - 1; i+= 2) {
            DrawModule(currX, currY, TEMP_DARK);
            DrawModule(currX+hStep, currY+vStep, TEMP_LIGHT);

            currX += hStep*2;
            currY += vStep*2;
        }

        // Place the last module (pattern length is odd)
        int endX = hStep ? endPos - 1 : startX;
        int endY = vStep ? endPos - 1 : startY;
        DrawModule(endX, endY, TEMP_DARK);
    };

    // Horizontal
    drawTimingPattern(1,0,8,6);
    // Vertical
    drawTimingPattern(0,1,6,8);
}

void QRMatrix::AddDarkModule()
{
    DrawModule(8, m_OriginalDimension - 8, TEMP_DARK);
}

void QRMatrix::AddTempRegions()
{
    // Reserve format information area
    
    // Top-left finder right side
    DrawLine(8, 0, 0, 1, 9, RESERVED);

    // Top-left finder bottom
    DrawLine(0, 8, 1, 0, 8, RESERVED);
    
    // Top-right finder bottom
    DrawLine(m_OriginalDimension-8, 8, 1, 0, 8, RESERVED);

    // Bottom-left finder right side
    DrawLine(8, m_OriginalDimension-8, 0, 1, 8, RESERVED);


    // Reserve version information regions (v7 and up)
    if (m_Version >= 7)
    {
        // Top-right finder left-side
        int trPosX = m_OriginalDimension - 11;
        int trPosY = 0;

        for (int row{trPosY}; row < 6; row++) {
            for (int col{trPosX}; col < trPosX + 3; col++) {
                DrawModule(col, row, RESERVED);
            }
        }

        // Bottom-left finder top
        trPosX = 0;
        trPosY = m_OriginalDimension - 11;
        for (int row{trPosY}; row < trPosY + 3; row++) {
            for (int col{trPosX}; col < 6; col++) {
                DrawModule(col, row, RESERVED);
            }
        }
    }
}

void QRMatrix::AddDataBits()
{
    m_DataInserter.posX = m_OriginalDimension-1;
    m_DataInserter.posY = m_OriginalDimension-1;

    for (const std::byte byte : *m_DataBits.get())
    {
        InsertByte(byte);
    }

    // Fill remainder bits with light modules if needed
    while (GetModule(m_DataInserter.posX, m_DataInserter.posY) == GREY) {
        InsertByte(std::byte(0));
    }
}

void QRMatrix::InsertByte(std::byte byte)
{
    uint32_t color;
    for (int8_t i{7}; i >= 0; i--) {
        color = LIGHT;
        if ((std::to_integer<uint8_t>(byte) >> i) & 0x1)
            color = DARK;

        DrawModule(m_DataInserter.posX, m_DataInserter.posY, color);

        GetNextPos();
    }
}

// void QRMatrix::GetNextPos()
// {
//     int nextX;
//     int nextY = m_DataInserter.posY + m_DataInserter.rowStep;

//     if (m_DataInserter.currCol == 1)
//         nextX = m_DataInserter.posX - 1;
//     else if (m_DataInserter.posX == 0)
//         nextX = m_DataInserter.posX + 1;

//     // Reach top or bottom
//     if (nextY == -1 || nextY == m_OriginalDimension) {
//         nextY = m_DataInserter.posY;
//         nextX = m_DataInserter.posX - 1;
//         m_DataInserter.rowStep = -m_DataInserter.rowStep;
//         return;
//     }

//     uint32_t nextModule = GetModule(nextX, nextY);
//     // Non-empty module
//     if (nextModule != GREY) {

//         // Light or dark module
//         if (nextModule == LIGHT || nextModule == DARK) {
//             GetNextPos();
//         }

//         // Reserved module
//         else if (nextModule == RESERVED) {
//             // Format strings on the bottom of finder patterns
//             if (nextY == 8) {
//                 nextY = m_DataInserter.posY;
//                 nextX = m_DataInserter.posX - 1;
//                 m_DataInserter.rowStep = -m_DataInserter.rowStep;
//             }

//             else if ()
//         }

//     }


//     // if (GetModule(nextX, nextY) != GREY) 

//     m_DataInserter.posX = nextX;
//     m_DataInserter.posY = nextY;
// }

void QRMatrix::GetNextPos()
{
    int nextX, nextY;

    if (m_DataInserter.movingUp) {
        if (m_DataInserter.currCol == 1) {
            // Move to the left
            nextX = m_DataInserter.posX - 1;
            nextY = m_DataInserter.posY;
            m_DataInserter.currCol = 0;
        }
        else {
            // Move to the top-right
            nextX = m_DataInserter.posX + 1;
            nextY = m_DataInserter.posY - 1;
            m_DataInserter.currCol = 1;
        }
        
        if (m_Version >= 7) {
            if ((nextX <= (m_OriginalDimension-9) && nextX >= (m_OriginalDimension-11)) && nextY == 6) {
                nextX = m_OriginalDimension - 12;
                nextY = 0;
                m_DataInserter.currCol = 0;
                m_DataInserter.movingUp = false;
            }
        }

        if (nextY < 0) {
            m_DataInserter.movingUp = false;
            nextX = m_DataInserter.posX - 1;
            nextY = m_DataInserter.posY;
            m_DataInserter.currCol = 1;
        }

        if (nextX < 0) {
            nextX = 0;
        }

        uint32_t nextColor = GetModule(nextX, nextY);

        if (nextColor == GREY) {
            m_DataInserter.posX = nextX;
            m_DataInserter.posY = nextY;
        }
        else if ((nextColor == RESERVED) && GetModule(nextX-1, nextY) == RESERVED) {
            m_DataInserter.movingUp = false;
            m_DataInserter.posX -= 1;
            m_DataInserter.currCol = 1;

            // Check for timing pattern to the left
            // and skip over it if it is there
            if (GetModule(m_DataInserter.posX, m_DataInserter.posY) != GREY) {
                m_DataInserter.posX -= 1;
            }
        }

        else {
            m_DataInserter.posX = nextX;
            m_DataInserter.posY = nextY;
            GetNextPos();
        }
    }
    else {
        // Moving down
        if (m_DataInserter.currCol == 1) {
            // Move to the left
            nextX = m_DataInserter.posX - 1;
            nextY = m_DataInserter.posY;
            m_DataInserter.currCol = 0;
        }
        else {
            // Move to the bottom-right
            nextX = m_DataInserter.posX + 1;
            nextY = m_DataInserter.posY + 1;
            m_DataInserter.currCol = 1;
        }

        if (nextY >= m_OriginalDimension) {
            m_DataInserter.movingUp = true;
            nextX = m_DataInserter.posX - 1;
            nextY = m_DataInserter.posY;
            m_DataInserter.currCol = 1;
        }

        if (m_Version >= 7)
        {
            if (nextX <= 5 && nextY == (m_OriginalDimension - 11))
            {
                nextX = m_DataInserter.posX - 1;
                nextY = m_DataInserter.posY;
                m_DataInserter.movingUp = true;
                m_DataInserter.currCol = 1;
            }
        }

        if (nextX < 0) {
            nextX = 0;
        }

        uint32_t nextColor = GetModule(nextX, nextY);

        if (nextColor == GREY) {
            m_DataInserter.posX = nextX;
            m_DataInserter.posY = nextY;
        }

        else {
            m_DataInserter.posX = nextX;
            m_DataInserter.posY = nextY;
            GetNextPos();
        }
    }
}

void QRMatrix::RestoreModuleColors()
{
    for (int i{0}; i < m_Matrix.size(); i++) {
        if (m_Matrix[i] == TEMP_LIGHT)
            m_Matrix[i] = LIGHT;
        else if (m_Matrix[i] == TEMP_DARK)
            m_Matrix[i] = DARK;
    }
}

void QRMatrix::AddQuietZone()
{
    int newDimension = m_OriginalDimension + 8;

    std::vector<uint32_t> fullMatrix(newDimension*newDimension, LIGHT);

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            fullMatrix[(row+4)*newDimension + (col+4)] = GetModule(col, row);
        }
    }

    m_Matrix = std::move(fullMatrix);
}

void QRMatrix::DrawSquare(int x, int y, int width, uint32_t color)
{
    for (int row{y}; row < y + width; row++) {
        for (int col{x}; col < x + width; col++){ 
            DrawModule(col, row, color);
        }
    }
}

void QRMatrix::DrawLine(int x, int y, int xStep, int yStep, int length, uint32_t color)
{
    for (int i{0}; i < length; i++) {
        DrawModule(x, y, color);
        x += xStep;
        y += yStep;
    }
}

void QRMatrix::DrawModule(int x, int y, uint32_t color)
{
    m_Matrix[y*m_OriginalDimension + x] = color;
}

void QRMatrix::ToggleModule(int x, int y)
{
    uint32_t module = GetModule(x, y);

    if (module == LIGHT) {
        DrawModule(x, y, DARK);
    }
    else if (module == DARK) {
        DrawModule(x, y, LIGHT);
    }
}

uint32_t QRMatrix::GetModule(int x, int y)
{
    return m_Matrix[y*m_OriginalDimension + x];
}

void QRMatrix::UnpackColor(const uint32_t color, uint8_t &r, uint8_t &g, uint8_t &b)
{
    r = color & 0xFF;
    g = (color>>8) & 0xFF;
    b = (color>>16) & 0xFF;
}

void QRMatrix::SelectMask()
{
    std::array<std::function<void(QRMatrix*)>, 8> maskFunctions{
        &QRMatrix::ApplyMask0,
        &QRMatrix::ApplyMask1,
        &QRMatrix::ApplyMask2,
        &QRMatrix::ApplyMask3,
        &QRMatrix::ApplyMask4,
        &QRMatrix::ApplyMask5,
        &QRMatrix::ApplyMask6,
        &QRMatrix::ApplyMask7
    };

    std::array<int, 8> penalties{};

    // Evaluate each mask function
    for (size_t i{0}; i < maskFunctions.size(); i++) {
        maskFunctions[i](this);
        penalties[i] = EvaluateMask();
        m_Matrix = m_TempMatrix;
    }

    // Find the mask with the lowest evaluation score 
    int lowestScoreIdx = 0, min = penalties[0];
    for (int i{1}; i < penalties.size(); i++) {
        if (penalties[i] < min) {
            min = penalties[i];
            lowestScoreIdx = i;
        }
    }

    // Use the mask pattern with the best score unless
    // the mask pattern was overrided in the options
    m_MaskPattern = m_MaskPattern == -1
        ? lowestScoreIdx
        : m_MaskPattern;
    std::cout << "Best mask score: " << lowestScoreIdx << " " << penalties[lowestScoreIdx] << std::endl;
    std::cout << "Using mask: " << m_MaskPattern << std::endl;

    // Apply selected mask function
    maskFunctions[m_MaskPattern](this);
}

void QRMatrix::ApplyMask0()
{
    m_TempMatrix = m_Matrix;
    
    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if ((row + col) % 2 == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

void QRMatrix::ApplyMask1()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row+=2) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            ToggleModule(col, row);
        }
    }
}

void QRMatrix::ApplyMask2()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col+=3) {
            ToggleModule(col, row);
        }
    }
}

void QRMatrix::ApplyMask3()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if ((row + col) % 3 == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

void QRMatrix::ApplyMask4()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if (((row/2) + (col/3)) % 2 == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

void QRMatrix::ApplyMask5()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if (((row*col)%2) + ((row*col)%3) == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

void QRMatrix::ApplyMask6()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if ((((row*col)%2) + ((row*col)%3)) % 2 == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

void QRMatrix::ApplyMask7()
{
    m_TempMatrix = m_Matrix;

    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if ((((row+col)%2) + ((row*col)%3)) % 2 == 0) {
                ToggleModule(col, row);
            }
        }
    }
}

int QRMatrix::EvaluateMask()
{
    // Convert temporary light and dark colors to regular light and dark
    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            uint32_t color = GetModule(col, row);

            if (color == TEMP_LIGHT) {
                DrawModule(col, row, LIGHT);
            }
            else if (color == TEMP_DARK) {
                DrawModule(col, row, DARK);
            }
            else if (color == RESERVED) {
                DrawModule(col, row, LIGHT);
            }
        }
    }
    

    // Condition 1:
    // Check each row, adding a penalty of 3 if there are five
    // consecutive modules of the same color. Add 1 penalty for
    // each following consecutive module. Do the same for each column
    int penalty1 = EvalCondition1(0);
    penalty1 += EvalCondition1(1);
    
    // Condition 2:
    // Check for 2x2 blocks of the same module, adding a penalty of 3
    // for each one
    // Each block is counted, even if it overlaps another 2x2 block
    int penalty2 = EvalCondition2();
    
    // Condition 3:
    // Check rows and columns for two patterns:
    // - DLDDDLDLLLL
    // - LLLLDLDDDLD
    // Add penalty of 40 for each occurance 
    int penalty3 = EvalCondition3();
    
    // Condition 4:
    // Calculate penalty based on the ratio of light modules to dark modules
    int penalty4 = EvalCondition4();
    
    int total = penalty1 + penalty2 + penalty3 + penalty4;
    
    return total;
}


static int CheckPenalty(int nConsecutive)
{
    if (nConsecutive >= 5) {
        return 3 + (nConsecutive - 5);
    }

    return 0;
}

// 0 for row-wise; 1 for column-wise
// Returns calculated penalty
int QRMatrix::EvalCondition1(bool dir)
{
    int penalty = 0;

    // last: 0 = dark, 1 = light
    bool prevModule = 0;
    int nConsecutive = 0;
    uint32_t currModule;

    int row=0, col=0;
    int* x, *y;
    if (!dir) {
        x = &col;
        y = &row;
    }
    else {
        x = &row;
        y = &col;
    }

    for (row = 0; row < m_OriginalDimension; row++) {
        for (col = 0; col < m_OriginalDimension; col++) {
            currModule = GetModule(*x, *y);

            if (currModule == DARK) {
                if (prevModule == 1) {
                    penalty += CheckPenalty(nConsecutive);
                    nConsecutive = 0;
                }

                prevModule = 0;
                nConsecutive++;
            }
            else if (currModule == LIGHT) {
                if (prevModule == 0) {
                    penalty += CheckPenalty(nConsecutive);
                    nConsecutive = 0;
                }
                prevModule = 1;
                nConsecutive++;
            }
        }

        // Check penalty at the end of the row and reset for the next row
        penalty += CheckPenalty(nConsecutive);
        nConsecutive = 0;
    }

    return penalty;
}

int QRMatrix::EvalCondition2()
{
    int penalty = 0;

    for (int row{0}; row < m_OriginalDimension-1; row++) {
        for (int col{0}; col < m_OriginalDimension-1; col++) {
            uint32_t c1 = GetModule(col, row);
            if (GetModule(col+1, row) != c1) continue;
            if (GetModule(col, row+1) != c1) continue;
            if (GetModule(col+1, row+1) != c1) continue;

            penalty += 3;
        }
    }

    return penalty;
}

std::vector<uint32_t> p1{LIGHT,DARK,DARK,DARK,LIGHT,DARK,LIGHT,LIGHT,LIGHT,LIGHT};
std::vector<uint32_t> p2{LIGHT,LIGHT,LIGHT,LIGHT,DARK,LIGHT,DARK,DARK,DARK,LIGHT};

// direction: 0 = column-wise; 1 = row-wise
int QRMatrix::TestPattern(int col, int row, bool direction)
{
    bool match = true;
    int c = col, r = row;

    auto CheckForPattern = [&](const std::vector<uint32_t> pattern) {
        for (int i{0}; i < pattern.size(); i++) {
            if (direction == 0) {
                c = col + i;
            } else {
                r = row + i;
            }

            if (GetModule(c, r) != pattern[i]) {
                match = false;
                break;
            }
        }

        if (match)
            return 40;

        return 0;
    };

    if (GetModule(col, row) == DARK) {
        return CheckForPattern(p1);
    }
    else if (GetModule(col, row) == LIGHT) {
        return CheckForPattern(p2);
    }

    return 0;
}

int QRMatrix::EvalCondition3()
{
    int penalty = 0;

    // Search horizontally
    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension-10; col++) {
            penalty += TestPattern(col, row, 0);
        }
    }
    
    // Search vertically
    for (int col{0}; col < m_OriginalDimension; col++) {
        for (int row{0}; row < m_OriginalDimension-10; row++) {
            penalty += TestPattern(col, row, 1);
        }
    }

    return penalty;
}

int QRMatrix::EvalCondition4()
{
    int totalModules = m_OriginalDimension*m_OriginalDimension;
    
    int nDarkModules = 0;
    for (int row{0}; row < m_OriginalDimension; row++) {
        for (int col{0}; col < m_OriginalDimension; col++) {
            if (GetModule(col, row) == DARK)
                nDarkModules++;
        }
    }

    int darkRatio = static_cast<int>((nDarkModules / static_cast<float>(totalModules)) * 100.0f);

    int nextMultipleOfFive = darkRatio + 4;
    nextMultipleOfFive -= (nextMultipleOfFive % 5);

    int prevMultipleOfFive = darkRatio - 5;
    prevMultipleOfFive = darkRatio - (prevMultipleOfFive % 5);

    int v1 = abs(nextMultipleOfFive - 50) / 5;
    int v2 = abs(prevMultipleOfFive - 50) / 5;

    return std::min(v1, v2) * 10;
}


void QRMatrix::InsertFormatString()
{
    uint8_t ecBits = 1;
    switch (m_EcLevel) {
        case qr::EC_LEVEL::L: ecBits = 1; break;
        case qr::EC_LEVEL::M: ecBits = 0; break;
        case qr::EC_LEVEL::Q: ecBits = 3; break;
        case qr::EC_LEVEL::H: ecBits = 2; break;
    }

    uint8_t formatString = (ecBits << 3) | (m_MaskPattern);

    std::vector<uint8_t> fullFormatString = qrEc::GenerateErrorCorrectedFormatStr(formatString);

    auto placeRight = [&](int x, int y, int idxStart, int n) {
        for (int i{0}; i < n; i++) {
            if (fullFormatString[idxStart+i] == 0)
                DrawModule(x+i, y, LIGHT);
            else
                DrawModule(x+i, y, DARK);
        }
    };

    auto placeUp = [&](int x, int y, int idxStart, int n) {
        for (int i{0}; i < n; i++) {
            if (fullFormatString[idxStart+i] == 0)
                DrawModule(x, y-i, LIGHT);
            else
                DrawModule(x, y-i, DARK);
        }
    };

    placeRight(0, 8, 0, 6);
    placeRight(7, 8, 6, 2);

    placeRight(m_OriginalDimension-8, 8, 7, 8);

    placeUp(8, 8, 7, 2);
    placeUp(8, 5, 9, 6);
    placeUp(8, m_OriginalDimension-1, 0, 7);
}

void QRMatrix::InsertVersionInformation()
{
    if (m_Version < 7)
        return;

    std::vector<uint8_t> versionString = qrEc::GenerateVersionInformationString(m_Version);


    // Bottom block
    int bitIdx = versionString.size() - 1;
    int rowStart = m_OriginalDimension-11;

    for (int col{0}; col < 6; col++) {
        for (int row{rowStart}; row < rowStart+3; row++) {
            uint8_t bit = versionString[bitIdx--];
            if (bit == 0)
                DrawModule(col, row, LIGHT);
            else
                DrawModule(col, row, DARK);       
        }
    }

    // Top block
    bitIdx = versionString.size() - 1;
    int colStart = m_OriginalDimension-11;

    for (int row{0}; row < 6; row++) {
        for (int col{colStart}; col < colStart+3; col++) {
            uint8_t bit = versionString[bitIdx--];
            if (bit == 0)
                DrawModule(col, row, LIGHT);
            else
                DrawModule(col, row, DARK);       
        }
    }
}

void QRMatrix::ScaleMatrix()
{
    std::vector<uint32_t> scaledMatrix(m_ScaledDimension*m_ScaledDimension);

    for (int row{0}; row < (m_OriginalDimension + 8); row++) {
        for (int col{0}; col < (m_OriginalDimension + 8); col++) {
            // Draw scaled module
            uint32_t color = m_Matrix[(row*(m_OriginalDimension+8)) + col];
            for (int h{row*m_ModuleSize}; h < (row*m_ModuleSize)+m_ModuleSize; h++) {
                for (int w{col*m_ModuleSize}; w < (col*m_ModuleSize)+m_ModuleSize; w++) {
                    scaledMatrix[(h*m_ScaledDimension) + w] = color;
                }
            }
        }
    }

    m_Matrix = std::move(scaledMatrix);
}
