# SimpleGameTemplate cmake build script.
cmake_minimum_required(VERSION 2.8)
project(SimpleGameTemplate)

# Check the build type
if (CMAKE_BUILD_TYPE STREQUAL "")
	# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
	# differentiation between debug and release builds.
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Special checks for Emscripten
if(${CMAKE_SYSTEM_NAME} STREQUAL Emscripten)
    set(ON_EMSCRIPTEN TRUE)
endif()

# Output paths.
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(SimpleGameTemplate_DATA_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist/data")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

# Turn warnings and use C++ 2011.
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	# using Visual Studio C++
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
	if(UNIX)
		if(NOT ON_EMSCRIPTEN)
        	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
		endif()
    endif()

	# Export symbols from applications.
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
endif()

# Perform platform checks
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)

# Set output dir.
set(EXECUTABLE_OUTPUT_PATH "${SimpleGameTemplate_BINARY_DIR}/dist")
set(LIBRARY_OUTPUT_PATH "${SimpleGameTemplate_BINARY_DIR}/dist")

option(LIVE_CODING_SUPPORT True "Build with live coding support")

# Use pkg-config.
find_package(PkgConfig)
if(ON_EMSCRIPTEN)
    # Use SDL2 port instead of SDL1
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"png\"]' -s USE_SDL_MIXER=2 -s TOTAL_MEMORY=50331648 -Wno-warn-absolute-paths")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"png\"]' -s USE_SDL_MIXER=2 -s TOTAL_MEMORY=50331648 -Wno-warn-absolute-paths")

    set(SDL2_LIBRARIES)
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(SimpleGameTemplate_DEP_LIBS ${SDL2_LIBRARIES} ${SimpleGameTemplate_DEP_LIBS})
    set(ASSET_FLAGS "--preload-file ${CMAKE_SOURCE_DIR}/assets@assets")

	set(LIVE_CODING_SUPPORT False)
	#add_definitions(-DNO_SDL_MIXER_AVAILABLE)
else()
    # For VisualStudio or mingw in Window
    find_path(SDL2_INCLUDE_DIR
      NAMES SDL.h
      PATH_SUFFIXES SDL2
    )

    find_path(SDL2_LIBRARY_PATH
      NAMES SDL2.lib SDL2.a libSDL2.a
    )

    find_library(SDL2_LIBRARY NAMES SDL2-2.0 SDL2 PATHS ${SDL2_LIBRARY_PATH})
    find_library(SDL2_IMAGE_LIBRARY NAMES SDL2_image-2.0 SDL2_image PATHS ${SDL2_LIBRARY_PATH})
	find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer-2.0 SDL2_mixer PATHS ${SDL2_LIBRARY_PATH})
    if(WIN32)
		find_library(SDL2_MAIN_LIBRARY NAMES SDL2main PATHS ${SDL2_LIBRARY_PATH})
    else()
		set(SDL2_MAIN_LIBRARY "")
		set(SimpleGameTemplate_DEP_LIBS "dl")
    endif()

    set(SimpleGameTemplate_DEP_LIBS ${SimpleGameTemplate_DEP_LIBS} ${SDL2_MAIN_LIBRARY} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_MIXER_LIBRARY})
    if(MINGW)
        set(SimpleGameTemplate_DEP_LIBS mingw32 ${SimpleGameTemplate_DEP_LIBS})
	add_definitions(-D__NO_INLINE__)
    endif()

    include_directories("${SDL2_INCLUDE_DIR}")
    set(ASSET_FLAGS "")
endif()

# Add the current directory.
include_directories(
.
"${PROJECT_SOURCE_DIR}/include"
)

# Build the program
add_subdirectory(src)
