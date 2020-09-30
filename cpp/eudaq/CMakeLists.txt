option(JADE_BUILD_EUDAQ_PRODUCER "build JadeProducer against EUDAQ2?" OFF)
if(NOT JADE_BUILD_EUDAQ_PRODUCER)
  return()
endif()

include_directories(../jade/include)
include_directories(include)

aux_source_directory(src MODULE_SRC)

find_package(eudaq REQUIRED
  COMPONENTS eudaq::core
  OPTIONAL_COMPONENTS eudaq::lcio
  CONFIG
  PATHS ${CMAKE_INSTALL_PREFIX}
  NO_DEFAULT_PATH
  )

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel")
endif()

add_library(eudaq_module_altel SHARED ${MODULE_SRC})
if(TARGET eudaq::lcio)
  set(EUDAQ_LIBRARIES eudaq::core eudaq::lcio)
else()
  set(EUDAQ_LIBRARIES eudaq::core)
endif()


set(EUDAQ_MODULE eudaq_module_jade)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_LIBRARIES} ${JADE_CORE_LIBRARY})
set_property( TARGET ${EUDAQ_MODULE} APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})

add_subdirectory(exe)

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
