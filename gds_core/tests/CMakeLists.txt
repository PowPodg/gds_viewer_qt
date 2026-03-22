enable_testing()

set(GDS_CORE_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${GDS_CORE_TEST_DATA_DIR}")
configure_file(data/cross.gds "${GDS_CORE_TEST_DATA_DIR}/cross.gds" COPYONLY)

add_executable(gds_core_tests ReaderTests.cpp)
target_link_libraries(gds_core_tests PRIVATE gds_core)
target_compile_features(gds_core_tests PRIVATE cxx_std_20)
target_compile_definitions(gds_core_tests PRIVATE GDS_CORE_TEST_DATA_DIR="${GDS_CORE_TEST_DATA_DIR}")

add_test(NAME gds_core_tests COMMAND gds_core_tests)
