#include "ParseArgs.h"

#include <iomanip>
#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace QRAppArgs
{
    const std::unordered_set<std::string> validExtensions{
        ".png", ".jpg", ".jpeg", ".ppm"
    };

    void AppArgs::Parse(int argc, char* argv[])
    {
        po::options_description desc("Options");
        desc.add_options()
            ("help","produce help mesage")
            ("ec-level,e", po::value<char>(), "Override error correction level (L, M, Q, or H).")
            ("scale,s", po::value<int>(), "Scale size of modules to n x n pixels. Default is 8.")
            ("mask,m", po::value<int>(), "Override mask pattern (0 - 7).")
            ("input-data", po::value<std::string>()->required())
            ("output-file", po::value<std::string>(), "Output extension must be .jpg, .jpeg, .png, or .ppm. Default output is qr.png.\n\n");

        po::positional_options_description positionalOpts;
        positionalOpts.add("input-data", 1);
        positionalOpts.add("output-file", 1);


        po::variables_map vm;
        try {
            po::store(po::command_line_parser(argc,argv)
                      .options(desc).positional(positionalOpts).run(), vm);
            po::notify(vm);
        

            if (vm.count("help")) {
                std::cout << desc << std::endl;
                exit(1);
            }

            if (vm.count("ec-level") ) {
                char opt = vm["ec-level"].as<char>();

                if (opt == 'l' || opt == 'L')
                    ecLevel = qr::EC_LEVEL::L;
                else if (opt == 'm' || opt == 'M')
                    ecLevel = qr::EC_LEVEL::M;
                else if (opt == 'q' || opt == 'Q')
                    ecLevel = qr::EC_LEVEL::Q;
                else if (opt == 'h' || opt == 'H')
                    ecLevel = qr::EC_LEVEL::H;
                else {
                    throw std::invalid_argument("Invalid error correction level");
                }
            }

            if (vm.count("scale")) {
                scalingFactor = vm["scale"].as<int>();
            }

            if (vm.count("mask")) {
                mask = vm["mask"].as<int>();
                if (mask < 0 || mask > 7) {
                    throw po::validation_error(po::validation_error::invalid_option_value, "mask");
                }
            }

            if (!vm.count("input-data") || !vm.count("output-file")) {
                std::cerr << desc << std::endl;
                exit(1);
            }

            if (vm.count("input-data")) {
                input = vm["input-data"].as<std::string>();
                if (input.size() < 1) {
                    throw po::validation_error(po::validation_error::invalid_option_value, "input-data");
                }
            }
            if (vm.count("output-file")) {
                output = vm["output-file"].as<std::string>();

                auto outputPath = std::filesystem::path{output};
                if (!outputPath.has_extension()) {
                    throw po::validation_error(po::validation_error::invalid_option_value, "output-file");
                }
                if (!validExtensions.contains(outputPath.extension().string())) {
                    throw po::validation_error(po::validation_error::invalid_option_value, "output-file");
                }

                outputExt = outputPath.extension().string();
            }
        }
        catch (po::error& e) {
            std::cerr << e.what() << std::endl;
            desc.print(std::cerr);
            exit(1);
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            desc.print(std::cerr);
            exit(1);
        }
    }
}