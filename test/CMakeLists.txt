enable_testing()
find_package(GTest)
if (GTEST_FOUND)
    INCLUDE_DIRECTORIES(GTEST_INCLUDE_DIRS)
    ADD_EXECUTABLE(mytest_test mytest_test.cpp super_block_test.cpp)
    TARGET_LINK_LIBRARIES(mytest_test
                        ${GTEST_BOTH_LIBRARIES}
                        pthread
                        mytestlib)
    gtest_discover_tests(mytest_test)
endif()