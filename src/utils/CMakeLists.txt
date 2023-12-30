set(UTILS_SRCS
	lib/Logger.cpp
	lib/MathUtils.cpp
	lib/Timer.cpp
)

add_library(utils STATIC ${UTILS_SRCS})
target_include_directories(utils PUBLIC include)
