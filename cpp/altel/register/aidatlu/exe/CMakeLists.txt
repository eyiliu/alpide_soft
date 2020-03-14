
include_directories(../include)
add_executable(mytlu tlu_test_main.cxx)
target_link_libraries(mytlu aidatlu)

add_executable(mytlu_ctrl AidaTluController.cxx)
target_link_libraries(mytlu_ctrl aidatlu)

install(TARGETS
  mytlu
  mytlu_ctrl
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
