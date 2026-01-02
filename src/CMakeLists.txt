
configure_file(version.h.in version.h)

set(source_base
    dehconsts.h
    Document.cc
    Document.h
    DocumentModule.cc
    DocumentModule.h
    hdr_fltk.h
    Instance.cc
    Instance.h
    LineDef.cc
    LineDef.h
    main.cc
    main.h
    objid.h
    Sector.cc
    Sector.h
    Side.h
    SideDef.cc
    SideDef.h
    Thing.cc
    Thing.h
    Vertex.cc
    Vertex.h
    WadData.cc
    WadData.h
)

set(source_bsp
    bsp_level.cc
    bsp_node.cc
    bsp_util.cc
    bsp.h
)

set(source_e
    e_basis.cc
    e_basis.h
    e_checks.cc
    e_checks.h
    e_commands.cc
    e_cutpaste.cc
    e_cutpaste.h
    e_hover.cc
    e_hover.h
    e_linedef.cc
    e_linedef.h
    e_main.cc
    e_main.h
    e_objects.cc
    e_objects.h
    e_path.cc
    e_path.h
    e_select.cc
    e_sector.cc
    e_sector.h
    e_things.cc
    e_things.h
    e_vertex.cc
    e_vertex.h
)

set(source_im
    im_color.cc
    im_color.h
    im_img.cc
    im_img.h
)

set(source_lib
    lib_tga.cc
    lib_tga.h
)

set(source_m
    m_config.cc
    m_config.h
    m_editlump.cc
    m_events.cc
    m_events.h
    m_files.cc
    m_files.h
    m_game.cc
    m_game.h
    m_keys.cc
    m_keys.h
    m_loadsave.cc
    m_loadsave.h
    m_nodes.cc
    m_nodes.h
    m_select.cc
    m_select.h
    m_testmap.cc
    m_testmap.h
    m_udmf.cc
    m_udmf.h
)

set(source_r
    r_grid.cc
    r_grid.h
    r_opengl.cc
    r_render.cc
    r_render.h
    r_software.cc
    r_subdiv.cc
    r_subdiv.h
)

set(source_ui
    ui_about.cc
    ui_about.h
    ui_browser.cc
    ui_browser.h
    ui_canvas.cc
    ui_canvas.h
    ui_category_button.cc
    ui_category_button.h
    ui_default.cc
    ui_default.h
    ui_dialog.cc
    ui_editor.cc
    ui_editor.h
    ui_file.cc
    ui_file.h
    ui_hyper.cc
    ui_hyper.h
    ui_infobar.cc
    ui_infobar.h
    ui_keybindingstable.cc
    ui_keybindingstable.h
    ui_linedef.cc
    ui_linedef.h
    ui_menu.cc
    ui_menu.h
    ui_misc.cc
    ui_misc.h
    ui_nombre.cc
    ui_nombre.h
    ui_panelinput.cc
    ui_panelinput.h
    ui_pic.cc
    ui_pic.h
    ui_prefs.cc
    ui_replace.cc
    ui_replace.h
    ui_scroll.cc
    ui_scroll.h
    ui_sector.cc
    ui_sector.h
    ui_sidedef.cc
    ui_sidedef.h
    ui_stackpanel.cc
    ui_stackpanel.h
    ui_thing.cc
    ui_thing.h
    ui_tile.cc
    ui_tile.h
    ui_vertex.cc
    ui_vertex.h
    ui_window.cc
    ui_window.h
)

set(source_w
    w_dehacked.cc
    w_dehacked.h
    w_loadpic.cc
    w_loadpic.h
    w_rawdef.h
    w_texture.cc
    w_texture.h
    w_wad.cc
    w_wad.h
)

add_library(
    eurekasrc STATIC
    ${source_base}
    ${source_bsp}
    ${source_e}
    ${source_im}
    ${source_lib}
    ${source_m}
    ${source_r}
    ${source_ui}
    ${source_w}
)

add_library(
    eurekacore STATIC
    Errors.cc
    Errors.h
    FixedPoint.h
    lib_adler.cc
    lib_adler.h
    lib_file.cc
    lib_file.h
    lib_util.cc
    lib_util.h
    m_bitvec.cc
    m_bitvec.h
    m_strings.cc
    m_strings.h
    m_parse.cc
    m_parse.h
    m_streams.cc
    m_streams.h
    m_vector.cc
    m_vector.h
    PrintfMacros.h
    safe_ctype.cc
    safe_ctype.h
    SafeOutFile.cc
    SafeOutFile.h
    sys_debug.cc
    sys_debug.h
    sys_endian.h
    sys_macro.h
    sys_type.h
    WindowsSanitization.h
)

target_link_libraries(eurekasrc PRIVATE eurekacore)

# Needed for macOS release archiving!
set_target_properties(eurekasrc eurekacore PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                      ${PROJECT_BINARY_DIR}/out/library)

option(ENABLE_OPENGL "Use OpenGL instead of software rendering" ON)

if(ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
else()
    set(FLTK_SKIP_OPENGL TRUE)
    target_compile_definitions(eurekasrc PUBLIC NO_OPENGL)
endif()

#
# Fetch FLTK
#

option(USE_SYSTEM_FLTK "Use system FLTK instead of locally-fetched source")
if(USE_SYSTEM_FLTK)
    message(STATUS "Using system FLTK.")
    find_package(FLTK REQUIRED)
    message(VERBOSE "Found FLTK_INCLUDE_DIR ${FLTK_INCLUDE_DIR}")
    message(VERBOSE "Found FLTK_LIBRARIES ${FLTK_LIBRARIES}")
    message(VERBOSE "Found FLTK_FLUID_EXECUTABLE ${FLTK_FLUID_EXECUTABLE}")
    set(LOCAL_FLTK_INSTALL_LIBS X11)    # Extra libraries needed with 1.3
else()
    message(STATUS "Using fetch-content FLTK for portability.")
    include(FetchContent)

    FetchContent_Declare(
        FLTK
        GIT_REPOSITORY https://github.com/ioan-chera/fltk.git
        GIT_TAG e0c419c286d0c21910dd46de1680aee6cb7128b9    # branch-1.4-eureka-fix
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(FLTK)
endif()

target_include_directories(
    eurekasrc
    PUBLIC
    ${fltk_BINARY_DIR}
    ${fltk_SOURCE_DIR}
    ${FLTK_INCLUDE_DIR}
)
target_link_libraries(
    eurekasrc
    PRIVATE
    fltk fltk_gl fltk_images ${LOCAL_FLTK_INSTALL_LIBS}
)

if(APPLE)
    # Need to link to them separately if FLTK is linked manually
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(eurekasrc PUBLIC ${CARBON_FRAMEWORK})
    find_library(COCOA_FRAMEWORK Cocoa)
    target_link_libraries(eurekasrc PUBLIC ${COCOA_FRAMEWORK})
    find_library(APPSERVICES_FRAMEWORK ApplicationServices)
    target_link_libraries(eurekasrc PUBLIC ${APPSERVICES_FRAMEWORK})
    find_package(ZLIB REQUIRED)
    target_link_libraries(eurekasrc PUBLIC ${ZLIB_LIBRARIES})
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)  # also libXPM
    find_package(ZLIB REQUIRED) # ZLIB is expected on Linux separately from FLTK
    find_package(JPEG REQUIRED)
    find_package(PNG REQUIRED)
    target_link_libraries(
        eurekasrc
        PUBLIC
        ${X11_Xpm_LIB}
        ${ZLIB_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${PNG_LIBRARIES}
    )
else()
    target_link_libraries(
        eurekasrc
        PRIVATE
        fltk_jpeg fltk_png fltk_z
    )
    target_include_directories(
        eurekasrc
        PUBLIC
        ${fltk_SOURCE_DIR}/zlib
    )
endif()

target_link_libraries(eurekasrc PUBLIC ${OPENGL_LIBRARIES})

target_include_directories(eurekasrc PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# May be needed by files from other projects to find headers here
target_include_directories(eurekasrc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(eurekasrc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
    target_compile_options(eurekasrc PUBLIC "/MP" "/utf-8")
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    target_compile_options(eurekasrc PUBLIC -Wall -Wextra
                           -Wno-unused-parameter -Wno-missing-field-initializers -Wunused-variable)
endif()
if ( MSVC )
    # TODO: make remove /W3 if there and set /W4
    #target_compile_options(eurekasrc PUBLIC ${CMAKE_CXX_FLAGS} /W4)
endif()

source_group(bsp FILES ${source_bsp})
source_group("e_ source" FILES ${source_e})
source_group("im_ source" FILES ${source_im})
source_group("lib_ source" FILES ${source_lib})
source_group("im_ source" FILES ${source_im})
source_group("lib_ source" FILES ${source_lib})
source_group("m_ source" FILES ${source_m})
source_group("r_ source" FILES ${source_r})
source_group("ui_ source" FILES ${source_ui})
source_group("w_ source" FILES ${source_w})

# TODO: add the warnings and warnings-as-errors
