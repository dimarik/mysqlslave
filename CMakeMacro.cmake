SET (FLAGS_DEFAULT  "-fPIC -pipe")
SET (FLAGS_WARNING  "-Wall -Werror -Wno-long-long -Wno-variadic-macros -Wno-strict-aliasing")# -Wextra -pedantic")
SET (FLAGS_CXX_LANG "-Wno-deprecated")
SET (FLAGS_RELEASE  "-O3 -DNDEBUG") # -fomit-frame-pointer -funroll-loops
SET (FLAGS_DEBUG    "-ggdb")

# This is needed because debian package builder sets -DCMAKE_BUILD_TYPE=None
IF (CMAKE_BUILD_TYPE STREQUAL None)
  SET (CMAKE_BUILD_TYPE Release)
ENDIF ()

SET (CMAKE_C_FLAGS_DEBUG     "${FLAGS_DEFAULT} ${FLAGS_WARNING} ${FLAGS_DEBUG}")
SET (CMAKE_C_FLAGS_RELEASE   "${FLAGS_DEFAULT} ${FLAGS_WARNING} ${FLAGS_DEBUG} ${FLAGS_RELEASE}")

SET (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}   ${FLAGS_CXX_LANG}")
SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${FLAGS_CXX_LANG}")

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  SET (CMAKE_BUILD_TYPE RELEASE)
  SET (CMAKE_BUILD_TYPE RELEASE CACHE STRING "Build type" FORCE)
ENDIF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

###############################################################################

# Enable printf format macros from <inttypes.h> in C++ code.
ADD_DEFINITIONS (-D__STDC_FORMAT_MACROS)

# Enable type limit macros from <stdint.h> in C++ code.
ADD_DEFINITIONS (-D__STDC_LIMIT_MACROS)

# Enable 64-bit off_t type to work with big files.
ADD_DEFINITIONS (-D_FILE_OFFSET_BITS=64)

# Make FIND_LIBRARY search for static libs first and make it search inside lib64/
# directory in addition to the usual lib/ one.

SET (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})
SET (CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_STATIC_LIBRARY_PREFIX} ${CMAKE_SHARED_LIBRARY_PREFIX})
SET (FIND_LIBRARY_USE_LIB64_PATHS TRUE)
SET (LINK_SEARCH_END_STATIC TRUE)

# Include source tree root, include directory inside it and build tree root,
# which is for files, generated by cmake from templates (e.g. autogenerated
# C/C++ includes).

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR})
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR})

###############################################################################
# USE_PROGRAM (bin)
# -----------------------------------------------------------------------------
# Find program [bin] using standard FIND_PROGRAM command and save its path into
# variable named BIN_[bin].

MACRO (USE_PROGRAM bin)
  FIND_PROGRAM (BIN_${bin} ${bin})
  IF (BIN_${bin})
    MESSAGE (STATUS "FOUND ${BIN_${bin}}")
  ELSE ()
    MESSAGE (STATUS "ERROR ${BIN_${bin}}")
  ENDIF ()
ENDMACRO (USE_PROGRAM)

# USE_INCLUDE (inc [FIND_PATH_ARGS ...])
# -----------------------------------------------------------------------------
# Find include [inc] using standard FIND_PATH command and save its dirname into
# variable named INC_[inc]. Also include its dirname into project.

MACRO (USE_INCLUDE inc)
  FIND_PATH (INC_${inc} ${inc} ${ARGN})
  IF (INC_${inc})
    MESSAGE (STATUS "FOUND ${INC_${inc}}/${inc}")  # SHOULD BE BOLD GREEN
    INCLUDE_DIRECTORIES (${INC_${inc}})
  ELSE ()
    MESSAGE (STATUS "ERROR ${INC_${inc}}/${inc}")  # SHOULD BE BOLD RED
  ENDIF ()
ENDMACRO (USE_INCLUDE)

# USE_LIBRARY (lib [FIND_LIBRARY_ARGS ...])
# -----------------------------------------------------------------------------
# Find library [lib] using standard FIND_LIBRARY command and save its path into
# variable named LIB_[lib].

MACRO (USE_LIBRARY lib)
  FIND_LIBRARY (LIB_${lib} ${lib} ${ARGN})
  IF (LIB_${lib})
    MESSAGE (STATUS "FOUND ${LIB_${lib}}")  # SHOULD BE BOLD GREEN
  ELSE ()
    MESSAGE (STATUS "ERROR ${LIB_${lib}}")  # SHOULD BE BOLD RED
  ENDIF ()
ENDMACRO (USE_LIBRARY)

# USE_PACKAGE (var lib inc [FIND_PATH_ARGS ...])
# -----------------------------------------------------------------------------
# Find package using USE_LIBRARY and USE_INCLUDE macros.

MACRO (USE_PACKAGE lib inc)
  USE_LIBRARY (${lib} ${ARGN})
  USE_INCLUDE (${inc} ${ARGN})
ENDMACRO (USE_PACKAGE)

MACRO (USE_PACKAGE_STATIC lib inc)
  SET (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
  USE_PACKAGE (${lib} ${inc} ${ARGN})
  SET (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})
ENDMACRO (USE_PACKAGE_STATIC)

MACRO (USE_PACKAGE_SHARED lib inc)
  SET (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
  USE_PACKAGE (${lib} ${inc} ${ARGN})
  SET (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})
ENDMACRO (USE_PACKAGE_SHARED)

# USE_SUBPATH (var sub)
# -----------------------------------------------------------------------------
# Find subpath [sub] using standard FIND_PATH command and save its dirname into
# variable named [var].

MACRO (USE_SUBPATH var sub)
  FIND_PATH (${var}_PREFIX ${sub} ONLY_CMAKE_FIND_ROOT_PATH)
  IF (${var}_PREFIX)
    GET_FILENAME_COMPONENT (${var} "${${var}_PREFIX}/${sub}" PATH)
    MESSAGE (STATUS "FOUND ${var}=${${var}}")
  ELSE (${var}_PREFIX)
    MESSAGE (STATUS "ERROR ${var}")
  ENDIF (${var}_PREFIX)
ENDMACRO (USE_SUBPATH)

###############################################################################
# MAKE_LIBRARY (apath <SHARED|STATIC> [LIBRARIES_TO_LINK_WITH [...]])
# -----------------------------------------------------------------------------
# Make library of SHARED or STATIC type from source code inside the [apath]
# subfolder and install it and all header files from the subfolder.

MACRO (MAKE_LIBRARY apath atype)
  GET_FILENAME_COMPONENT (${apath}_NAME "${apath}" NAME)
  AUX_SOURCE_DIRECTORY (${apath} SRC_${${apath}_NAME})
  ADD_LIBRARY (${${apath}_NAME} ${atype} ${SRC_${${apath}_NAME}})
  IF (${ARGC} GREATER 2)
    TARGET_LINK_LIBRARIES (${${apath}_NAME} ${ARGN})
  ENDIF (${ARGC} GREATER 2)
  # TODO SET_TARGET_PROPERTIES (...)
  INSTALL (TARGETS ${${apath}_NAME} DESTINATION lib)
  INSTALL (DIRECTORY ${apath} DESTINATION include FILES_MATCHING PATTERN "*.h")
  INSTALL (DIRECTORY ${apath} DESTINATION include FILES_MATCHING PATTERN "*.hpp")
  INSTALL (DIRECTORY ${apath} DESTINATION include FILES_MATCHING PATTERN "*.tcc")
ENDMACRO (MAKE_LIBRARY)

# MAKE_SHARED (apath [LIBRARIES_TO_LINK_WITH [...]])
# -----------------------------------------------------------------------------
# Make SHARED library with MAKE_LIBRARY macro.

MACRO (MAKE_SHARED apath)
  MAKE_LIBRARY (${apath} SHARED ${ARGN})
ENDMACRO (MAKE_SHARED)

# MAKE_STATIC (apath [LIBRARIES_TO_LINK_WITH [...]])
# -----------------------------------------------------------------------------
# Make STATIC library with MAKE_LIBRARY macro.

MACRO (MAKE_STATIC apath)
  MAKE_LIBRARY (${apath} STATIC ${ARGN})
ENDMACRO (MAKE_STATIC)

# MAKE_PROGRAM (apath)
# -----------------------------------------------------------------------------
# Make program (executable) from source code inside the [apath] subfolder and
# install it.

MACRO (MAKE_PROGRAM apath)
  GET_FILENAME_COMPONENT (${apath}_NAME "${apath}" NAME)
  AUX_SOURCE_DIRECTORY (${apath} SRC_${${apath}_NAME})
  ADD_EXECUTABLE (${${apath}_NAME} ${SRC_${${apath}_NAME}})
  IF (${ARGC} GREATER 1)
    TARGET_LINK_LIBRARIES (${${apath}_NAME} ${ARGN})
  ENDIF (${ARGC} GREATER 1)
  INSTALL (TARGETS ${${apath}_NAME} DESTINATION bin)
ENDMACRO (MAKE_PROGRAM)

# MAKE_TEST (apath)
# -----------------------------------------------------------------------------
# Make test from source code inside the [apath] subfolder.

MACRO (MAKE_TEST apath)
  GET_FILENAME_COMPONENT (${apath}_NAME "${apath}" NAME)
  AUX_SOURCE_DIRECTORY (${apath} SRC_test_${${apath}_NAME})
  ADD_EXECUTABLE (test_${${apath}_NAME} ${SRC_test_${${apath}_NAME}})
  IF (${ARGC} GREATER 1)
    TARGET_LINK_LIBRARIES (test_${${apath}_NAME} ${ARGN})
  ENDIF (${ARGC} GREATER 1)
  ADD_TEST (test_${${apath}_NAME} test_${${apath}_NAME}})
ENDMACRO (MAKE_TEST)

# INSTALL_TEMPLATE (sub [INSTALL_ARGS [...]])
# -----------------------------------------------------------------------------
# Install template files (*.in) with one line of code, all arguments except the
# first one will be left untouched and proxied to INSTALL (FILES) call.

MACRO (INSTALL_TEMPLATE sub)
  STRING (REGEX REPLACE "\\.in$" "" ${sub}_NOIN ${sub})
  CONFIGURE_FILE (${sub} ${PROJECT_BINARY_DIR}/auto/${${sub}_NOIN})
  INSTALL (FILES ${PROJECT_BINARY_DIR}/auto/${${sub}_NOIN} ${ARGN})
ENDMACRO (INSTALL_TEMPLATE)

###############################################################################

function(PROTOBUF_GENERATE_CPP SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    string(REPLACE ".proto" "" FIL_WE ${FIL})

    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
      COMMAND protoc
      ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

