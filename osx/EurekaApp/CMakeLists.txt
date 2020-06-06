set(source_mac
    AppDelegate.h
    AppDelegate.mm
    OSXCalls.h
    OSXCalls.mm
)

target_sources(${executable_name} PRIVATE ${source_mac})

source_group(macOS FILES ${source_mac})

# May be needed by files from other projects to find headers here
target_include_directories(${executable_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Need to silence OpenGL deprecation warnings (they tell us what we know and can't fix)
target_compile_definitions(${executable_name} PRIVATE
    "GL_SILENCE_DEPRECATION"
    "main=main_ORIGINAL"
)
