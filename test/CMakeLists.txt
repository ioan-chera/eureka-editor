option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

if(ENABLE_UNIT_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
    )
    FetchContent_GetProperties(googletest)
    if(NOT googletest_POPULATED)
        FetchContent_Populate(googletest)
        add_subdirectory(
            ${googletest_SOURCE_DIR}
            ${googletest_BINARY_DIR}
        )
    endif()

    set(src ${CMAKE_SOURCE_DIR}/src)
    get_target_property(src_includes eurekasrc INCLUDE_DIRECTORIES)

    enable_testing()

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
        foreach(source ${ARGN})
            if(source STREQUAL "SRC")
                set(_use_src_files YES)
                continue()
            endif()
            if(NOT _use_src_files)
                list(APPEND _sources ${source})
            else()
                list(APPEND _sources ${src}/${source})
            endif()
        endforeach()

        add_executable(${_name} ${_sources} ${_testUtils})
        target_link_libraries(${_name} PRIVATE gtest_main)
        target_include_directories(${_name} PRIVATE ${src} ${src_includes})
        if(MSVC)
            target_compile_options(${_name} PUBLIC "/MTd")  # needed for linking to gtest
        endif()

        add_test(NAME ${_name} COMMAND $<TARGET_FILE:${_name}>)
    endfunction()

    unit_test(lib_file lib_file_test.cpp SRC lib_file.cc)
    unit_test(SString SStringTest.cpp SRC Errors.cc m_strings.cc)

endif()
