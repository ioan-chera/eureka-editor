set(resource_mac "${CMAKE_CURRENT_SOURCE_DIR}/Eureka Doom EditorIcon.icns" CACHE FILEPATH "")
set(source_mac
    AppDelegate.h
    AppDelegate.mm
    OSXCalls.h
    OSXCalls.mm
)

add_library(eurekamac STATIC ${source_mac})

# Needed for macOS release archiving!
set_target_properties(eurekamac PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)

target_include_directories(eurekamac INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# Need to silence OpenGL deprecation warnings (they tell us what we know and can't fix)
target_compile_definitions(eurekamac PUBLIC
    "GL_SILENCE_DEPRECATION"
)
