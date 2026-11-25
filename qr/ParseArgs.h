#pragma once

#include <iostream>
#include <stdexcept>
#include <unordered_set>

#include "ErrorCorrection.h"

namespace QRAppArgs
{
    extern const std::unordered_set<std::string> validExtensions;

    struct AppArgs
    {
        AppArgs() {};
    
        void Parse(int argc, char* argv[]);
    
        qr::EC_LEVEL ecLevel{qr::EC_LEVEL::M};
        int version{-1};
        int scalingFactor{8};
        int mask{-1};
        std::string input{};
        std::string output{"qr.png"};
        std::string outputExt{".png"};
    };
}