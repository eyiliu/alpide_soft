add_subdirectory(linenoiseng)
add_subdirectory(getopt)
add_subdirectory(rapidjson)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(mycommon INTERFACE)

target_link_libraries(mycommon INTERFACE
  linenoiseng
  getopt
  rapidjson
  Threads::Threads
  )

target_include_directories(mycommon
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

set(LIB_PUBLIC_HEADERS mycommon.hh)
set_target_properties(mycommon PROPERTIES PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}")
