CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
PROJECT(OtoPi
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Oto lawn sprinkler driver for Raspberry Pi")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Setup GoogleTest
include(FetchContent)
set(BUILD_GMOCK OFF)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/cc366710bbf40a9816d47c35802d06dbaccb8792.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory("src")
