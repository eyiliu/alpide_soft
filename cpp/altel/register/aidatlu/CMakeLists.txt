cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(aidatlu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# set(CMAKE_LINK_SEARCH_START_STATIC ON)
# set(CMAKE_LINK_SEARCH_END_STATIC ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_STATIC_LIBS ON) 
find_package(Boost REQUIRED COMPONENTS
  system thread filesystem chrono regex program_options unit_test_framework
  )

add_library(cactus_uhal_grammars STATIC IMPORTED)
set_target_properties(cactus_uhal_grammars PROPERTIES
  IMPORTED_LOCATION  /opt/cactus/lib/libcactus_uhal_grammars.a
  INTERFACE_INCLUDE_DIRECTORIES /opt/cactus/include
  )
add_library(cactus_uhal_log STATIC IMPORTED)
set_target_properties(cactus_uhal_log PROPERTIES
  IMPORTED_LOCATION  /opt/cactus/lib/libcactus_uhal_log.a
  INTERFACE_INCLUDE_DIRECTORIES /opt/cactus/include
  )
add_library(cactus_uhal_tests STATIC IMPORTED)
set_target_properties(cactus_uhal_tests PROPERTIES
  IMPORTED_LOCATION  /opt/cactus/lib/libcactus_uhal_tests.a
  INTERFACE_INCLUDE_DIRECTORIES /opt/cactus/include
  )
add_library(cactus_uhal_uhal STATIC IMPORTED)
set_target_properties(cactus_uhal_uhal PROPERTIES
  IMPORTED_LOCATION  /opt/cactus/lib/libcactus_uhal_uhal.a
  INTERFACE_INCLUDE_DIRECTORIES /opt/cactus/include
  )

add_library(cactus INTERFACE)
target_link_libraries(cactus INTERFACE
  cactus_uhal_uhal cactus_uhal_grammars cactus_uhal_tests cactus_uhal_log
  Boost::chrono Boost::filesystem Boost::program_options Boost::unit_test_framework Boost::regex Boost::system Boost::thread
  rt Threads::Threads)
target_include_directories(cactus INTERFACE /opt/cactus/include)


set(AIDA_TLU_REG_XML ${CMAKE_SOURCE_DIR}/doc/aida_tlu_address-fw_version_14.xml)
set(AIDA_TLU_REG_HH  ${CMAKE_BINARY_DIR}/aida_tlu_address-fw_version_14.hh)
if( (NOT EXISTS ${AIDA_TLU_REG_HH}) OR (${AIDA_TLU_REG_XML} IS_NEWER_THAN ${AIDA_TLU_REG_HH}) )
  set(AIDA_TLU_REG_HH_DELIM "~~~cpp~raw~~~")
  file(READ ${AIDA_TLU_REG_XML} AIDA_TLU_REG_XML_CONTENT)
  set(AIDA_TLU_REG_XML_CONTENT "R\"${AIDA_TLU_REG_HH_DELIM}(${AIDA_TLU_REG_XML_CONTENT})${AIDA_TLU_REG_HH_DELIM}\"")
  file(WRITE ${AIDA_TLU_REG_HH} "${AIDA_TLU_REG_XML_CONTENT}")
endif()

set(AIDA_TLU_CLK_TXT ${CMAKE_SOURCE_DIR}/doc/aida_tlu_clk_config.txt)
set(AIDA_TLU_CLK_HH  ${CMAKE_BINARY_DIR}/aida_tlu_clk_config.hh)
if( (NOT EXISTS ${AIDA_TLU_CLK_HH}) OR (${AIDA_TLU_CLK_TXT} IS_NEWER_THAN ${AIDA_TLU_CLK_HH}) )
  set(AIDA_TLU_CLK_HH_DELIM "~~~cpp~raw~~~")
  file(READ ${AIDA_TLU_CLK_TXT} AIDA_TLU_CLK_TXT_CONTENT)
  set(AIDA_TLU_CLK_TXT_CONTENT "R\"${AIDA_TLU_CLK_HH_DELIM}(${AIDA_TLU_CLK_TXT_CONTENT})${AIDA_TLU_CLK_HH_DELIM}\"")
  file(WRITE ${AIDA_TLU_CLK_HH} "${AIDA_TLU_CLK_TXT_CONTENT}")
endif()

include_directories(include)
include_directories(${CMAKE_BINARY_DIR})
aux_source_directory(src LIB_SRC)
add_library(aidatlu STATIC ${LIB_SRC})
target_link_libraries(aidatlu PRIVATE cactus)

#target_link_libraries(aidatlu PRIVATE -Wl,--whole-archive ${CACTUS_LIBRARIES} -Wl,--no-whole-archive)
#set_target_properties(aidatlu PROPERTIES INSTALL_RPATH /opt/cactus/lib)

add_subdirectory(exe)

install(TARGETS
  aidatlu
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


