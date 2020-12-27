# Note: Does _NOT_ track external dependencies!

add_custom_command (
  OUTPUT "${CMAKE_BINARY_DIR}/smoothness.pnet"
         "${CMAKE_BINARY_DIR}/gen/pnetc/op/libsingular_smoothness.so"
  COMMAND "${GSPC_HOME}/bin/pnetc"
          --gen-cxxflags=-O3
          --gen-cxxflags=-std=c++11
          --gen-cxxflags=-I"${CMAKE_BINARY_DIR}"
          --gen-cxxflags=-I"${CMAKE_SOURCE_DIR}"
          --gen-cxxflags=-I"${CMAKE_SOURCE_DIR}/src"
          --gen-cxxflags=-I"${GSPC_HOME}/include"
          --gen-cxxflags=-I"${GSPC_HOME}/external/boost/include"
          --gen-ldflags="-L${CMAKE_BINARY_DIR}"
          --gen-ldflags="-L${CMAKE_BINARY_DIR}/src/util-generic"
          --gen-ldflags="-Wl,-rpath='${GSPC_HOME}/lib'"
          --gen-ldflags="-Wl,-rpath='${GSPC_HOME}/external/boost/lib'"
          --force-overwrite-file=true
          --input="${CMAKE_SOURCE_DIR}/workflow/smoothness.xpnet"
          --output="${CMAKE_BINARY_DIR}/smoothness.pnet"
          --path-to-cpp="${CMAKE_BINARY_DIR}/gen"
          --Wbackup-file=OFF
          --Woverwrite-file=OFF
          &&
          make -C "${CMAKE_BINARY_DIR}/gen"
  DEPENDS smoothness.xpnet
)
add_custom_target (smoothness.pnet_and_wrapper-library
  ALL
  DEPENDS "${CMAKE_BINARY_DIR}/gen/pnetc/op/libsingular_smoothness.so"
          "${CMAKE_BINARY_DIR}/smoothness.pnet"
)

install (FILES
  "${CMAKE_BINARY_DIR}/smoothness.pnet"
  DESTINATION libexec/workflow
)

install (FILES
  "${CMAKE_BINARY_DIR}/gen/pnetc/op/libsingular_smoothness.so"
  DESTINATION libexec/workflow
)
