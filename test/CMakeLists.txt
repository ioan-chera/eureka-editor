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
get_target_property(eureka_compile_options eurekasrc COMPILE_OPTIONS)
list(FILTER fltk_libs INCLUDE REGEX fltk)

# This is the common testing library. Contains whatever's in testUtils and some very common
# eurekasrc files

set(_testUtils
    testUtils/FatalHandler.cpp
    testUtils/FatalHandler.hpp
    testUtils/TempDirContext.cpp
    testUtils/TempDirContext.hpp
    ${src}/Errors.cc
    ${src}/m_strings.cc
)
add_library(testutils STATIC ${_testUtils})
target_link_libraries(testutils PUBLIC gtest_main)
if(WIN32)
    target_link_libraries(testutils PUBLIC Rpcrt4.lib)
endif()
target_include_directories(testutils PUBLIC ${src} ${src_includes})
target_compile_options(testutils PUBLIC ${eureka_compile_options})

#
# The function to add a unit test
#
function(unit_test _name)
    set(_name test_${_name})
    set(_use_src_files NO)
    set(_use_fltk NO)
    unset(_sources)
    unset(_windows_libs)
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

    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} PRIVATE testutils)

    if(_use_fltk)
        target_link_libraries(${_name} PRIVATE ${fltk_libs})
    endif()
    if(WIN32 AND _windows_libs)
        target_link_libraries(${_name} PRIVATE ${_windows_libs})
    endif()

    add_test(NAME ${_name} COMMAND $<TARGET_FILE:${_name}>)
endfunction()

# IMPORTANT: the eurekasrc files from testutils are already linked!

unit_test(lib_file
    lib_file_test.cpp
    SRC lib_file.cc
        lib_util.cc
    FLTK
)

unit_test(sys_debug
    sys_debug_test.cpp
    SRC m_streams.cc
        sys_debug.cc
)

# Units independent on complex frameworks or libraries
unit_test(independent
    lib_util_test.cpp
    m_bitvec_test.cpp
    m_parse_test.cpp
    m_streams_test.cpp
    SStringTest.cpp
    StringTableTest.cpp
    SRC lib_util.cc
        m_bitvec.cc
        m_parse.cc
        m_streams.cc
)

find_package(Python3)
if(NOT Python3_FOUND)
    message(WARNING "Python 3 not found, will not run system tests.")
else()
    add_test(NAME system_test_print_quit COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/python/test_print_quit.py
        --executable $<TARGET_FILE:eureka>
        --version ${PROJECT_VERSION}
    )
endif()
