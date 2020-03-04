
include_directories(../include)
add_executable(mytlu tlu_test_main.cxx)
target_link_libraries(mytlu aidatlu)

add_executable(mytlu_ctrl AidaTluController.cxx)
target_link_libraries(mytlu_ctrl aidatlu)

