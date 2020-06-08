cmake_minimum_required(VERSION 3.13)

project(eureka-editor VERSION 2.0.0)

# Why not use the latest C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(resource_common
    bindings.cfg
    defaults.cfg
    misc/about_logo.png
    operations.cfg
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
        "${CMAKE_CURRENT_SOURCE_DIR}/${osxmaindir}/Eureka Doom EditorIcon.icns"
    )
    add_subdirectory(osx)
    set_source_files_properties(${resource_mac} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()
add_subdirectory(src)
