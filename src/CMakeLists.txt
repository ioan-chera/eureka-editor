
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

set(source_thirdparty
    filesystem.hpp
    tl/optional.hpp
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
    ui_panelinput.cc
    ui_panelinput.h
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
    ${source_thirdparty}
    ${source_ui}
    ${source_w}
)

add_library(
    eurekacore STATIC
    Errors.cc
    Errors.h
    filesystem.hpp
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

if(APPLE OR WIN32)
    message(STATUS "Using local FLTK for portability.")

	set(LOCAL_FLTK_VERSION "1.3.8")
	set(LOCAL_FLTK_URL "https://www.fltk.org/pub/fltk/${LOCAL_FLTK_VERSION}/fltk-${LOCAL_FLTK_VERSION}-source.tar.gz")
	set(LOCAL_FLTK_MD5_HASH 84907602c2e50fadec3bc40fb61935cd)
	set(LOCAL_FLTK_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/fltk-${LOCAL_FLTK_VERSION}")

    set(LOCAL_FLTK_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/fltk-${LOCAL_FLTK_VERSION}-source.tar.gz")
    set(LOCAL_FLTK_BUILD "${LOCAL_FLTK_SOURCE}/build")

    # Download it if not found
    if(NOT EXISTS "${LOCAL_FLTK_ARCHIVE}")
        message(STATUS "Downloading FLTK from ${LOCAL_FLTK_URL}")
        file(
            DOWNLOAD ${LOCAL_FLTK_URL} "${LOCAL_FLTK_ARCHIVE}"
            EXPECTED_HASH MD5=${LOCAL_FLTK_MD5_HASH}
        )
    endif()
    if(NOT EXISTS "${LOCAL_FLTK_SOURCE}")
        message(STATUS "Extracting to ${LOCAL_FLTK_SOURCE}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xzf "${LOCAL_FLTK_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        # Patch needed to prevent optimization bug
        if(APPLE)
            execute_process(
                COMMAND patch "${LOCAL_FLTK_SOURCE}/src/Fl_JPEG_Image.cxx"
                              "${CMAKE_CURRENT_SOURCE_DIR}/fltk-patch/Fl_JPEG_Image-fltk1.3.8-macos.diff"
            )
        endif()
    endif()
    if(NOT EXISTS "${LOCAL_FLTK_BUILD}/lib")

        message(STATUS "Building ${LOCAL_FLTK_SOURCE}")

        set(LOCAL_FLTK_DEFS -D CMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                            -D CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
                            -D CMAKE_BUILD_TYPE=Release
                            -D OPTION_USE_SYSTEM_LIBJPEG=OFF -D OPTION_USE_SYSTEM_LIBPNG=OFF
                            -D OPTION_USE_SYSTEM_ZLIB=OFF
        )
        if(CMAKE_GENERATOR_PLATFORM)
            list(APPEND LOCAL_FLTK_DEFS -A ${CMAKE_GENERATOR_PLATFORM})
        endif()
        if(FLTK_SKIP_OPENGL)
            list(APPEND LOCAL_FLTK_DEFS -D OPTION_USE_GL=OFF)
        endif()
        if(WIN32)
            list(APPEND LOCAL_FLTK_DEFS -G ${CMAKE_GENERATOR})
        endif()

        execute_process(
            COMMAND "${CMAKE_COMMAND}" -S "${LOCAL_FLTK_SOURCE}" -B "${LOCAL_FLTK_BUILD}"
                    ${LOCAL_FLTK_DEFS}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        include(ProcessorCount)
        ProcessorCount(nproc)
        message(STATUS "Building...")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build "${LOCAL_FLTK_BUILD}" -j ${nproc} --config Release
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
    endif()

    set(FLTK_INCLUDE_DIR "${LOCAL_FLTK_SOURCE};${LOCAL_FLTK_BUILD}")
    if(WIN32)
        set(FLTK_LIBRARIES "${LOCAL_FLTK_BUILD}/lib/Release/fltk.lib"
                           "${LOCAL_FLTK_BUILD}/lib/Release/fltk_gl.lib"
                           "${LOCAL_FLTK_BUILD}/lib/Release/fltk_images.lib"
                           "${LOCAL_FLTK_BUILD}/lib/Release/fltk_jpeg.lib"
                           "${LOCAL_FLTK_BUILD}/lib/Release/fltk_png.lib"
                           "${LOCAL_FLTK_BUILD}/lib/Release/fltk_z.lib"
        )
        target_include_directories(eurekasrc PUBLIC "${LOCAL_FLTK_SOURCE}/zlib")
    else()  # APPLE
        set(FLTK_LIBRARIES "${LOCAL_FLTK_BUILD}/lib/libfltk.a"
                           "${LOCAL_FLTK_BUILD}/lib/libfltk_gl.a"
                           "${LOCAL_FLTK_BUILD}/lib/libfltk_images.a"
                           "${LOCAL_FLTK_BUILD}/lib/libfltk_jpeg.a"
                           "${LOCAL_FLTK_BUILD}/lib/libfltk_z.a"
                           "${LOCAL_FLTK_BUILD}/lib/libfltk_png.a"
        )

        # Need to link to them separately if FLTK is linked manually
        find_library(CARBON_FRAMEWORK Carbon)
        target_link_libraries(eurekasrc PUBLIC ${CARBON_FRAMEWORK})
        find_library(COCOA_FRAMEWORK Cocoa)
        target_link_libraries(eurekasrc PUBLIC ${COCOA_FRAMEWORK})
        find_library(APPSERVICES_FRAMEWORK ApplicationServices)
        target_link_libraries(eurekasrc PUBLIC ${APPSERVICES_FRAMEWORK})
    endif()

else()
    # UNIX/Linux
    find_package(FLTK REQUIRED)
    find_package(ZLIB REQUIRED) # ZLIB is expected on Linux separately from FLTK
    find_package(X11 REQUIRED)  # also libXPM
endif()

target_link_libraries(eurekasrc PUBLIC ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES})
if(UNIX AND NOT APPLE)  # Linux
    target_link_libraries(eurekasrc PUBLIC ${X11_Xpm_LIB} ${ZLIB_LIBRARIES})
endif()

target_include_directories(eurekasrc PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${FLTK_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# May be needed by files from other projects to find headers here
target_include_directories(eurekasrc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(eurekasrc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
    target_compile_options(eurekasrc PUBLIC "/MP")
endif()

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    target_compile_options(eurekasrc PUBLIC "${CMAKE_CXX_FLAGS}" -Wall -Wextra -Werror
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
source_group("third-party" FILES ${source_thirdparty})

# TODO: add the warnings and warnings-as-errors
