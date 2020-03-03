cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(aidatlu VERSION 1.0.0 LANGUAGES CXX)

#list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
#find_package(PkgConfig)
#find_package(CACTUS)

set(CACTUS_INCLUDE_DIR /opt/cactus/include)
set(CACTUS_LIBRARIES
  /opt/cactus/lib/libcactus_uhal_grammars.a
  /opt/cactus/lib/libcactus_uhal_log.a
  /opt/cactus/lib/libcactus_uhal_tests.a
  /opt/cactus/lib/libcactus_uhal_uhal.a
  )

include_directories(${CACTUS_INCLUDE_DIR})
include_directories(include)
aux_source_directory(src LIB_SRC)
add_library(aidatlu SHARED ${LIB_SRC})

target_link_libraries(aidatlu PRIVATE ${CACTUS_LIBRARIES})

#set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH ${CACTUS_LIBRARY_DIR})
