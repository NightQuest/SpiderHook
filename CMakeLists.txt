cmake_minimum_required(VERSION 3.24)
project(SpiderHook)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)

include_directories(src)
include_directories(dep/capstone/include)

set(base_files
		src/Application.cpp
		src/Application.h
		src/SpiderHook.cpp
		src/SpiderHook.h
		src/SpiderHook.def
		src/preCompiled.cpp
		src/preCompiled.h)

set(engine_files
		src/Engine/Engine.cpp
		src/Engine/Engine.h)

set(source_files
		${base_files}
		${engine_files})

# Set a group for Visual Studio
source_group("" FILES ${base_files})
source_group("Engine" FILES ${engine_files})

add_library(SpiderHook MODULE ${source_files})

# Make sure the output filename is faultrep.dll
set_target_properties(SpiderHook PROPERTIES OUTPUT_NAME "faultrep")

# Add capstone
add_subdirectory(dep/capstone)

target_link_libraries(SpiderHook PRIVATE capstone Comctl32.lib dwmapi.lib)

target_precompile_headers(SpiderHook PUBLIC src/preCompiled.h)

install(TARGETS SpiderHook LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
