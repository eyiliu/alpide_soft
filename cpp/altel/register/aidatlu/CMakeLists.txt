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
  boost_chrono
  boost_filesystem
  boost_program_options
  boost_unit_test_framework
  boost_regex
  boost_system
  boost_thread
  rt
  pthread
  )

set(AIDA_TLU_REG_XML ${CMAKE_SOURCE_DIR}/doc/aida_tlu_address-fw_version_14.xml)
set(AIDA_TLU_REG_HH  ${CMAKE_BINARY_DIR}/aida_tlu_address-fw_version_14.hh)
if( (NOT EXISTS ${AIDA_TLU_REG_HH}) OR (${AIDA_TLU_REG_XML} IS_NEWER_THAN ${AIDA_TLU_REG_HH}) )
  set(AIDA_TLU_REG_HH_DELIM "~~~cpp~raw~~~")
  file(READ ${AIDA_TLU_REG_XML} AIDA_TLU_REG_XML_CONTENT)
  set(AIDA_TLU_REG_XML_CONTENT "R\"${AIDA_TLU_REG_HH_DELIM}(\n${AIDA_TLU_REG_XML_CONTENT})${AIDA_TLU_REG_HH_DELIM}\"")
  file(WRITE ${AIDA_TLU_REG_HH} "${AIDA_TLU_REG_XML_CONTENT}")
endif()

set(AIDA_TLU_CLK_TXT ${CMAKE_SOURCE_DIR}/doc/aida_tlu_clk_config.txt)
set(AIDA_TLU_CLK_HH  ${CMAKE_BINARY_DIR}/aida_tlu_clk_config.hh)
if( (NOT EXISTS ${AIDA_TLU_CLK_HH}) OR (${AIDA_TLU_CLK_TXT} IS_NEWER_THAN ${AIDA_TLU_CLK_HH}) )
  set(AIDA_TLU_CLK_HH_DELIM "~~~cpp~raw~~~")
  file(READ ${AIDA_TLU_CLK_TXT} AIDA_TLU_CLK_TXT_CONTENT)
  set(AIDA_TLU_CLK_TXT_CONTENT "R\"${AIDA_TLU_CLK_HH_DELIM}(\n${AIDA_TLU_CLK_TXT_CONTENT})${AIDA_TLU_CLK_HH_DELIM}\"")
  file(WRITE ${AIDA_TLU_CLK_HH} "${AIDA_TLU_CLK_TXT_CONTENT}")
endif()



include_directories(${CACTUS_INCLUDE_DIR})
include_directories(include)
include_directories(${CMAKE_BINARY_DIR})
aux_source_directory(src LIB_SRC)
add_library(aidatlu STATIC ${LIB_SRC})

#target_link_libraries(aidatlu PRIVATE -Wl,--whole-archive ${CACTUS_LIBRARIES})

target_link_libraries(aidatlu PRIVATE -Wl,--whole-archive ${CACTUS_LIBRARIES} -Wl,--no-whole-archive)

#set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH ${CACTUS_LIBRARY_DIR})

add_subdirectory(exe)

