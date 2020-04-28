# TODO: configure to get the version header
# TODO: update the main header file to include version

set(source_base
    hdr_fltk.h
    main.cc
    main.h
    objid.h
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

add_executable(
    eureka MACOSX_BUNDLE
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
