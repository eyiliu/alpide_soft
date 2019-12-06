option(JADE_BUILD_EUDAQ_PRODUCER "build JadeProducer against EUDAQ2?" OFF)
if(NOT JADE_BUILD_EUDAQ_PRODUCER)
  return()
endif()

include_directories(../jade/include)
include_directories(include)

aux_source_directory(src MODULE_SRC)

find_package(eudaq CONFIG 2.4 REQUIRED NO_DEFAULT_PATH)
include_directories(${EUDAQ_INCLUDE_DIRS})
add_subdirectory(exe)

list(FIND EUDAQ_LIBRARIES "eudaq_lcio" LCIO_FIND_POS)
if(${LCIO_FIND_POS} EQUAL -1)
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")
endif()


set(EUDAQ_MODULE eudaq_module_jade)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ2_LIBRARY} eudaq::core ${JADE_CORE_LIBRARY})
set_property( TARGET ${EUDAQ_MODULE} APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

