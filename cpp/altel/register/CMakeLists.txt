cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(altel VERSION 1.0.0 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/INSTALL CACHE PATH "Default prefix of install directories" FORCE)
endif()

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

include(common/mysystem.cmake)
add_subdirectory(common)

add_subdirectory(telescope)
#add_subdirectory(aidatlu)

#add_subdirectory(eudaq)
add_subdirectory(web)
add_subdirectory(gl)
