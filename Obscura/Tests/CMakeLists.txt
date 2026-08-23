file(GLOB_RECURSE TEST_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_executable(Obscura.Tests ${TEST_SOURCES})

target_include_directories(Obscura.Tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_link_libraries(Obscura.Tests PRIVATE
    Obscura.Core
    Obscura.Renderer
    Obscura.Engine
    GTest::gtest
    nlohmann_json
)

target_compile_features(Obscura.Tests PUBLIC cxx_std_23)
