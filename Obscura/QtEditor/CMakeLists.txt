qt_add_executable(ObscuraQtEditor
    Source/main.cpp
    Source/EditorApplication.hpp
    Source/EditorApplication.cpp
    Source/EngineViewport.hpp
    Source/EngineViewport.cpp
    Source/FrameBufferTransport.hpp
)

qt_add_qml_module(ObscuraQtEditor
    URI Obscura.Editor
    VERSION 1.0
    QML_FILES
        QML/Main.qml
        QML/EditorMenuBar.qml
        QML/SceneOutliner.qml
        QML/ViewportPanel.qml
        QML/ViewportToolbar.qml
        QML/Float3.qml
        QML/ContextualInspector.qml
        QML/Components/ComponentBase.qml
        QML/Components/IDComponent.qml
        QML/Components/TransformComponent.qml
        QML/Components/Sprite2DComponent.qml
)

target_include_directories(ObscuraQtEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(ObscuraQtEditor PRIVATE
    Obscura.Core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::QuickWidgets
    Vulkan::Vulkan
)

# Deployment via windeployqt
find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6 PATHS "${Qt6_DIR}/../../../bin" "$ENV{QT_SDK}/bin")

if (WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ObscuraQtEditor POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/QML" "$<TARGET_FILE:ObscuraQtEditor>"
        COMMENT "Deploying Qt dependencies and QML modules"
    )
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(ObscuraQtEditor PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appObscura
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
