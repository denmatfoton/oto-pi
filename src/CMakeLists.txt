add_subdirectory("hardware")
add_subdirectory("irrigation")
add_subdirectory("utils")

set(COMMON_LIBS
 	hardware utils
)

# create_zone
add_executable(create_zone)
target_sources(create_zone PRIVATE create_zone.cpp)
target_link_libraries(create_zone
	${COMMON_LIBS}
	irrigation utils
	ncurses
)

# oto_pi
add_executable(oto_pi)
target_sources(oto_pi PRIVATE oto_pi.cpp)
target_link_libraries(oto_pi
	 ${COMMON_LIBS}
	irrigation utils
)

# run_motor
add_executable(run_motor)
target_sources(run_motor PRIVATE run_motor.cpp)
target_link_libraries(run_motor ${COMMON_LIBS})
