
include(ExternalProject)
externalproject_add(
  extern_libwebsockets
  PREFIX ${CMAKE_BINARY_DIR}/extern_libwebsockets
  INSTALL_DIR ${CMAKE_BINARY_DIR}/extern_libwebsockets/INSTALL
  GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
  GIT_TAG v3.2.2
  GIT_PROGRESS true
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/extern_libwebsockets/INSTALL -DLWS_WITH_THREADPOOL:BOOL=ON -DLWS_STATIC_PIC:BOOL=ON -DLWS_WITH_STATIC:BOOL=ON -DLWS_WITH_SHARED:BOOL=OFF -DLWS_WITH_SSL:BOOL=OFF
  )

ExternalProject_Get_Property(extern_libwebsockets INSTALL_DIR)

add_library(websockets INTERFACE)
target_include_directories(websockets INTERFACE ${INSTALL_DIR}/include)
target_link_libraries(websockets INTERFACE ${INSTALL_DIR}/lib/libwebsockets.a)
add_dependencies(websockets extern_libwebsockets)

add_executable(myws websockets_main.cc)
target_link_libraries(myws PRIVATE mycommon websockets telescope)

install(TARGETS
  myws
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
