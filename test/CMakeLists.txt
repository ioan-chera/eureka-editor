message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
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
    ${src}/lib_adler.cc
    ${src}/lib_util.cc
    ${src}/m_strings.cc
    ${src}/sys_debug.cc
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
            if(APPLE)
                # Need to link to them separately if FLTK is linked manually
                find_library(CARBON_FRAMEWORK Carbon)
                target_link_libraries(testutils PUBLIC ${CARBON_FRAMEWORK})
                find_library(COCOA_FRAMEWORK Cocoa)
                target_link_libraries(testutils PUBLIC ${COCOA_FRAMEWORK})
                find_library(APPSERVICES_FRAMEWORK ApplicationServices)
                target_link_libraries(testutils PUBLIC ${APPSERVICES_FRAMEWORK})
            endif()
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

    if(APPLE)
        # Need to get access to the OSXCalls.h stuff
        target_include_directories(${_name} PRIVATE ${CMAKE_SOURCE_DIR}/osx/EurekaApp)
        target_compile_definitions(${_name} PRIVATE GL_SILENCE_DEPRECATION)
    endif()
    target_compile_definitions(${_name} PRIVATE main=mainDISABLED NO_OPENGL UNIT_TESTING)

    if(_use_fltk)
        target_link_libraries(${_name} PRIVATE ${fltk_libs})
    endif()
    if(WIN32 AND _windows_libs)
        target_link_libraries(${_name} PRIVATE ${_windows_libs})
    endif()
    if(UNIX AND NOT APPLE)
        # NOTE: maybe also X11_Xpm_LIB, but then remove the UNIT_TEST macro
        find_package(ZLIB REQUIRED)
        target_link_libraries(${_name} PRIVATE ${ZLIB_LIBRARIES})
    endif()

    add_test(NAME ${_name} COMMAND $<TARGET_FILE:${_name}>)
endfunction()

# IMPORTANT: the eurekasrc files from testutils are already linked!

unit_test(general
    m_files_test.cpp
    stub/osxcalls_stub.cpp
    SRC bsp_level.cc
        bsp_node.cc
        bsp_util.cc
        Document.cc
        DocumentModule.cc
        e_basis.cc
        e_checks.cc
        e_commands.cc
        e_cutpaste.cc
        e_hover.cc
        e_linedef.cc
        e_main.cc
        e_objects.cc
        e_path.cc
        e_sector.cc
        e_things.cc
        e_vertex.cc
        im_color.cc
        im_img.cc
        Instance.cc
        lib_file.cc
        lib_tga.cc
        LineDef.cc
        m_bitvec.cc
        m_config.cc
        m_editlump.cc
        m_events.cc
        m_files.cc
        m_keys.cc
        m_game.cc
        m_loadsave.cc
        m_nodes.cc
        m_parse.cc
        m_select.cc
        m_streams.cc
        m_testmap.cc
        m_udmf.cc
        main.cc
        r_grid.cc
        r_opengl.cc
        r_render.cc
        r_software.cc
        r_subdiv.cc
        SafeOutFile.cc
        Sector.cc
        SideDef.cc
        Thing.cc
        ui_about.cc
        ui_browser.cc
        ui_canvas.cc
        ui_default.cc
        ui_dialog.cc
        ui_editor.cc
        ui_file.cc
        ui_hyper.cc
        ui_infobar.cc
        ui_linedef.cc
        ui_menu.cc
        ui_misc.cc
        ui_nombre.cc
        ui_panelinput.cc
        ui_pic.cc
        ui_prefs.cc
        ui_replace.cc
        ui_scroll.cc
        ui_sector.cc
        ui_sidedef.cc
        ui_thing.cc
        ui_tile.cc
        ui_vertex.cc
        ui_window.cc
        w_loadpic.cc
        w_texture.cc
        w_wad.cc
    FLTK
)

unit_test(document
    DocumentTest.cpp
    stub/e_cutpaste_stub.cpp
    stub/e_main_stub.cpp
    stub/r_render_stub.cpp
    stub/ui_infobar_stub.cpp
    SectorTest.cpp
    ThingTest.cpp
    VertexTest.cpp
    SRC Document.cc
        DocumentModule.cc
        e_basis.cc
        LineDef.cc
        m_bitvec.cc
        m_select.cc
        Sector.cc
        SideDef.cc
        Thing.cc
)

unit_test(e_checks
    e_checks_test.cpp
    stub/e_cutpaste_stub.cpp
    stub/e_hover_stub.cpp
    stub/e_main_stub.cpp
    stub/e_path_stub.cpp
    stub/im_img_stub.cpp
    stub/m_game_stub.cpp
    stub/m_keys_stub.cpp
    stub/r_render_stub.cpp
    stub/ui_dialog_stub.cpp
    stub/ui_infobar_stub.cpp
    SRC DocumentModule.cc
        e_basis.cc
        e_checks.cc
        LineDef.cc
        m_bitvec.cc
        m_select.cc
        Sector.cc
        SideDef.cc
    FLTK
)

unit_test(lib_file
    lib_file_test.cpp
    SRC lib_file.cc
    FLTK
)

unit_test(m_game
    m_game_test.cpp
    SRC e_basis.cc
        lib_file.cc
        m_bitvec.cc
        m_game.cc
        m_select.cc
        m_streams.cc
    FLTK
)

unit_test(m_config_keys
    m_config_test.cpp
    m_keys_test.cpp
    SRC lib_file.cc
        m_config.cc
        m_keys.cc
        m_parse.cc
        m_streams.cc
    FLTK
)



unit_test(w_wad
    w_wad_test.cpp
    SRC lib_file.cc
        SafeOutFile.cc
        w_wad.cc
    FLTK
)

# Units independent on complex frameworks or libraries
unit_test(independent
    FixedPointTest.cpp
    lib_util_test.cpp
    m_bitvec_test.cpp
    m_parse_test.cpp
    m_select_test.cpp
    m_streams_test.cpp
    SafeOutFileTest.cpp
    SideTest.cpp
    SStringTest.cpp
    StringTableTest.cpp
    sys_debug_test.cpp
    SRC m_bitvec.cc
        m_parse.cc
        m_select.cc
        m_streams.cc
        SafeOutFile.cc
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
