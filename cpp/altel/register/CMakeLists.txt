cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(altel VERSION 1.0.0 LANGUAGES CXX)

include(common/mysystem.cmake)

add_subdirectory(common)
add_subdirectory(rbcp)

add_executable(mytest FirmwarePortal.cc FirmwarePortal_main.cc)
target_include_directories(mytest BEFORE PRIVATE common rbcp)
target_link_libraries(mytest PRIVATE mycommon rbcp stdc++fs)


find_package(Threads REQUIRED)
add_executable(myws FirmwarePortal.cc websockets_main.cc JadeDataFrame.cc AltelReader.cc base64.cpp)
target_include_directories(myws BEFORE PRIVATE common rbcp )
target_link_libraries(myws PRIVATE mycommon rbcp websockets Threads::Threads stdc++fs)

add_executable(mytele FirmwarePortal.cc telescope_main.cc JadeDataFrame.cc AltelReader.cc base64.cpp)
target_include_directories(mytele BEFORE PRIVATE common rbcp )
target_link_libraries(mytele PRIVATE mycommon rbcp websockets Threads::Threads stdc++fs)

install(TARGETS
  mytest
  myws
  mytele
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
