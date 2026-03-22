enable_testing()

add_executable(gds_viewer_tests
    ViewerSmokeTests.cpp
)

target_link_libraries(gds_viewer_tests
    PRIVATE
        gds_viewer_lib
        Qt6::Test
        Qt6::Widgets
)

target_compile_features(gds_viewer_tests PRIVATE cxx_std_20)

add_test(NAME gds_viewer_tests COMMAND gds_viewer_tests)