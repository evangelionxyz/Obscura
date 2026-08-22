file(GLOB_RECURSE SCRIPTING_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_library(Obscura.Scripting SHARED ${SCRIPTING_SOURCES})

target_include_directories(Obscura.Scripting PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_link_libraries(Obscura.Scripting PRIVATE
    Obscura.Core
)

target_compile_definitions(Obscura.Scripting PRIVATE OBSCURA_SCRIPTING_EXPORTS)
target_compile_features(Obscura.Scripting PUBLIC cxx_std_23)
