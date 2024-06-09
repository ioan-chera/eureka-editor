message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

# Following https://github.com/google/googletest/blob/main/googletest/README.md
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


set(src ${CMAKE_SOURCE_DIR}/src)
get_target_property(src_includes eurekasrc INCLUDE_DIRECTORIES)
get_target_property(fltk_libs eurekasrc LINK_LIBRARIES)
get_target_property(eureka_compile_options eurekasrc COMPILE_OPTIONS)
list(FILTER fltk_libs INCLUDE REGEX fltk)

# This is the common testing library. Contains whatever's in testUtils and some very common
# eurekasrc files

add_library(
    testutils 
    STATIC 
    testUtils/FatalHandler.cpp
    testUtils/FatalHandler.hpp
    testUtils/TempDirContext.cpp
    testUtils/TempDirContext.hpp
    testUtils/Palette.cpp
    testUtils/Palette.hpp
)
target_link_libraries(testutils PUBLIC gtest_main eurekacore)
if(WIN32)
    target_link_libraries(testutils PUBLIC Rpcrt4.lib)
endif()
target_include_directories(testutils PUBLIC ${src} ${src_includes})
target_compile_options(testutils PUBLIC ${eureka_compile_options})
# Need to link to them separately if FLTK is linked manually
if(APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(testutils PUBLIC ${CARBON_FRAMEWORK})
    find_library(COCOA_FRAMEWORK Cocoa)
    target_link_libraries(testutils PUBLIC ${COCOA_FRAMEWORK})
    find_library(APPSERVICES_FRAMEWORK ApplicationServices)
    target_link_libraries(testutils PUBLIC ${APPSERVICES_FRAMEWORK})
endif()

add_executable(
    test_general
    DocumentTest.cpp
    e_checks_test.cpp
    e_commands_test.cpp
    e_objects_test.cpp
    FixedPointTest.cpp
    im_color_test.cpp
    im_img_test.cpp
    lib_file_test.cpp
    lib_tga_test.cpp
    lib_util_test.cpp
    m_bitvec_test.cpp
    m_files_test.cpp
    m_game_test.cpp
    m_parse_test.cpp
    m_select_test.cpp
    m_streams_test.cpp
    m_testmap_test.cpp
    main_test.cpp
    r_grid_test.cpp
	SafeOutFileTest.cpp
    SectorTest.cpp
    SideTest.cpp
    SStringTest.cpp
    StringTableTest.cpp
    sys_debug_test.cpp
    ThingTest.cpp
    VertexTest.cpp
    w_dehacked_test.cpp
    w_loadpic_test.cpp
    w_texture_test.cpp
    w_wad_test.cpp
    WadDataTest.cpp
    stub/osxcalls_stub.cpp
)
target_link_libraries(test_general PRIVATE testutils eurekasrc)
if(APPLE)
    # Need to get access to the OSXCalls.h stuff
    target_include_directories(test_general PRIVATE ${CMAKE_SOURCE_DIR}/osx/EurekaApp)
    target_compile_definitions(test_general PRIVATE GL_SILENCE_DEPRECATION)
endif()
target_compile_definitions(test_general PUBLIC NO_OPENGL)
target_link_libraries(test_general PRIVATE ${fltk_libs})
if(UNIX AND NOT APPLE)
    find_package(ZLIB REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(test_general PRIVATE ${X11_X11_LIB} ${X11_Xpm_LIB} ${ZLIB_LIBRARIES})
endif()

add_test(NAME test_general COMMAND $<TARGET_FILE:test_general>)


# IMPORTANT: the eurekasrc files from testutils are already linked!


# unit_test(m_config_keys
#     m_config_test.cpp
#     m_keys_test.cpp
#     SRC im_color.cc
#         im_img.cc
#         lib_file.cc
#         m_config.cc
#         m_keys.cc
#         m_parse.cc
#         m_streams.cc
#         SafeOutFile.cc
#         w_wad.cc
#     FLTK
# )

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
