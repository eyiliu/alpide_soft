include(ExternalProject)

externalproject_add(
  external_eudaq
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/stable/eudaq
  GIT_REPOSITORY https://github.com/eudaq/eudaq.git
  GIT_PROGRESS ON
  GIT_CONFIG advice.detachedHead=false
  GIT_TAG v2.4.5
  # also compatible with 2.4.4 + 1 commit
  # GIT_TAG fc77bf805509e910091917bfa4072ced3f5b3480 # 2.4.4 + 1 commit
  CMAKE_ARGS -DEUDAQ_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DEUDAQ_BUILD_GUI=OFF -DEUDAQ_BUILD_STDEVENT_MONITOR=OFF -DEUDAQ_LIBRARY_BUILD_LCIO=ON -DUSER_EUDET_BUILD=ON -DUSER_TLU_BUILD=ON -DUSER_TLU_BUILD_EUDET=OFF
  )
set_target_properties(external_eudaq PROPERTIES EXCLUDE_FROM_ALL TRUE)
message("To build EUDAQ package, use command:\n>  make external_eudaq")
