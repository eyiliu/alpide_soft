cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(altel VERSION 1.0.0 LANGUAGES CXX)

include(common/mysystem.cmake)


include(ExternalProject)
externalproject_add(
  extern_libwebsockets
  PREFIX ${CMAKE_BINARY_DIR}/libwebsockets_build
  INSTALL_DIR ${CMAKE_BINARY_DIR}/libwebsockets_install
  GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
  GIT_TAG v3.2.2
  GIT_PROGRESS true
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/libwebsockets_install -DLWS_WITH_THREADPOOL:BOOL=ON -DLWS_STATIC_PIC:BOOL=ON -DLWS_WITH_STATIC:BOOL=ON -DLWS_WITH_SHARED:BOOL=OFF -DLWS_WITH_SSL:BOOL=OFF
  )


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

ExternalProject_Get_Property(extern_libwebsockets INSTALL_DIR)
add_library(websockets STATIC IMPORTED)
set_target_properties(websockets PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/libwebsockets.a
    INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
    )

include_directories(${INSTALL_DIR}/include)
 
add_dependencies(websockets extern_libwebsockets)

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

install(TARGETS
  mytest
  myws
  mytele
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
