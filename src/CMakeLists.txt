# TODO: update the main header file to include version
# TODO: move these to where they can be reached by Visual Studio
configure_file(main.rc.in main.rc)
configure_file(version.h.in version.h)

set(lookout_dir ${PROJECT_SOURCE_DIR}/prereqs)

find_package(OpenGL REQUIRED)
find_package(FLTK)
if(NOT FLTK_FOUND)
    if(WIN32)
        message("FLTK not found in standard location; looking in ${lookout_dir}/lib/x86")
        set(FLTK_LIBRARIES
            ${lookout_dir}/lib/x86/fltk.lib
            ${lookout_dir}/lib/x86/fltkforms.lib
            ${lookout_dir}/lib/x86/fltkgl.lib
            ${lookout_dir}/lib/x86/fltkimages.lib
            ${lookout_dir}/lib/x86/fltkjpeg.lib
            ${lookout_dir}/lib/x86/fltkpng.lib
            ${lookout_dir}/lib/x86/fltkzlib.lib
        )
        set(FLTK_INCLUDE_DIR ${lookout_dir}/include)
    endif()
endif()

if(APPLE)
    # Linking statically needs to also find extra dependencies
    find_package(PNG REQUIRED)
    find_package(JPEG REQUIRED)
endif()

set(source_base
    hdr_fltk.h
    main.cc
    main.h
    main.rc
    objid.h
    version.h
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
    lib_adler.cc
    lib_adler.h
    lib_file.cc
    lib_file.h
    lib_tga.cc
    lib_tga.h
    lib_util.cc
    lib_util.h
)

set(source_m
    m_bitvec.cc
    m_bitvec.h
    m_config.cc
    m_config.h
    m_editlump.cc
    m_editlump.h
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
    m_strings.cc
    m_strings.h
    m_testmap.cc
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

set(source_sys
    sys_debug.cc
    sys_debug.h
    sys_endian.h
    sys_macro.h
    sys_type.h
)

set(source_ui
    ui_about.cc
    ui_about.h
    ui_browser.cc
    ui_browser.h
    ui_canvas.cc
    ui_canvas.h
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
    ui_linedef.cc
    ui_linedef.h
    ui_menu.cc
    ui_menu.h
    ui_misc.cc
    ui_misc.h
    ui_nombre.cc
    ui_nombre.h
    ui_pic.cc
    ui_pic.h
    ui_prefs.cc
    ui_prefs.h
    ui_replace.cc
    ui_replace.h
    ui_scroll.cc
    ui_scroll.h
    ui_sector.cc
    ui_sector.h
    ui_sidedef.cc
    ui_sidedef.h
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
    w_loadpic.cc
    w_loadpic.h
    w_rawdef.h
    w_texture.cc
    w_texture.h
    w_wad.cc
    w_wad.h
)

target_sources(
    eureka PRIVATE
    ${source_base}
    ${source_bsp}
    ${source_e}
    ${source_im}
    ${source_lib}
    ${source_m}
    ${source_r}
    ${source_sys}
    ${source_ui}
    ${source_w}
)

if(APPLE)
    # We need to link statically, not dynamically, if we want portability
    string(REPLACE ".dylib" ".a" FLTK_LIBRARIES "${FLTK_LIBRARIES}" )
    string(REPLACE ".dylib" ".a" JPEG_LIBRARIES "${JPEG_LIBRARIES}" )
    string(REPLACE "libpng.dylib" "libpng.a" PNG_LIBRARIES "${PNG_LIBRARIES}" )
endif()

target_link_libraries(eureka PRIVATE ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES})
if(APPLE)
    target_link_libraries(eureka PRIVATE ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
endif()

target_include_directories(eureka PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${FLTK_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# May be needed by files from other projects to find headers here
target_include_directories(eureka PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(eureka PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
    target_compile_options(eureka PRIVATE "/MP")
endif()

# FIXME: this doesn't work when in this file I only have "target_sources"
source_group(bsp FILES ${source_bsp})
source_group("e_ source" FILES ${source_e})
source_group("im_ source" FILES ${source_im})
source_group("lib_ source" FILES ${source_lib})
source_group("im_ source" FILES ${source_im})
source_group("lib_ source" FILES ${source_lib})
source_group("m_ source" FILES ${source_m})
source_group("r_ source" FILES ${source_r})
source_group("sys_ source" FILES ${source_sys})
source_group("ui_ source" FILES ${source_ui})
source_group("w_ source" FILES ${source_w})