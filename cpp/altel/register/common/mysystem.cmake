include(CMakeDependentOption)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
endif()
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

  add_definitions("/wd4251")
  add_definitions("/wd4996")
  add_definitions("/wd4251")

  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "MSVC"))
    message(FATAL_ERROR "only VC++ is support on Windows")
  endif()
elseif(APPLE)
  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang"))
    message(FATEL ERROR "only AppleClang is support on MacOS")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-undefined,error")
else()
  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
    message(FATEL ERROR "only GCC is support On Linux")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif()
