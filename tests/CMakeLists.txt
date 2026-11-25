add_executable(qrGeneratorTests)
target_sources(qrGeneratorTests PRIVATE tests.cpp)

find_package(GTest)

target_link_libraries(qrGeneratorTests PRIVATE Boost::program_options qrGenerator GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(qrGeneratorTests)