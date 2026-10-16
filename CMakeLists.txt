cmake_minimum_required(VERSION 3.20)

project(GenCAD
    VERSION 4.0.0
    DESCRIPTION "GenCAD 2D CAD Desktop Application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Discover Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt6 OPTIONAL_COMPONENTS Test)

# Project include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Core CAD sources shared between application and tests
set(GENCAD_CORE_SOURCES
    # Geometry
    src/geometry/CadEntity.h
    src/geometry/CadEntity.cpp
    src/geometry/LineEntity.h
    src/geometry/LineEntity.cpp
    src/geometry/CircleEntity.h
    src/geometry/CircleEntity.cpp
    src/geometry/RectangleEntity.h
    src/geometry/RectangleEntity.cpp
    src/geometry/ArcEntity.h
    src/geometry/ArcEntity.cpp
    src/geometry/PolylineEntity.h
    src/geometry/PolylineEntity.cpp
    src/geometry/PolygonEntity.h
    src/geometry/PolygonEntity.cpp
    src/geometry/EllipseEntity.h
    src/geometry/EllipseEntity.cpp
    src/geometry/TextEntity.h
    src/geometry/TextEntity.cpp
    src/geometry/DimensionEntity.h
    src/geometry/DimensionEntity.cpp

    # Layers
    src/layers/CadLayer.h
    src/layers/CadLayer.cpp
    src/layers/LayerManager.h
    src/layers/LayerManager.cpp

    # Snap
    src/snap/SnapManager.h
    src/snap/SnapManager.cpp

    # Document
    src/document/UnitSystem.h
    src/document/UnitSystem.cpp
    src/document/PageSettings.h
    src/document/PageSettings.cpp
    src/document/CoordinateSystem.h
    src/document/GridSettings.h
    src/document/ConstraintEngine.h
    src/document/ConstraintEngine.cpp
    src/document/CadDocument.h
    src/document/CadDocument.cpp

    # Commands
    src/commands/Command.h
    src/commands/CreateEntityCommand.h
    src/commands/CreateEntityCommand.cpp
    src/commands/DeleteEntityCommand.h
    src/commands/DeleteEntityCommand.cpp
    src/commands/MoveEntityCommand.h
    src/commands/MoveEntityCommand.cpp
    src/commands/RotateEntityCommand.h
    src/commands/RotateEntityCommand.cpp
    src/commands/MirrorEntityCommand.h
    src/commands/MirrorEntityCommand.cpp
    src/commands/CopyEntityCommand.h
    src/commands/CopyEntityCommand.cpp
    src/commands/ModifyEntityPropertiesCommand.h
    src/commands/ModifyEntityPropertiesCommand.cpp

    # Canvas
    src/canvas/CadScene.h
    src/canvas/CadScene.cpp
    src/canvas/CadView.h
    src/canvas/CadView.cpp

    # Tools
    src/tools/Tool.h
    src/tools/SelectTool.h
    src/tools/SelectTool.cpp
    src/tools/LineTool.h
    src/tools/LineTool.cpp
    src/tools/CircleTool.h
    src/tools/CircleTool.cpp
    src/tools/RectangleTool.h
    src/tools/RectangleTool.cpp
    src/tools/ArcTool.h
    src/tools/ArcTool.cpp
    src/tools/PolylineTool.h
    src/tools/PolylineTool.cpp
    src/tools/PolygonTool.h
    src/tools/PolygonTool.cpp
    src/tools/EllipseTool.h
    src/tools/EllipseTool.cpp
    src/tools/TextTool.h
    src/tools/TextTool.cpp
    src/tools/DimensionTool.h
    src/tools/DimensionTool.cpp
    src/tools/MoveTool.h
    src/tools/MoveTool.cpp
    src/tools/RotateTool.h
    src/tools/RotateTool.cpp
    src/tools/MirrorTool.h
    src/tools/MirrorTool.cpp
    src/tools/CopyTool.h
    src/tools/CopyTool.cpp
    src/tools/OffsetTool.h
    src/tools/OffsetTool.cpp
    src/tools/TrimTool.h
    src/tools/TrimTool.cpp
    src/tools/ExtendTool.h
    src/tools/ExtendTool.cpp

    # File I/O
    src/io/GencadFile.h
    src/io/GencadFile.cpp
    src/io/DxfFile.h
    src/io/DxfFile.cpp
    src/io/DrawingExporter.h
    src/io/DrawingExporter.cpp

    # UI
    src/ui/CadTheme.h
    src/ui/CadTheme.cpp
    src/ui/CadRibbon.h
    src/ui/CadRibbon.cpp
    src/ui/CadCommandLine.h
    src/ui/CadCommandLine.cpp
    src/ui/PropertiesDock.h
    src/ui/PropertiesDock.cpp
    src/ui/PageSetupDialog.h
    src/ui/PageSetupDialog.cpp
    src/ui/LayerWidget.h
    src/ui/LayerWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/ToolBar.h
    src/ui/ToolBar.cpp
    src/ui/CloudWorkspaceDialog.h
    src/ui/CloudWorkspaceDialog.cpp
    src/ui/AiAssistantDock.h
    src/ui/AiAssistantDock.cpp

    # Cloud
    src/cloud/CloudClient.h
    src/cloud/CloudClient.cpp

    # AI (Phase 4)
    src/ai/AiDsl.h
    src/ai/AiDsl.cpp
    src/ai/AiValidator.h
    src/ai/AiValidator.cpp
    src/ai/AiCommandExecutor.h
    src/ai/AiCommandExecutor.cpp
    src/ai/AiGeometryAnalyzer.h
    src/ai/AiGeometryAnalyzer.cpp
    src/ai/AiDimensionGenerator.h
    src/ai/AiDimensionGenerator.cpp
    src/ai/AiRagEngine.h
    src/ai/AiRagEngine.cpp
    src/ai/AiLocalEngine.h
    src/ai/AiLocalEngine.cpp
    src/ai/AiClient.h
    src/ai/AiClient.cpp
    src/ai/AiFloorPlanSpec.h
    src/ai/AiFloorPlanSpec.cpp
    src/ai/AiFloorPlanGenerator.h
    src/ai/AiFloorPlanGenerator.cpp
    src/ai/AiFloorPlanProvider.h
    src/ai/AiFloorPlanProvider.cpp

    # Resources
    resources/resources.qrc
)

# Application source files
set(GENCAD_SOURCES
    src/main.cpp
    src/app/Application.h
    src/app/Application.cpp
    ${GENCAD_CORE_SOURCES}
)

# GenCAD executable
add_executable(gencad ${GENCAD_SOURCES})

target_link_libraries(gencad PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(gencad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler-specific flags
if(MSVC)
    target_compile_options(gencad PRIVATE /W4 /permissive- /Zc:__cplusplus)
endif()

# Tests
if(Qt6Test_FOUND)
    enable_testing()

    set(TEST_SOURCES
        tests/TestMain.cpp
        tests/GeometryTests.cpp
        tests/DocumentTests.cpp
        tests/CommandTests.cpp
        tests/FileIoTests.cpp
        tests/FunctionalE2ETest.cpp
        tests/Phase2Tests.cpp
        tests/Phase3Tests.cpp
        tests/Phase4Tests.cpp
        tests/CadDimensionWorkflowTests.cpp
        tests/AiFloorPlanTests.cpp
        ${GENCAD_CORE_SOURCES}
    )

    add_executable(gencad_tests ${TEST_SOURCES})

    target_link_libraries(gencad_tests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Test
    )

    target_include_directories(gencad_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(gencad_tests PRIVATE /W4 /permissive- /Zc:__cplusplus)
    endif()

    add_test(NAME GenCADUnitTests COMMAND gencad_tests)
endif()
