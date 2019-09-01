include_directories(../../jade/include)
include_directories(../include)

add_executable(AlpideRegCtrl src/AlpideRegCtrl.cxx)
target_link_libraries(AlpideRegCtrl ${JADE_CORE_LIBRARY} ${ALTEL_LIBRARY})
list(APPEND INSTALL_TARGETS AlpideRegCtrl)

add_executable(AlpideTest src/AlpideTest.cxx)
target_link_libraries(AlpideTest ${JADE_CORE_LIBRARY} ${ALTEL_LIBRARY})
list(APPEND INSTALL_TARGETS AlpideTest)


add_executable(alpide_change_ip src/alpide_change_ip.cxx)
target_link_libraries(alpide_change_ip ${JADE_CORE_LIBRARY} ${ALTEL_LIBRARY})
list(APPEND INSTALL_TARGETS alpide_change_ip)


install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

