# This file is automatically generated from cmake.toml - DO NOT EDIT
# See https://github.com/build-cpp/cmkr for more information

cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

# Enable support for MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

set(CMKR_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CMKR_ROOT_PROJECT ON)

	# Bootstrap cmkr and automatically regenerate CMakeLists.txt
	include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)
	if(CMKR_INCLUDE_RESULT)
		cmkr()
	endif()

	# Enable folder support
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Create a configure-time dependency on cmake.toml to improve IDE support
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS cmake.toml)
	if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()

endif()

include("cmake/msvc-static-runtime.cmake")
include("cmake/msvc-configurations.cmake")

project(ManyTypes)

include(FetchContent)

# Fix warnings about DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
endif()
message(STATUS "Fetching x64dbg...")
FetchContent_Declare(x64dbg
	URL
		"https://sourceforge.net/projects/x64dbg/files/snapshots/snapshot_2025-03-15_15-57.zip"
)
FetchContent_MakeAvailable(x64dbg)

include("cmake/x64dbg.cmake")

# Target: libclang
add_library(libclang INTERFACE)

target_include_directories(libclang INTERFACE
	"extern/include"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x32
	target_link_libraries(libclang INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/extern/lib/x32/libclang.lib"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	target_link_libraries(libclang INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/extern/lib/x64/libclang.lib"
	)
endif()

# Target: manytypes-lib
set(manytypes-lib_SOURCES
	cmake.toml
	"manytypes-lib/include/manytypes-lib/db/clang-database.h"
	"manytypes-lib/include/manytypes-lib/db/database.h"
	"manytypes-lib/include/manytypes-lib/exceptions.h"
	"manytypes-lib/include/manytypes-lib/formatter/clang.h"
	"manytypes-lib/include/manytypes-lib/formatter/x64dbg.h"
	"manytypes-lib/include/manytypes-lib/manytypes.h"
	"manytypes-lib/include/manytypes-lib/types/alias.h"
	"manytypes-lib/include/manytypes-lib/types/basic.h"
	"manytypes-lib/include/manytypes-lib/types/enum.h"
	"manytypes-lib/include/manytypes-lib/types/function.h"
	"manytypes-lib/include/manytypes-lib/types/models/field.h"
	"manytypes-lib/include/manytypes-lib/types/models/named_sized.h"
	"manytypes-lib/include/manytypes-lib/types/structure.h"
	"manytypes-lib/include/manytypes-lib/util/clang-context.h"
	"manytypes-lib/include/manytypes-lib/util/clang-utils.h"
	"manytypes-lib/include/manytypes-lib/util/util.h"
	"manytypes-lib/src/db/database.cpp"
	"manytypes-lib/src/formatter/clang.cpp"
	"manytypes-lib/src/formatter/x64dbg.cpp"
	"manytypes-lib/src/manytypes.cpp"
	"manytypes-lib/src/types/structure.cpp"
)

add_library(manytypes-lib)

target_sources(manytypes-lib PRIVATE ${manytypes-lib_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${manytypes-lib_SOURCES})

target_compile_features(manytypes-lib PUBLIC
	cxx_std_23
)

target_include_directories(manytypes-lib PUBLIC
	"manytypes-lib/include"
)

if(NOT TARGET libclang)
	message(FATAL_ERROR "Target \"libclang\" referenced by \"manytypes-lib\" does not exist!")
endif()

target_link_libraries(manytypes-lib PUBLIC
	libclang
)

# Target: manytypes-sandbox
set(manytypes-sandbox_SOURCES
	cmake.toml
	"manytypes-sandbox/main.cpp"
)

add_executable(manytypes-sandbox)

target_sources(manytypes-sandbox PRIVATE ${manytypes-sandbox_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${manytypes-sandbox_SOURCES})

target_compile_features(manytypes-sandbox PRIVATE
	cxx_std_23
)

if(NOT TARGET manytypes-lib)
	message(FATAL_ERROR "Target \"manytypes-lib\" referenced by \"manytypes-sandbox\" does not exist!")
endif()

target_link_libraries(manytypes-sandbox PRIVATE
	manytypes-lib
)

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT manytypes-sandbox)
endif()

# Target: manytypes-plugin
set(manytypes-plugin_SOURCES
	cmake.toml
	"manytypes-plugin/.gitattributes"
	"manytypes-plugin/.gitignore"
	"manytypes-plugin/src/plugin.cpp"
	"manytypes-plugin/src/plugin.h"
	"manytypes-plugin/src/pluginmain.cpp"
	"manytypes-plugin/src/pluginmain.h"
)

x64dbg_plugin(manytypes-plugin)

target_sources(manytypes-plugin PRIVATE ${manytypes-plugin_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${manytypes-plugin_SOURCES})

target_compile_features(manytypes-plugin PUBLIC
	cxx_std_23
)

if(NOT TARGET manytypes-lib)
	message(FATAL_ERROR "Target \"manytypes-lib\" referenced by \"manytypes-plugin\" does not exist!")
endif()

target_link_libraries(manytypes-plugin PUBLIC
	manytypes-lib
)

set_target_properties(manytypes-plugin PROPERTIES
	MSVC_RUNTIME_LIBRARY
		"MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
