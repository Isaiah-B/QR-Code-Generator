cmake_minimum_required(VERSION 3.28)

project(qrGenerator)

set(CMAKE_CXX_STANDARD 20)

add_library(qrGenerator)

target_sources(qrGenerator PUBLIC
    Generator.cpp
    helpers.cpp
    ErrorCorrection.cpp
    Polynomial.cpp
    QRMatrix.cpp
    ParseArgs.cpp
    DataTables.cpp)
    
target_include_directories(qrGenerator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qrGenerator PRIVATE Boost::program_options)