set(UTILS_SRCS
	lib/ConfigManager.cpp
	lib/Logger.cpp
	lib/MathUtils.cpp
	lib/MqttWrapper.cpp
	lib/PolarCoordinates.cpp
	lib/Timer.cpp
)

add_library(utils STATIC ${UTILS_SRCS})
target_include_directories(utils PUBLIC	include)
target_link_libraries(utils paho-mqtt3c)

# Tests
enable_testing()
set(TEST_TARGET utils_test)
file(GLOB TEST_SOURCES test/*.cpp)
add_executable(${TEST_TARGET} ${TEST_SOURCES})
target_include_directories(${TEST_TARGET} PUBLIC include)
target_link_libraries(${TEST_TARGET} utils gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
