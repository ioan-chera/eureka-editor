set(resource_common
    ../bindings.cfg
    ../common
    ../defaults.cfg
    ../games
    ../misc/about_logo.png
    ../operations.cfg
    ../ports
)

add_executable(eureka MACOSX_BUNDLE ${resource_common})

set_source_files_properties(${resource_common} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

if(APPLE)
    set(exe_display_name "Eureka Doom Editor")
    set_target_properties(eureka PROPERTIES OUTPUT_NAME "${exe_display_name}")
else()
    set(exe_display_name "eureka")
endif()

if(APPLE)
    # Unfortunately we can't put set_source_files_properties in another file than the one with add_executable.
    set(osxmaindir "osx/EurekaApp")
    set(resource_mac
        "../${osxmaindir}/Eureka Doom EditorIcon.icns"
    )
    set(source_mac
        "../osx/EurekaApp/AppDelegate.h"
        "../osx/EurekaApp/AppDelegate.mm"
        "../osx/EurekaApp/OSXCalls.h"
        "../osx/EurekaApp/OSXCalls.mm"
    )

    target_sources(eureka PRIVATE ${source_mac})

    target_sources(eureka PRIVATE ${resource_mac})
    source_group(macOS FILES ${source_mac})

    # May be needed by files from other projects to find headers here
    target_include_directories(eureka PRIVATE "../osx/EurekaApp/")

    # Need to silence OpenGL deprecation warnings (they tell us what we know and can't fix)
    target_compile_definitions(eureka PRIVATE
        "GL_SILENCE_DEPRECATION"
        "main=main_ORIGINAL"
    )

    set_target_properties(eureka PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "../osx/EurekaApp/Eureka Doom Editor-Info.plist.in"

        MACOSX_BUNDLE_BUNDLE_NAME "${exe_display_name}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © The Eureka Team"
        MACOSX_BUNDLE_EXECUTABLE_NAME "${exe_display_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.eureka.Eureka-Doom-Editor"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "Eureka Doom EditorIcon"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )


    set_source_files_properties(${resource_mac} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# TODO: update the main header file to include version
# TODO: move these to where they can be reached by Visual Studio
configure_file(main.rc.in main.rc)
configure_file(version.h.in version.h)

set(lookout_dir ${PROJECT_SOURCE_DIR}/prereqs)

option(ENABLE_OPENGL "Use OpenGL instead of software rendering" ON)

if(ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
else()
    set(FLTK_SKIP_OPENGL TRUE)
    target_compile_definitions(eureka PRIVATE NO_OPENGL)
endif()

find_package(ZLIB REQUIRED)

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
    Document.cc
    Document.h
    DocumentModule.cc
    DocumentModule.h
    Errors.cc
    Errors.h
    hdr_fltk.h
    Instance.cc
    Instance.h
    Lump.cc
    Lump.h
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
    m_select.cc
    m_select.h
    m_strings.cc
    m_strings.h
    m_testmap.cc
    m_udmf.cc
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

target_link_libraries(eureka PRIVATE ${FLTK_LIBRARIES} ${ZLIB_LIBRARIES} ${OPENGL_LIBRARIES})
if(APPLE)
    target_link_libraries(eureka PRIVATE ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
endif()
if(UNIX AND NOT APPLE)  # Linux
    target_link_libraries(eureka PRIVATE Xpm)
endif()

target_include_directories(eureka PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${FLTK_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
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

if(UNIX AND NOT APPLE)  # Linux
    set(eureka_install_dir "${CMAKE_INSTALL_PREFIX}/share/eureka")
    install(TARGETS eureka
        RUNTIME DESTINATION "${BIN_DIR}")
    install(DIRECTORY ../games DESTINATION "${eureka_install_dir}" PATTERN "freedoom.ugh" EXCLUDE)
    install(DIRECTORY ../common DESTINATION "${eureka_install_dir}")
    install(DIRECTORY ../ports DESTINATION "${eureka_install_dir}")
    install(FILES ../bindings.cfg
                ../defaults.cfg
                ../misc/about_logo.png
                ../operations.cfg
                DESTINATION "${eureka_install_dir}")
    # The full-install sequence is here
    install(CODE "execute_process(
        COMMAND xdg-desktop-menu install --novendor ../misc/eureka.desktop
        COMMAND xdg-icon-resource install --novendor --size 32 ../misc/eureka.xpm
    )")
endif()
# TODO: add the warnings and warnings-as-errors
