

function(alpide_eudaq_exe)  
  add_executable(${ARGV0}  ${ARGV0}.cxx)
  target_link_libraries(${ARGV0} ${EUDAQ_LIBRARIES})
  install(TARGETS ${ARGV0}
    DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endfunction()

alpide_eudaq_exe(alpide_noise_ratio)

alpide_eudaq_exe(alpide_valid_ratio)

alpide_eudaq_exe(alpide_trigger_print)

alpide_eudaq_exe(alpide_pickup)
