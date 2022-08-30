configure_file("installation.hpp.in" "installation.hpp" @ONLY)

extended_add_library (NAME installation
  TYPE SHARED
  NAMESPACE SINGULAR-parallel
  SOURCES "installation.cpp"
  LIBRARIES Util::Generic GPISpace::execution
  POSITION_INDEPENDENT
  INSTALL
)
set_target_properties (SINGULAR-parallel-installation
  PROPERTIES
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

extended_add_library (NAME module
  TYPE SHARED
  NAMESPACE SINGULAR-parallel
  SOURCES "bin/SINGULAR-parallel-module.cpp" "../parallel/singular_functions.cpp"
  COMPILE_DEFINITIONS SING_NDEBUG OM_NDEBUG
  LIBRARIES SINGULAR-parallel::installation Util::Generic GPISpace::execution Singular
  POSITION_INDEPENDENT
  INSTALL
  INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
set_target_properties (SINGULAR-parallel-module
  PROPERTIES
  INSTALL_RPATH "$ORIGIN/lib"
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

extended_add_library (NAME module
  TYPE SHARED
  NAMESPACE SINGULAR-smoothness
  SOURCES "bin/sminterface.cpp" "../smoothness/singular_commands.cpp"
  LIBRARIES SINGULAR-parallel::installation Util::Generic GPISpace::execution Singular
  POSITION_INDEPENDENT
  INSTALL
  INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
set_target_properties (SINGULAR-smoothness-module
  PROPERTIES
  INSTALL_RPATH "$ORIGIN/lib"
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

extended_add_executable (NAME SINGULAR-smoothness
  SOURCES "bin/SINGULAR-smoothness.cpp"
  LIBRARIES SINGULAR-parallel::installation GPISpace::execution
  INSTALL
)
set_target_properties (SINGULAR-smoothness.exe
  PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
  INSTALL_RPATH_USE_LINK_PATH TRUE
)
