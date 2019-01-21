include_directories(../jade/include)
add_subdirectory(exe)

install(TARGETS
  ${ALTEL_LIBRARY}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
