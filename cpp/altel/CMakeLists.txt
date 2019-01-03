include_directories(include)

include_directories(../jade/include)
add_definitions("-DJADE_DLL_EXPORT")

aux_source_directory(src ALTEL_SRC)
set(ALTEL_LIBRARY altel CACHE INTERNAL "alpide telescope library")
add_library(${ALTEL_LIBRARY} SHARED ${ALTEL_SRC})
target_link_libraries(${ALTEL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${JADE_CORE_LIBRARY})

add_subdirectory(exe)

install(TARGETS
  ${ALTEL_LIBRARY}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
