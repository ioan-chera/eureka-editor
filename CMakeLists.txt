cmake_minimum_required(VERSION 3.13)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")

project(eureka-editor VERSION 2.0.1)

# Why not use the latest C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

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

set(resource_common_dirs common games ports)
set(resource_common_cfgs bindings.cfg defaults.cfg operations.cfg)
set(resource_common_files ${resource_common_cfgs} misc/about_logo.png)

set(resource_common ${resource_common_dirs} ${resource_common_files})
source_group("eureka-res" FILES ${resource_common})

set_source_files_properties(${resource_common} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
# This is the main program. MACOSX_BUNDLE is needed for Mac, dummywin for Windows
add_executable(eureka MACOSX_BUNDLE ${resource_common} win/dummywin.cpp)
foreach(dir ${resource_common_dirs})
    add_path_resource(eureka common)
    add_path_resource(eureka games)
    add_path_resource(eureka ports)
endforeach()

if(WIN32)
    configure_file(src/main.rc.in src/main.rc)
    target_sources(eureka PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/main.rc)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT eureka)
    list(TRANSFORM resource_common_cfgs PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

    add_custom_command(
        TARGET eureka
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${resource_common_cfgs} $<TARGET_FILE_DIR:eureka>
    )
    foreach(dir ${resource_common_dirs})
        add_custom_command(
            TARGET eureka
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                "$<TARGET_FILE_DIR:eureka>/${dir}"
        )
    endforeach()
    add_custom_command(
        TARGET eureka
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/misc/about_logo.png"
            "$<TARGET_FILE_DIR:eureka>/common"
    )
endif()

if(NOT APPLE)
    target_sources(eureka PRIVATE mainroot.cc)
endif()

if(APPLE)
    set(exe_display_name "Eureka Doom Editor")
    set_target_properties(eureka PROPERTIES OUTPUT_NAME "${exe_display_name}")

    add_subdirectory(osx)
    target_link_libraries(eureka eurekamac)
    set_source_files_properties(${resource_mac} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(eureka PRIVATE ${resource_mac})

    # Use the same warnings as in the old manual project
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_FLOAT_CONVERSION NO)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INFINITE_RECURSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_NON_LITERAL_NULL_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_STRICT_PROTOTYPES YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_TREAT_IMPLICIT_FUNCTION_DECLARATIONS_AS_ERRORS YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_TREAT_INCOMPATIBLE_POINTER_TYPE_WARNINGS_AS_ERRORS YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_MISSING_PARENTHESES NO)  # do not want
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION YES)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE YES)

    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_RANGE_LOOP_ANALYSIS YES)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_MOVE YES)

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
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS  ${CMAKE_SOURCE_DIR}/osx/EurekaApp/eureka.entitlements

        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS           "YES"
        XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT                  "NO"
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP                  "NO"
    )

else()
    set(exe_display_name "eureka")
    if(WIN32)
        add_subdirectory(win)
    endif()
endif()

add_subdirectory(src)
if(APPLE)
    target_link_libraries(eurekasrc PRIVATE eurekamac)
    target_link_libraries(eurekamac PRIVATE eurekasrc)
endif()
if(WIN32)
    target_link_libraries(eurekasrc PRIVATE eurekawin)
endif()
target_link_libraries(eureka eurekasrc)

option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
if(ENABLE_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Currently, installing via CMake is only supported on GNU/Linux and similar systems
if(UNIX AND NOT APPLE)  # Linux
    set(eureka_install_dir "${CMAKE_INSTALL_PREFIX}/share/eureka")
    install(TARGETS eureka
            RUNTIME DESTINATION "${BIN_DIR}"
    )
    install(DIRECTORY games DESTINATION "${eureka_install_dir}"
            PATTERN "freedoom.ugh" EXCLUDE)
    install(DIRECTORY common ports DESTINATION "${eureka_install_dir}")
    install(FILES bindings.cfg
                  defaults.cfg
                  misc/about_logo.png
                  operations.cfg
            DESTINATION "${eureka_install_dir}"
    )
    # The full-install sequence is here
    install(CODE "execute_process(
        COMMAND xdg-desktop-menu install --novendor ${CMAKE_CURRENT_SOURCE_DIR}/misc/eureka.desktop
        COMMAND xdg-icon-resource install --novendor --size 32 ${CMAKE_CURRENT_SOURCE_DIR}/misc/eureka.xpm
    )")

    # uninstall target
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
