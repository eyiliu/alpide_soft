add_executable(mygl opengl_main.cc)
target_link_libraries(mygl PRIVATE mycommon m glut GL GLU)

add_library(telgl SHARED src/TelescopeGL.cc src/TelescopeGL_shaders.cc)
target_link_libraries(telgl
  PRIVATE GL GLEW glut
  PUBLIC sfml-window  sfml-system
  )

set(LIB_PUBLIC_HEADERS include/TelescopeGL.hh)
set_target_properties(telgl PROPERTIES PUBLIC_HEADER "${LIB_PUBLIC_HEADERS}")
target_include_directories(telgl
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

add_executable(mygl_test TelescopeGL_main.cxx)
target_link_libraries(mygl_test PRIVATE telgl)

install(TARGETS telgl mygl mygl_test
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime  NAMELINK_COMPONENT devel
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
