add_executable(oto_pi)
add_executable(run_motor)

set(OTO_SRCS
	oto_pi.cpp
)

set(MOTOR_SRCS
	run_motor.cpp
)

set(HARDWARE_SRCS
	hardware/I2cAccessor.cpp
	hardware/MagnetSensor.cpp
	hardware/PressureSensor.cpp
	hardware/MotorControl.cpp
)

add_library(hardware STATIC ${HARDWARE_SRCS})
target_include_directories(hardware PRIVATE hardware utils)

set(COMMON_LIBS pthread i2c pigpio rt hardware)

target_sources(oto_pi PRIVATE ${OTO_SRCS})
target_include_directories(oto_pi PRIVATE hardware utils)
target_link_libraries(oto_pi PRIVATE ${COMMON_LIBS})

target_sources(run_motor PRIVATE ${MOTOR_SRCS})
target_include_directories(run_motor PRIVATE hardware utils)
target_link_libraries(run_motor PRIVATE ${COMMON_LIBS})
