
add_executable(mygl opengl_main.cc)
target_link_libraries(mygl PRIVATE mycommon m glut GL GLU)


add_executable(mygl_test src/TelescopeGL.cc src/TelescopeGL_shaders.cc)
target_link_libraries(mygl_test PRIVATE sfml-window sfml-system GL GLEW)

target_include_directories(mygl_test
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )


install(TARGETS mygl mygl_test
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime  NAMELINK_COMPONENT devel
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )


