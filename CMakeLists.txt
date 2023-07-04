CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(OtoPi
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Oto lawn sprinkler driver for Raspberry Pi")

FIND_PACKAGE(Threads REQUIRED)

set(COMMON_CXX_FLAGS
-Wall
-Wconversion
-Werror
-Wextra
-Winit-self
-Wno-psabi
-Wold-style-cast
-Woverloaded-virtual
-Wshadow
-Wsign-promo
-Wunreachable-code
-pedantic
-std=gnu++17
)

ADD_COMPILE_OPTIONS(
    "$<$<COMPILE_LANGUAGE:CXX>:${COMMON_CXX_FLAGS}>"
)

add_subdirectory("src")
