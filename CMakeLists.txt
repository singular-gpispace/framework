cmake_minimum_required (VERSION 3.13)
project (singular-smoothness LANGUAGES C CXX)

include (CTest)
enable_testing()

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

add_subdirectory (cmake)
include (add_macros)
include (beautify_find_boost)
include (beautify_find_GPISpace)

find_package (GPISpace REQUIRED COMPONENTS VERSION=20.12)
find_boost (1.61 REQUIRED QUIET FROM_GPISPACE_INSTALLATION COMPONENTS
  date_time
  filesystem
  program_options
  regex
  serialization
  system
  test_exec_monitor
  thread
  unit_test_framework
  timer
  iostreams
  chrono
)

#set (BUNDLE_ROOT "libexec/bundle")
#bundle_GPISpace (DESTINATION "${BUNDLE_ROOT}/gpispace" COMPONENTS "runtime" "monitoring")

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories ("src")

add_subdirectory (src/util-generic)

extended_add_library (NAME installation
  TYPE SHARED
  NAMESPACE SINGULAR-smoothness
  SOURCES "src/installation.cpp"
  LIBRARIES Util::Generic GPISpace::execution
  POSITION_INDEPENDENT
  INSTALL
)
set_target_properties (SINGULAR-smoothness-installation
  PROPERTIES
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

extended_add_library (NAME module
  TYPE SHARED
  NAMESPACE SINGULAR-smoothness
  SOURCES "src/bin/sminterface.cpp" "smoothness/singular_commands.cpp"
  LIBRARIES SINGULAR-smoothness::installation Util::Generic GPISpace::execution Singular
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
  SOURCES "src/bin/SINGULAR-smoothness.cpp"
  LIBRARIES SINGULAR-smoothness::installation GPISpace::execution
  INSTALL
)
set_target_properties (SINGULAR-smoothness.exe
  PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

add_subdirectory (smoothness)
add_subdirectory (workflow)

#install (DIRECTORY "${SINGULAR_HOME}/"
#  DESTINATION libexec/bundle/singular
#  USE_SOURCE_PERMISSIONS
#  PATTERN "share/singular/html/*" EXCLUDE
#)

