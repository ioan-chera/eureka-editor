add_library(eurekawin STATIC include/GL/glext.h include/KHR/khrplatform.h dummywin.cpp)
target_include_directories(eurekawin PUBLIC include)
set_target_properties(eurekawin PROPERTIES LINKER_LANGUAGE CXX)
