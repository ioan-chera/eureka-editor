set(source_mac
    AppDelegate.h
    AppDelegate.mm
    OSXCalls.h
    OSXCalls.mm
    "Eureka Doom EditorIcon.icns"
)

target_sources(eureka PRIVATE ${source_mac})

source_group(macOS FILES ${source_mac})

# May be needed by files from other projects to find headers here
target_include_directories(eureka PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Need to silence OpenGL deprecation warnings (they tell us what we know and can't fix)
target_compile_definitions(eureka PRIVATE
    "GL_SILENCE_DEPRECATION"
    "main=main_ORIGINAL"
)

set_target_properties(eureka PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Eureka Doom Editor-Info.plist.in"

    MACOSX_BUNDLE_BUNDLE_NAME "${exe_display_name}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright © The Eureka Team"
    MACOSX_BUNDLE_EXECUTABLE_NAME "${exe_display_name}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.eureka.Eureka-Doom-Editor"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "Eureka Doom EditorIcon"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)
