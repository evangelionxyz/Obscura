cmake_minimum_required(VERSION 3.22)

project(Obscura VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/FS)
endif()

# Qt Project Setup
# Enable Qt automoc, autouic, autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Widgets
    QuickWidgets
)

qt_standard_project_setup(REQUIRES 6.5)

# Dynamically resolve Qt from QT_SDK environment variable
if(DEFINED ENV{QT_SDK})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_SDK}")
endif()

# Vulkan SDK detection
find_package(Vulkan REQUIRED)

# Unified output directories for all executables and libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}")
endforeach()

# Add Thirdparty dependencies
add_subdirectory(Thirdparty/spdlog)
add_subdirectory(Thirdparty/ShaderCompiler)
add_subdirectory(Thirdparty/glm)
add_subdirectory(Thirdparty/entt)
add_subdirectory(Thirdparty/cppcoro)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/stb")

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(Thirdparty/GoogleTest)

# Add Subdirectories
add_subdirectory(Obscura/Core)
add_subdirectory(Obscura/Renderer)
add_subdirectory(Obscura/Scripting)
add_subdirectory(Obscura/Engine)
add_subdirectory(Obscura/Plugins/TestPlugin)
add_subdirectory(Obscura/App)
add_subdirectory(Obscura/Tests)
add_subdirectory(Obscura/QtEditor)

# Deploy Resources incrementally (copies only if modified)
add_custom_target(DeployResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Obscura/Resources"
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
    COMMENT "Deploying Obscura Resources incrementally to binary directory"
)
