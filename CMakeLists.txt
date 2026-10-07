################################################################################
# nTop wrapper: build Geogram as part of the current CMake project
#
# Usable through add_subdirectory() (turbo) or as a standalone top-level
# project.  The upstream root file is kept as GeogramMain.cmake.
################################################################################

cmake_minimum_required(VERSION 3.11)

if(TARGET geogram::geogram)
	return()
endif()

set(GEOGRAM_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_path(GEOGRAM_SOURCE_INCLUDE_DIR
		geogram/basic/common.h
		PATHS ${GEOGRAM_ROOT}
		PATH_SUFFIXES src/lib
		NO_DEFAULT_PATH
)

################################################################################

set(_geogram_os "${CMAKE_SYSTEM_NAME}")
if(NOT _geogram_os)
	# standalone top-level configure: project() has not been called yet
	set(_geogram_os "${CMAKE_HOST_SYSTEM_NAME}")
endif()

# Static libraries on all platforms
if(${_geogram_os} MATCHES "Windows")
	set(VORPALINE_ARCH_64 TRUE CACHE BOOL "" FORCE)
	set(VORPALINE_PLATFORM Win-vs-generic CACHE STRING "" FORCE)
elseif(${_geogram_os} MATCHES "Linux")
	set(VORPALINE_PLATFORM Linux64-gcc CACHE STRING "" FORCE)
elseif(${_geogram_os} MATCHES "Darwin")
	set(VORPALINE_PLATFORM Darwin-clang CACHE STRING "" FORCE)
endif()

option(GEOGRAM_WITH_GRAPHICS        "Viewers and geogram_gfx library"                                        OFF)
option(GEOGRAM_WITH_LEGACY_NUMERICS "Legacy numerical libraries"                                             OFF)
option(GEOGRAM_WITH_HLBFGS          "Non-linear solver (Yang Liu's HLBFGS)"                                   ON)
option(GEOGRAM_WITH_TETGEN          "Tetrahedral mesher (Hang Si's TetGen)"                                  OFF)
option(GEOGRAM_WITH_TRIANGLE        "Triangle mesher (Jonathan Shewchuk's triangle)"                         OFF)
option(GEOGRAM_WITH_EXPLORAGRAM     "Experimental code (hexahedral meshing pipeline and optimal transport)" OFF)
option(GEOGRAM_WITH_LUA             "Built-in LUA interpreter"                                               OFF)
option(GEOGRAM_LIB_ONLY             "Libraries only (no example programs/no viewer)"                          ON)
option(GEOGRAM_WITH_FPG             "Predicate generator (Sylvain Pion's FPG)"                               OFF)
option(GEOGRAM_USE_SYSTEM_GLFW3     "Use the version of GLFW3 installed in the system if found"              OFF)
option(GEOGRAM_WITH_GARGANTUA       "64-bit indices"                                                         OFF)
# TBB drives the parallel_for / spinlock implementation (GEO_TBB)
option(GEOGRAM_WITH_TBB             "Use TBB for multi-threading"                                             ON)
# zlib supplied by the parent project (turbo: target zlibstatic) instead of
# the copy bundled with geogram
option(GEOGRAM_USE_EXTERNAL_ZLIB    "Link an externally provided zlib target"                                 OFF)
set(GEOGRAM_EXTERNAL_ZLIB_TARGET zlibstatic CACHE STRING "zlib target used when GEOGRAM_USE_EXTERNAL_ZLIB is ON")

################################################################################
# TBB: use the parent project's target if there is one (turbo provides
# tbb::tbb), expose it under the name upstream expects (TBB::tbb).
# Otherwise upstream's cmake/onetbb.cmake fetches oneTBB.

if(GEOGRAM_WITH_TBB AND NOT TARGET TBB::tbb)
	if(COMMAND ntop_import)
		ntop_import(tbb)
	endif()
	if(TARGET tbb::tbb)
		get_target_property(_geogram_tbb_aliased tbb::tbb ALIASED_TARGET)
		if(NOT _geogram_tbb_aliased)
			set(_geogram_tbb_aliased tbb::tbb)
		endif()
		add_library(TBB::tbb ALIAS ${_geogram_tbb_aliased})
	endif()
endif()

################################################################################

include(GeogramMain.cmake)
add_library(geogram::geogram ALIAS geogram)
target_include_directories(geogram SYSTEM PUBLIC ${GEOGRAM_SOURCE_INCLUDE_DIR})

################################################################################

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# do not define a min() and a max() macro, breaks
	# std::min() and std::max() !!
	target_compile_definitions(geogram INTERFACE -DNOMINMAX)

	# we want M_PI etc...
	target_compile_definitions(geogram INTERFACE -D_USE_MATH_DEFINES)

	if(NOT VORPALINE_BUILD_DYNAMIC)
		# If we use static library, we link with the static C++ runtime.
		foreach(config ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${config} config)
			string(REPLACE /MD /MT CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_${config}}")
			string(REPLACE /MD /MT CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
		endforeach()
	endif()
endif()
