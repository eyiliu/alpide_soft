

set(SRC_GETOPT getopt/getopt.c)
set(SRC_LINENOISENG linenoiseng/linenoise.cpp linenoiseng/wcwidth.cpp ConvertUTF.cpp)
add_library(common ${SRC_GETOPT} ${SRC_LINENOISENG})

