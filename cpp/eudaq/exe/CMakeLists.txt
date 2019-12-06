macro(add_my_eudaq_exe EXE_TARGET SRC_FILE)
  add_executable(${EXE_TARGET} ${SRC_FILE})
  target_link_libraries(${EXE_TARGET} ${EUDAQ2_LIBRARY} eudaq::core)
  list(APPEND INSTALL_TARGETS ${EXE_TARGET})
endmacro()

add_my_eudaq_exe(alpide_noise_ratio alpide_noise_ratio.cxx)
add_my_eudaq_exe(alpide_valid_ratio alpide_valid_ratio.cxx)


install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
