set(API_TESTS_SRCS
    test_chip.cpp
    test_cluster_descriptor.cpp
    test_cluster.cpp
    test_soc_descriptor_gs.cpp
    test_soc_descriptor_wh.cpp
    test_soc_descriptor_bh.cpp
)

add_executable(api_tests ${API_TESTS_SRCS})
target_link_libraries(api_tests PRIVATE test_common)
set_target_properties(
    api_tests
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            ${CMAKE_BINARY_DIR}/test/umd/api
        OUTPUT_NAME
            api_tests
)
