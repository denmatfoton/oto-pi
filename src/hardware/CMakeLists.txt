set(HARDWARE_SRCS
	lib/I2cAccessor.cpp
	lib/MagnetSensor.cpp
	lib/MotorControl.cpp
	lib/PressureSensor.cpp
    lib/NozzleControl.cpp
)

add_library(hardware STATIC ${HARDWARE_SRCS})
target_include_directories(hardware PUBLIC include ../utils/include)
target_link_libraries(hardware i2c pigpio pthread)

