file(GLOB_RECURSE ENGINE_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_library(Obscura.Engine SHARED ${ENGINE_SOURCES})

target_include_directories(Obscura.Engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_link_libraries(Obscura.Engine PRIVATE
    Obscura.Core
    Obscura.Renderer
    EnTT::EnTT
    stb
    nlohmann_json
)

target_compile_definitions(Obscura.Engine PRIVATE OBSCURA_ENGINE_EXPORTS)
target_compile_features(Obscura.Engine PUBLIC cxx_std_23)
