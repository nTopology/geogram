################################################################################
# Build Geogram as part of the current CMake project
################################################################################

<<<<<<< HEAD
cmake_minimum_required(VERSION 3.11)

if(TARGET geogram::geogram)
	return()
=======
# CMake 2.8.11 is required for 2 reasons:
# - it is the first version that fully supports the specification of Visual
# Studio toolsets (v110_xp).
# - it is the version that supports the command string(TIMESTAMP ...)
# ... set to 2.8.12 (< 2.8.12 has been deprecated)
cmake_minimum_required(VERSION 2.8.12)

# Note: geogram.cmake defines GEOGRAM_WITH_VORPALINE
# that we could have used instead,
# but geogram.cmake needs to be included after the project()
# command, since project() resets CFLAGS and CXXFLAGS.

if("$ENV{GEOGRAM_WITH_VORPALINE}" STREQUAL "")
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/src/lib/vorpalib)
        project(Vorpaline)
    else()
        project(Geogram)
    endif()
elseif ("$ENV{GEOGRAM_WITH_VORPALINE}" STREQUAL ON)
    project(Vorpaline)
else()
    project(Geogram)
endif()

# Optional modules
# (can be overriden in CMakeOptions.txt)

# Set GEOGRAM_SUB_BUILD if Geogram sources included in buildtree, then
# VORPALINE_PLATFORM can be set directly in parent CMakeLists.txt
if(NOT GEOGRAM_SUB_BUILD)
   option(GEOGRAM_WITH_GRAPHICS "Viewers and geogram_gfx library" ON)
   option(GEOGRAM_WITH_LEGACY_NUMERICS "Legacy numerical libraries" ON)
   option(GEOGRAM_WITH_HLBFGS "Non-linear solver (Yang Liu's HLBFGS)" ON)
   option(GEOGRAM_WITH_TETGEN "Tetrahedral mesher (Hang Si's TetGen)" ON)
   option(GEOGRAM_WITH_TRIANGLE "Triangle mesher (Jonathan Shewchuk's triangle)" ON)
   option(GEOGRAM_WITH_EXPLORAGRAM "Experimental code (hexahedral meshing pipeline and optimal transport)" ON)
   option(GEOGRAM_WITH_LUA "Built-in LUA interpreter" ON)
   option(GEOGRAM_LIB_ONLY "Libraries only (no example programs/no viewer)" OFF)
   option(GEOGRAM_WITH_FPG "Predicate generator (Sylvain Pion's FPG)" OFF)
   option(GEOGRAM_USE_SYSTEM_GLFW3 "Use the version of GLFW3 installed in the system if found" OFF)
   option(GEOGRAM_WITH_GARGANTUA "64-bit indices" OFF)
   include(cmake/geo_detect_platform.cmake)
endif()

include(cmake/geogram.cmake)

set(VORPALINE_VERSION_MAJOR 1)
set(VORPALINE_VERSION_MINOR 7)
set(VORPALINE_VERSION_PATCH 9)
set(VORPALINE_VERSION ${VORPALINE_VERSION_MAJOR}.${VORPALINE_VERSION_MINOR}.${VORPALINE_VERSION_PATCH})

set(VORPALINE_INCLUDE_SUBPATH geogram${VORPALINE_VERSION_MAJOR})

# Determine the current Build-OS (Build-platform without the compiler info)
string(REGEX REPLACE "-[^-]+$" "" VORPALINE_OS ${VORPALINE_PLATFORM})

# Determine the current build date
string(TIMESTAMP VORPALINE_BUILD_DATE "%Y-%m-%d %H:%M:%S")
string(TIMESTAMP YEAR "%Y")

# Determine the current build number
# This is set by Jenkins in environment variable BUILD_NUMBER
set(VORPALINE_BUILD_NUMBER $ENV{BUILD_NUMBER})

if(GEOGRAM_WITH_GARGANTUA)
  add_definitions(-DGARGANTUA)
endif()


##############################################################################
# Get SVN revision info

if(GEOGRAM_WITH_VORPALINE)
   find_package(Subversion QUIET)
   if(NOT SUBVERSION_FOUND)
       message(WARNING "Subversion executable not found - cannot determine current revision")
   else()
       Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Vorpaline)
       message(STATUS "Vorpaline revision is ${Vorpaline_WC_REVISION}")
       set(VORPALINE_SVN_REVISION ${Vorpaline_WC_REVISION})
   endif()
endif()

##############################################################################
# RPATH (where executables find the .so / DLLs)
# - Enables RPATH support for MACOSX
# - Makes RPATH of dynamic libraries and executable point to the directory
#   where libraries are installed.

if(VORPALINE_BUILD_DYNAMIC)
   set(CMAKE_MACOSX_RPATH 1)
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# uninstall target
# Needs to be created before add_subdirectory() because GLFw has
# also an uninstall target that will be inhibited if there is
# already one (Geogram's one needs to be first)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

##############################################################################
# Geogram/Vorpaline sources

add_subdirectory(src/lib/geogram)
if(GEOGRAM_WITH_VORPALINE)
   add_subdirectory(src/lib/vorpalib)
endif()

add_subdirectory(src/lib/third_party)

if(GEOGRAM_WITH_GRAPHICS)
  add_subdirectory(src/lib/geogram_gfx)
endif()

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/exploragram)
  if(GEOGRAM_WITH_EXPLORAGRAM)
     add_subdirectory(src/lib/exploragram)
  endif()
endif()

if(NOT GEOGRAM_LIB_ONLY)
  add_subdirectory(src/bin)
  add_subdirectory(src/tests)
  add_subdirectory(src/examples)
  add_subdirectory(tests)
>>>>>>> origin/master
endif()

set(GEOGRAM_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_path(GEOGRAM_SOURCE_INCLUDE_DIR
		geogram/basic/common.h
		PATHS ${GEOGRAM_ROOT}
		PATH_SUFFIXES src/lib
		NO_DEFAULT_PATH
)

################################################################################

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(VORPALINE_ARCH_64 TRUE CACHE BOOL "" FORCE)
	set(VORPALINE_PLATFORM Win-vs-generic CACHE STRING "" FORCE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(VORPALINE_PLATFORM Linux64-gcc CACHE STRING "" FORCE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(VORPALINE_PLATFORM Darwin-clang CACHE STRING "" FORCE)
endif()

<<<<<<< HEAD
option(GEOGRAM_WITH_GRAPHICS        "Viewers and geogram_gfx library"                                         ON)
option(GEOGRAM_WITH_LEGACY_NUMERICS "Legacy numerical libraries"                                             OFF)
option(GEOGRAM_WITH_HLBFGS          "Non-linear solver (Yang Liu's HLBFGS)"                                   ON)
option(GEOGRAM_WITH_TETGEN          "Tetrahedral mesher (Hang Si's TetGen)"                                  OFF)
option(GEOGRAM_WITH_TRIANGLE        "Triangle mesher (Jonathan Shewchuk's triangle)"                         OFF)
option(GEOGRAM_WITH_EXPLORAGRAM     "Experimental code (hexahedral meshing vpipeline and optimal transport)" OFF)
option(GEOGRAM_WITH_LUA             "Built-in LUA interpreter"                                               OFF)
option(GEOGRAM_LIB_ONLY             "Libraries only (no example programs/no viewer)"                          ON)
option(GEOGRAM_WITH_FPG             "Predicate generator (Sylvain Pion's FPG)"                               OFF)
option(GEOGRAM_USE_SYSTEM_GLFW3     "Use the version of GLFW3 installed in the system if found"              OFF)

################################################################################

include(GeogramMain.cmake)
add_library(geogram::geogram ALIAS geogram)
add_library(geogram::gfx ALIAS geogram_gfx)
target_include_directories(geogram SYSTEM PUBLIC ${GEOGRAM_SOURCE_INCLUDE_DIR})

################################################################################

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# remove warning for multiply defined symbols (caused by multiple
	# instanciations of STL templates)
	# target_compile_options(geogram INTERFACE /wd4251)

	# remove all unused stuff from windows.h
	# target_compile_definitions(geogram INTERFACE -DWIN32_LEAN_AND_MEAN)
	# target_compile_definitions(geogram INTERFACE -DVC_EXTRALEAN)

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
=======
if(NOT DEFINED CPACK_GENERATOR)
  if(WIN32)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_GENERATOR TGZ)
  endif()
>>>>>>> origin/master
endif()
