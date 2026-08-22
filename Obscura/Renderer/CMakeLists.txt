file(GLOB_RECURSE RENDERER_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_library(Obscura.Renderer SHARED ${RENDERER_SOURCES})

target_include_directories(Obscura.Renderer
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Obscura.Renderer
    PRIVATE
        Obscura.Core
        Vulkan::Vulkan
)

target_compile_definitions(Obscura.Renderer PRIVATE OBSCURA_RENDERER_EXPORTS)
target_compile_features(Obscura.Renderer PUBLIC cxx_std_23)
