
add_executable(mygl opengl_main.cc)
target_link_libraries(mygl PRIVATE mycommon m glut GL GLU)

install(TARGETS mygl
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME       DESTINATION bin      COMPONENT runtime
  LIBRARY       DESTINATION lib      COMPONENT runtime  NAMELINK_COMPONENT devel
  ARCHIVE       DESTINATION lib      COMPONENT devel
  PUBLIC_HEADER DESTINATION include  COMPONENT devel
  RESOURCE      DESTINATION resource COMPONENT runtime
  )
