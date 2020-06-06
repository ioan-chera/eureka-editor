cmake_minimum_required(VERSION 3.13)

project(eureka-editor VERSION 2.0.0)

# Why not use the latest C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(executable_name eureka)

add_executable(${executable_name} MACOSX_BUNDLE)
if(APPLE)
    set_target_properties(${executable_name} PROPERTIES OUTPUT_NAME "Eureka Doom Editor")
endif()

if(APPLE)
    add_subdirectory(osx)
endif()
add_subdirectory(src)
