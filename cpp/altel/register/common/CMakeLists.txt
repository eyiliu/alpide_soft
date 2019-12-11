
set(SRC_GETOPT getopt/getopt.c)
set(SRC_LINENOISENG linenoiseng/linenoise.cpp linenoiseng/wcwidth.cpp linenoiseng/ConvertUTF.cpp)
add_library(mycommon ${SRC_GETOPT} ${SRC_LINENOISENG})
link_libraries(mycommon)
