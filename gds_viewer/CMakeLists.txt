find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
if(BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(GDS_VIEWER_SOURCES
    MainWindow.hpp
    MainWindow.cpp
    MainWindow.ui
    CoroutinePipeline.hpp
    PreparedSceneData.hpp
    GdsSceneView.hpp
    GdsSceneView.cpp
    ElementPropertiesDialog.hpp
    ElementPropertiesDialog.cpp
    HoverPolygonItem.hpp
    HoverPolygonItem.cpp
    HoverPathItem.hpp
    HoverPathItem.cpp
)

add_library(gds_viewer_lib
    ${GDS_VIEWER_SOURCES}
)

target_link_libraries(gds_viewer_lib
    PUBLIC
        Qt6::Widgets
        Qt6::Concurrent
        gds_core
)

target_include_directories(gds_viewer_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(gds_viewer_lib PUBLIC cxx_std_20)

add_executable(gds_viewer
    main.cpp
)

target_link_libraries(gds_viewer
    PRIVATE
        gds_viewer_lib
)

target_compile_features(gds_viewer PRIVATE cxx_std_20)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
