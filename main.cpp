#include <iostream>

#include "Generator.h"
#include "ErrorCorrection.h"
#include "QRMatrix.h"
#include "ParseArgs.h"

static char PrintECLevel(qr::EC_LEVEL ecLevel);

int main(int argc, char* argv[])
{
    QRAppArgs::AppArgs appArgs;
    appArgs.Parse(argc, argv);

    qr::QRGenerator generator(appArgs.ecLevel, appArgs.version);
    generator.Init(appArgs.input);

    std::cout << "Error correction level: " << PrintECLevel(generator.GetECLevel()) << std::endl;
    std::cout << "Version: " << generator.GetVersion() << std::endl;

    std::vector<std::byte> codewords = generator.Encode(appArgs.input);

    std::vector<std::byte> finalMessage = qrEc::ConstructFinalMessage(codewords, generator.GetVersion(), generator.GetECLevel());

    auto finalMessagePtr = std::make_shared<std::vector<std::byte>>(finalMessage);

    QRMatrixOptions options {
        .version = generator.GetVersion(),
        .ecLevel = generator.GetECLevel(),
        .pDataBits = finalMessagePtr,
        .scalingFac = appArgs.scalingFactor,
        .mask = appArgs.mask
    };

    QRMatrix matrix(options);
    
    
    const char* outputFile = appArgs.output.c_str();

    if (appArgs.outputExt == ".png") {
        matrix.WriteImagePNG(outputFile);
    }
    else if (appArgs.outputExt == ".jpg" || appArgs.outputExt == ".jpeg") {
        matrix.WriteImageJPEG(outputFile);
    }
    else if (appArgs.outputExt == ".ppm") {
        matrix.WriteImagePPM(outputFile);
        std::string call = "feh " + std::string(outputFile);
        system(call.c_str());
    }

    return 0;
}

static char PrintECLevel(qr::EC_LEVEL ecLevel)
{
    switch (ecLevel) {
        case qr::EC_LEVEL::L: return 'L';
        case qr::EC_LEVEL::M: return 'M';
        case qr::EC_LEVEL::Q: return 'Q';
        case qr::EC_LEVEL::H: return 'H';
    }
    
    return -1;
}