include(ExternalProject)

if(${CMAKE_VERSION} VERSION_LESS "3.8.0")
  set(_UPDATE_DISCONNECTED 0)
else()
  set(_UPDATE_DISCONNECTED 1)
endif()

set(IOWOW_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")

if("${IOWOW_URL}" STREQUAL "")
  if(EXISTS ${CMAKE_SOURCE_DIR}/iowow.zip)
    set(IOWOW_URL ${CMAKE_SOURCE_DIR}/iowow.zip)
  else()
    set(IOWOW_URL https://github.com/Softmotions/iowow/archive/master.zip)
  endif()
endif()

message("IOWOW_URL: ${IOWOW_URL}")

if(APPLE)
  set(BYPRODUCT "${CMAKE_BINARY_DIR}/lib/libiowow-1.a")
else()
  set(BYPRODUCT "${CMAKE_BINARY_DIR}/src/extern_iowow-build/src/libiowow-1.a")
endif()

set(CMAKE_ARGS
    -DOWNER_PROJECT_NAME=${PROJECT_NAME} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} -DASAN=${ASAN}
    -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF)

# In order to properly pass owner project CMAKE variables than contains semicolons,
# we used a specific separator for 'ExternalProject_Add', using the LIST_SEPARATOR parameter.
# This allows building fat binaries on macOS, etc. (ie: -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64")
# Otherwise, semicolons get replaced with spaces and CMake isn't aware of a multi-arch setup.
set(semicolon_sub "^^")

foreach(
  extra
  CMAKE_OSX_ARCHITECTURES
  CMAKE_C_COMPILER
  CMAKE_TOOLCHAIN_FILE
  ANDROID_PLATFORM
  ANDROID_ABI
  TEST_TOOL_CMD
  PLATFORM
  ENABLE_BITCODE
  ENABLE_ARC
  ENABLE_VISIBILITY
  ENABLE_STRICT_TRY_COMPILE
  ARCHS)
  if(DEFINED ${extra})
    # Replace occurences of ";" with our custom separator and append to CMAKE_ARGS
    string(REPLACE ";" "${semicolon_sub}" extra_sub "${${extra}}") 
    list(APPEND CMAKE_ARGS "-D${extra}=${extra_sub}")
  endif()
endforeach()

message("IOWOW CMAKE_ARGS: ${CMAKE_ARGS}")

ExternalProject_Add(
  extern_iowow
  URL ${IOWOW_URL}
  DOWNLOAD_NAME iowow.zip
  TIMEOUT 360
  PREFIX ${CMAKE_BINARY_DIR}
  BUILD_IN_SOURCE OFF
  UPDATE_COMMAND ""
  UPDATE_DISCONNECTED ${_UPDATE_DISCONNECTED}
  LOG_DOWNLOAD OFF
  LOG_UPDATE OFF
  LOG_BUILD OFF
  LOG_CONFIGURE OFF
  LOG_INSTALL OFF
  LIST_SEPARATOR "${semicolon_sub}"
  CMAKE_ARGS ${CMAKE_ARGS}
  BUILD_BYPRODUCTS ${BYPRODUCT})

add_library(iowow_s STATIC IMPORTED GLOBAL)
set_target_properties(iowow_s PROPERTIES IMPORTED_LOCATION ${BYPRODUCT})
add_dependencies(iowow_s extern_iowow)

if(DO_INSTALL_CORE)
  install(FILES "${BYPRODUCT}" DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

list(APPEND PROJECT_LLIBRARIES iowow_s m)
list(APPEND PROJECT_INCLUDE_DIRS "${IOWOW_INCLUDE_DIR}")
