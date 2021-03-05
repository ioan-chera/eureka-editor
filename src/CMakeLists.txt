
# TODO: move these to where they can be reached by Visual Studio
configure_file(main.rc.in main.rc)
configure_file(version.h.in version.h)

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

add_library(
    eurekamain STATIC
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

set(lookout_dir ${PROJECT_SOURCE_DIR}/prereqs)

option(ENABLE_OPENGL "Use OpenGL instead of software rendering" ON)

# TODO: add library files

# TODO: remove this when everything is moved to CMake
target_compile_definitions(eurekamain PUBLIC BUILT_VIA_CMAKE)

if(ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
else()
    set(FLTK_SKIP_OPENGL TRUE)
    target_compile_definitions(eurekamain PUBLIC NO_OPENGL)
endif()

if(APPLE OR WIN32)
    message(STATUS "Using local FLTK for portability.")

    set(LOCAL_FLTK_VERSION "1.3.5")
    set(LOCAL_FLTK_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/fltk-${LOCAL_FLTK_VERSION}-source.tar.gz")
    set(LOCAL_FLTK_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/fltk-${LOCAL_FLTK_VERSION}")
    set(LOCAL_FLTK_BUILD "${LOCAL_FLTK_SOURCE}/build")

    # Download it if not found
    if(NOT EXISTS "${LOCAL_FLTK_ARCHIVE}")
        message(STATUS "Downloading ${LOCAL_FLTK_ARCHIVE}")
        file(
            DOWNLOAD "https://www.fltk.org/pub/fltk/${LOCAL_FLTK_VERSION}/fltk-${LOCAL_FLTK_VERSION}-source.tar.gz" 
                "${LOCAL_FLTK_ARCHIVE}" 
            EXPECTED_HASH MD5=e85017defd5a03ae82e634311db87bbf
        )
    endif()
    if(NOT EXISTS "${LOCAL_FLTK_SOURCE}")
        message(STATUS "Extracting to ${LOCAL_FLTK_SOURCE}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xzf "${LOCAL_FLTK_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()
    if(NOT EXISTS "${LOCAL_FLTK_BUILD}/lib")
                                
        message(STATUS "Building ${LOCAL_FLTK_SOURCE}")

        set(LOCAL_FLTK_DEFS -D OPTION_USE_SYSTEM_LIBJPEG=OFF -D OPTION_USE_SYSTEM_LIBPNG=OFF -D OPTION_USE_SYSTEM_ZLIB=OFF)
        if(FLTK_SKIP_OPENGL)
            set(LOCAL_FLTK_DEFS "${LOCAL_FLTK_DEFS} -D OPTION_USE_GL=OFF")
        endif()

        execute_process(
            COMMAND "${CMAKE_COMMAND}" -S "${LOCAL_FLTK_SOURCE}" -B "${LOCAL_FLTK_BUILD}" ${LOCAL_FLTK_DEFS}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build "${LOCAL_FLTK_BUILD}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()

    set(FLTK_INCLUDE_DIR "${LOCAL_FLTK_SOURCE};${LOCAL_FLTK_BUILD}")
    set(FLTK_LIBRARIES "${LOCAL_FLTK_BUILD}/lib/libfltk.a"
                       "${LOCAL_FLTK_BUILD}/lib/libfltk_gl.a"
                       "${LOCAL_FLTK_BUILD}/lib/libfltk_images.a"
                       "${LOCAL_FLTK_BUILD}/lib/libfltk_jpeg.a"
                       "${LOCAL_FLTK_BUILD}/lib/libfltk_z.a"
                       "${LOCAL_FLTK_BUILD}/lib/libfltk_png.a")

    if(APPLE)
        # Need to link to them separately if FLTK is linked manually
        find_library(CARBON_FRAMEWORK Carbon)
        target_link_libraries(eurekamain PUBLIC ${CARBON_FRAMEWORK}) 
        find_library(COCOA_FRAMEWORK Cocoa)
        target_link_libraries(eurekamain PUBLIC ${COCOA_FRAMEWORK}) 
        find_library(APPSERVICES_FRAMEWORK ApplicationServices)
        target_link_libraries(eurekamain PUBLIC ${APPSERVICES_FRAMEWORK})
    endif()

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
else()
    # UNIX/Linux
    find_package(FLTK REQUIRED)
endif()

target_link_libraries(eurekamain PUBLIC ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES})
if(UNIX AND NOT APPLE)  # Linux
    target_link_libraries(eurekamain PUBLIC Xpm)
endif()

target_include_directories(eurekamain PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${FLTK_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# May be needed by files from other projects to find headers here
target_include_directories(eurekamain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(eurekamain PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
    target_compile_options(eurekamain PUBLIC "/MP")
endif()

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(eurekamain PUBLIC "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers -O2 -g")
endif()
if ( MSVC )
    target_compile_options(eurekamain PUBLIC "${CMAKE_CXX_FLAGS} /W4")
endif()

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

# TODO: add the warnings and warnings-as-errors
