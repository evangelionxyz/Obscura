file(GLOB_RECURSE APP_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_executable(Obscura.App ${APP_SOURCES})

target_include_directories(Obscura.App PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_link_libraries(Obscura.App PRIVATE
    Obscura.Core
)

target_compile_features(Obscura.App PUBLIC cxx_std_23)
