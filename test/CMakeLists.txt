message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    message(STATUS "Populating Google Test directory")
    FetchContent_Populate(googletest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(
        ${googletest_SOURCE_DIR}
        ${googletest_BINARY_DIR}
    )
endif()

set(src ${CMAKE_SOURCE_DIR}/src)
get_target_property(src_includes eurekasrc INCLUDE_DIRECTORIES)
get_target_property(fltk_libs eurekasrc LINK_LIBRARIES)
list(FILTER fltk_libs INCLUDE REGEX fltk)

set(_testUtils
    testUtils/FatalHandler.cpp
    testUtils/FatalHandler.hpp
    testUtils/TempDirContext.cpp
    testUtils/TempDirContext.hpp
)

#
# The function to add a unit test
#
function(unit_test _name)
    set(_name test${_name})
    set(_use_src_files NO)
    set(_use_fltk NO)
    unset(_sources)
    set(_windows_libs Rpcrt4.lib)
    foreach(_source ${ARGN})
        if(_source STREQUAL "SRC")
            set(_use_src_files YES)
            continue()
        endif()
        if(_source STREQUAL "FLTK")
            set(_use_fltk YES)
            continue()
        endif()
        if(_source MATCHES ".*\\.lib")
            list(APPEND _windows_libs ${_source})
            continue()
        endif()
        if(NOT _use_src_files)
            list(APPEND _sources ${_source})
        else()
            list(APPEND _sources ${src}/${_source})
        endif()
    endforeach()

    add_executable(${_name} ${_sources} ${_testUtils})
    target_link_libraries(${_name} PRIVATE gtest_main)
    target_include_directories(${_name} PRIVATE ${src} ${src_includes})
    if(_use_fltk)
        target_link_libraries(${_name} PRIVATE ${fltk_libs})
    endif()
    if(WIN32 AND _windows_libs)
        target_link_libraries(${_name} PRIVATE ${_windows_libs})
    endif()

    add_test(NAME ${_name} COMMAND $<TARGET_FILE:${_name}>)
endfunction()

unit_test(lib_file
    lib_file_test.cpp
    SRC Errors.cc
        lib_file.cc
        lib_util.cc
        m_strings.cc
    FLTK
)
unit_test(SString
    SStringTest.cpp
    SRC Errors.cc
        m_strings.cc
)
