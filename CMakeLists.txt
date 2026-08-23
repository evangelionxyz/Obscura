cmake_minimum_required(VERSION 3.22)

project(Obscura VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/FS /Zc:__cplusplus)
endif()

# Qt Project Setup
# Enable Qt automoc, autouic, autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Resolve Qt SDK path before find_package so configure and IntelliSense can discover Qt headers.
if(DEFINED ENV{QT_SDK})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_SDK}")
elseif(DEFINED ENV{QTDIR})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
elseif(WIN32)
    # Auto-detect default Qt installations on Windows if no environment variable is set
    if(MSVC)
        file(GLOB _QT_CANDIDATES LIST_DIRECTORIES true "C:/Qt/*/msvc*_64")
    else()
        file(GLOB _QT_CANDIDATES LIST_DIRECTORIES true "C:/Qt/*/llvm-mingw_64" "C:/Qt/*/mingw*_64")
    endif()
    if(_QT_CANDIDATES)
        list(REVERSE _QT_CANDIDATES) # Pick highest version found
        list(GET _QT_CANDIDATES 0 _LATEST_QT)
        list(PREPEND CMAKE_PREFIX_PATH "${_LATEST_QT}")
        message(STATUS "Auto-detected Qt SDK: ${_LATEST_QT}")
    endif()
endif()

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

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty/nlohmann" "${CMAKE_CURRENT_SOURCE_DIR}/Thirdparty")

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
