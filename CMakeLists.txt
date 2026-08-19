cmake_minimum_required(VERSION 3.22.1)
project(zygisk-detach-v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

file(GLOB_RECURSE MODULE_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_library(zygisk-detach SHARED ${MODULE_SOURCES})

target_link_libraries(zygisk-detach log android)

target_compile_options(zygisk-detach PRIVATE
    -fvisibility=hidden
    -fno-exceptions
    -fno-rtti
    -Os
    -fomit-frame-pointer
)

set_target_properties(zygisk-detach PROPERTIES
    OUTPUT_NAME "zygisk-detach"
    PREFIX "lib"
)
