option(ALPIDE_BUILD_EUDAQ_PRODUCER "build JadeProducer against EUDAQ2?" ON)
if(NOT ALPIDE_BUILD_EUDAQ_PRODUCER)
  return()
endif()


message("")
message("")
message(STATUS "option ALPIDE_BUILD_EUDAQ_PRODUCER is ON. It is set to build against eudaq")
message(STATUS "In case to build againt to your own EUDAQ installation, please set eudaq_DIR to the path of that installation's cmake folder")
message("")
message("")

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

include_directories(${EUDAQ_INCLUDE_DIRS})
if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_altel")
endif()

add_library(eudaq_module_alpide_jade SHARED ${MODULE_SRC})
if(TARGET eudaq::lcio)
  set(EUDAQ_LIBRARIES eudaq::core eudaq::lcio)
  get_target_property(MY_EUDAQ_LCIO_INCLUDE_DIRS  eudaq::lcio INTERFACE_INCLUDE_DIRECTORIES)
  message(${MY_EUDAQ_LCIO_INCLUDE_DIRS})
else()
  set(EUDAQ_LIBRARIES eudaq::core)
endif()

target_link_libraries(eudaq_module_alpide_jade ${EUDAQ_LIBRARIES} ${JADE_CORE_LIBRARY})
set_property(TARGET eudaq_module_alpide_jade APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})

add_subdirectory(exe)

install(TARGETS
  eudaq_module_alpide_jade
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
