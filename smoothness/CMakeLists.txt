list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package (Singular REQUIRED)

extended_add_library (NAME smoothness_implementation
  TYPE MODULE
  SOURCES "implementation.cpp" "singular_commands.cpp"
  INCLUDE_DIRECTORIES PRIVATE "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/gen"
  LIBRARIES GPISpace::workflow_development Singular
  DEPENDS smoothness.pnet_and_wrapper-library
  INSTALL INSTALL_DESTINATION "libexec/workflow"
)
set_target_properties (smoothness_implementation
  PROPERTIES
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

install (FILES LIB/smoothtestgspc.lib
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
