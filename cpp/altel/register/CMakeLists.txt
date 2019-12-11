cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(altel VERSION 1.0.0 LANGUAGES CXX)

include(common/mysystem.cmake)

add_subdirectory(common)
add_subdirectory(rbcp)

add_executable(mytest FirmwarePortal.cc FirmwarePortal_main.cc)
target_include_directories(mytest BEFORE PRIVATE common rbcp)
target_link_libraries(mytest PRIVATE mycommon rbcp)

install(TARGETS
  mytest
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
