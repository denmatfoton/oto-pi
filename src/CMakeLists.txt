add_executable(oto_pi)
add_executable(run_motor)

set(OTO_SRCS
	oto_pi.cpp
	hardware/PressureSensor.cpp
	hardware/MagnetSensor.cpp
)

set(MOTOR_SRCS
	run_motor.cpp
	hardware/MotorControl.cpp
)

set(COMMON_LIBS pthread i2c pigpio rt)

target_sources(oto_pi PRIVATE ${OTO_SRCS})
target_include_directories(oto_pi PRIVATE hardware)
target_link_libraries(oto_pi PRIVATE ${COMMON_LIBS})

target_sources(run_motor PRIVATE ${MOTOR_SRCS})
target_include_directories(run_motor PRIVATE hardware)
target_link_libraries(run_motor PRIVATE ${COMMON_LIBS})
