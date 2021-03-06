cmake_minimum_required(VERSION 3.13)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(eureka-editor VERSION 2.0.0)

# Why not use the latest C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# https://stackoverflow.com/a/66401708/11738219
function(add_path_resource target folder)
    file(GLOB_RECURSE resources "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/*")
    target_sources(${target} PRIVATE ${resources})
    foreach(resfile ${resources})
        #Get the relative path from the data-folder to the particular file
        file(RELATIVE_PATH newfile "${CMAKE_CURRENT_SOURCE_DIR}/${folder}" ${resfile})

        get_filename_component(newfilepath ${newfile} DIRECTORY)
        #Set its location inside the app package (under Resources)
        set_property(SOURCE ${resfile} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${folder}/${newfilepath}")

    endforeach()
    source_group("eureka-res-clutter" FILES ${resources})
endfunction()

set(resource_common
    bindings.cfg
    common
    defaults.cfg
    games
    misc/about_logo.png
    operations.cfg
    ports
)
source_group("eureka-res" FILES ${resource_common})

set_source_files_properties(${resource_common} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
add_executable(eureka MACOSX_BUNDLE ${resource_common})
add_path_resource(eureka common)
add_path_resource(eureka games)
add_path_resource(eureka ports)

if(APPLE)
    set(exe_display_name "Eureka Doom Editor")
    set_target_properties(eureka PROPERTIES OUTPUT_NAME "${exe_display_name}")

    add_subdirectory(osx)
    target_link_libraries(eureka eurekamac)
    set_source_files_properties(${resource_mac} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(eureka PRIVATE ${resource_mac})

    set_target_properties(eureka PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/osx/EurekaApp/Eureka Doom Editor-Info.plist.in"

        MACOSX_BUNDLE_BUNDLE_NAME "${exe_display_name}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © The Eureka Team"
        MACOSX_BUNDLE_EXECUTABLE_NAME "${exe_display_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "$(PRODUCT_BUNDLE_IDENTIFIER)"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "Eureka Doom EditorIcon"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"

        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.eureka.Eureka-Doom-Editor"

        XCODE_ATTRIBUTE_INSTALL_PATH "/Applications"
        XCODE_ATTRIBUTE_SKIP_INSTALL "No"
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )

else()
    set(exe_display_name "eureka")
endif()

add_subdirectory(src)
if(APPLE)
    target_link_libraries(eurekasrc PRIVATE eurekamac)
    target_link_libraries(eurekamac PRIVATE eurekasrc)
endif()
target_link_libraries(eureka eurekasrc)

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

# uninstall target
if(UNIX AND NOT APPLE)
    if(NOT TARGET uninstall)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY
        )

        add_custom_target(
            uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        )
    endif()
endif()
