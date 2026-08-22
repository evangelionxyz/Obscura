file(GLOB_RECURSE CORE_SOURCES
    "Include/*.hpp"
    "Include/*.h"
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.h"
)

add_library(Obscura.Core STATIC ${CORE_SOURCES})

target_include_directories(Obscura.Core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_link_libraries(Obscura.Core PUBLIC spdlog::spdlog glm::glm EnTT::EnTT cppcoro)

target_compile_features(Obscura.Core PUBLIC cxx_std_23)
