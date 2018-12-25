include_directories(./)

set(RBCP_LIBRARY_SRC rbcp_com.c)
set(RBCP_LIBRARY rbcp)
add_library(${RBCP_LIBRARY} SHARED ${RBCP_LIBRARY_SRC})
target_link_libraries(${RBCP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

set(RBCP_WIN_EXE_SRC rbcp.c rbcp_com.c)
set(RBCP_WIN_EXE rbcpWin)
add_executable(${RBCP_WIN_EXE} ${RBCP_WIN_EXE_SRC})
target_link_libraries(${RBCP_WIN_EXE} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

install(TARGETS
  ${RBCP_LIBRARY}
  ${RBCP_WIN_EXE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
