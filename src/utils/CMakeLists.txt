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
