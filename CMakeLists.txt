project(alpidedaq)
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
include(CMakeDependentOption)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(ALPIDE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR} CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX ${ALPIDE_INSTALL_PREFIX} CACHE INTERNAL "Prefix of install directories" FORCE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
endif()
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)



if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  #set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

  add_definitions("/wd4251")
  add_definitions("/wd4996")
  add_definitions("/wd4251")

  if(NOT MSVC)
    message(FATAL_ERROR "only VC++ is support on Windows")
  endif()
elseif(APPLE)
  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    message(FATEL ERROR "only Clang is support on MacOS")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-undefined,error")
else()
  if(NOT(CMAKE_COMPILER_IS_GNUCC))
    message(FATEL ERROR "only GCC is support On Linux")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif()

add_subdirectory(cpp)
