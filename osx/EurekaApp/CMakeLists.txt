set(resource_mac "${CMAKE_CURRENT_SOURCE_DIR}/Eureka Doom EditorIcon.icns" CACHE FILEPATH "")
set(source_mac
    AppDelegate.h
    AppDelegate.mm
    OSXCalls.h
    OSXCalls.mm
)

add_library(macspecial STATIC ${source_mac})

target_include_directories(macspecial INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# Need to silence OpenGL deprecation warnings (they tell us what we know and can't fix)
target_compile_definitions(macspecial PUBLIC
    "GL_SILENCE_DEPRECATION"
    "main=main_ORIGINAL"
)
