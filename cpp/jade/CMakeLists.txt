include_directories(include)
add_definitions("-DJADE_DLL_EXPORT")
add_definitions(-DBUILDING_WINGETOPT_DLL -DWINGETOPT_SHARED_LIB)

aux_source_directory(src JADE_CORE_SRC)
set(JADE_CORE_LIBRARY jade CACHE INTERNAL "Jade core library")
add_library(${JADE_CORE_LIBRARY} SHARED ${JADE_CORE_SRC})
#add_library(${JADE_CORE_LIBRARY}_static STATIC ${JADE_CORE_SRC})

target_link_libraries(${JADE_CORE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
#target_link_libraries(${JADE_CORE_LIBRARY}_static ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

# add_subdirectory(exe)

install(TARGETS
  ${JADE_CORE_LIBRARY}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
