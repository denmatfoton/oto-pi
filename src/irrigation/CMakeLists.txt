set(IRRIGATION_SRCS
	lib/Zone.cpp
)

add_library(irrigation STATIC ${IRRIGATION_SRCS})
target_include_directories(irrigation PUBLIC include)

# Tests
enable_testing()
set(TEST_TARGET irrigation_test)
file(GLOB TEST_SOURCES test/*.cpp)
add_executable(${TEST_TARGET} ${TEST_SOURCES})
target_include_directories(${TEST_TARGET} PUBLIC include)
target_link_libraries(${TEST_TARGET} irrigation gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
