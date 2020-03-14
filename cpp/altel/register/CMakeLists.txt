cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(altel VERSION 1.0.0 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/INSTALL CACHE PATH "Default prefix of install directories" FORCE)
endif()

include(common/mysystem.cmake)
include(ExternalProject)

set(EXTERN_PROJECT_PREFIX ${CMAKE_BINARY_DIR}/externproject)

set(EUDAQ_CMAKE_INSTALL_PREFIX  ${CMAKE_INSTALL_PREFIX})
externalproject_add(
  extern_eudaq
  PREFIX ${EXTERN_PROJECT_PREFIX}/eudaq
  INSTALL_DIR ${EUDAQ_CMAKE_INSTALL_PREFIX}
  GIT_REPOSITORY https://github.com/eudaq/eudaq.git
  GIT_TAG v2.3.0
  GIT_PROGRESS true
  CMAKE_ARGS -DEUDAQ_INSTALL_PREFIX:PATH=${EUDAQ_CMAKE_INSTALL_PREFIX} -DUSER_TLU_BUILD:BOOL=ON -DUSER_EUDET_BUILD:BOOL=ON -DUSER_EXAMPLE_BUILD:BOOL=ON -DEUDAQ_BUILD_STDEVENT_MONITOR:BOOL=ON
  )

set(LIBWEBSOCKETS_CMAKE_INSTALL_PREFIX ${EXTERN_PROJECT_PREFIX})
externalproject_add(
  extern_libwebsockets
  PREFIX ${EXTERN_PROJECT_PREFIX}/libwebsockets
  INSTALL_DIR ${LIBWEBSOCKETS_CMAKE_INSTALL_PREFIX}
  GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
  GIT_TAG v3.2.2
  GIT_PROGRESS true
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${LIBWEBSOCKETS_CMAKE_INSTALL_PREFIX} -DLWS_WITH_THREADPOOL:BOOL=ON -DLWS_STATIC_PIC:BOOL=ON -DLWS_WITH_STATIC:BOOL=ON -DLWS_WITH_SHARED:BOOL=OFF -DLWS_WITH_SSL:BOOL=OFF
  )

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

add_library(websockets INTERFACE)
ExternalProject_Get_Property(extern_libwebsockets INSTALL_DIR)
target_include_directories(websockets INTERFACE ${INSTALL_DIR}/include)
target_link_libraries(websockets INTERFACE ${INSTALL_DIR}/lib/libwebsockets.a)
add_dependencies(websockets extern_libwebsockets extern_eudaq)

include_directories(${INSTALL_DIR}/include)

add_subdirectory(common)
add_subdirectory(rbcp)

add_executable(mytest FirmwarePortal.cc FirmwarePortal_main.cc)
target_include_directories(mytest BEFORE PRIVATE common rbcp)
target_link_libraries(mytest PRIVATE mycommon rbcp stdc++fs)

find_package(Threads REQUIRED)
add_executable(myws FirmwarePortal.cc Telescope.cc websockets_main.cc JadeDataFrame.cc AltelReader.cc base64.cpp)
target_include_directories(myws BEFORE PRIVATE common rbcp )
target_link_libraries(myws PRIVATE mycommon rbcp websockets Threads::Threads stdc++fs)

add_executable(mytele FirmwarePortal.cc Telescope.cc telescope_main.cc JadeDataFrame.cc AltelReader.cc base64.cpp)
target_include_directories(mytele BEFORE PRIVATE common rbcp )
target_link_libraries(mytele PRIVATE mycommon rbcp Threads::Threads stdc++fs)


add_executable(mygl FirmwarePortal.cc Telescope.cc opengl_main.cc JadeDataFrame.cc AltelReader.cc base64.cpp)
target_include_directories(mygl BEFORE PRIVATE common rbcp )
target_link_libraries(mygl PRIVATE mycommon rbcp Threads::Threads stdc++fs m glut GL GLU)

install(TARGETS
  mytest
  myws
  mytele
  mygl
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
