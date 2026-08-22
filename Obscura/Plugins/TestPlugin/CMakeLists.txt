file(GLOB_RECURSE PLUGIN_SOURCES
    "Source/*.cpp"
    "Source/*.hpp"
    "Source/*.c"
    "Source/*.h"
)

add_library(Obscura.TestPlugin SHARED ${PLUGIN_SOURCES})

target_include_directories(Obscura.TestPlugin PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_link_libraries(Obscura.TestPlugin PRIVATE
    Obscura.Core
)

target_compile_definitions(Obscura.TestPlugin PRIVATE OBSCURA_PLUGIN_EXPORTS)
target_compile_features(Obscura.TestPlugin PUBLIC cxx_std_23)

set_target_properties(Obscura.TestPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Plugins"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(Obscura.TestPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/Plugins"
        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}/Plugins"
    )
endforeach()
