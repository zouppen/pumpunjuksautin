# Git integration
find_package(Git)
if(Git_FOUND)
  EXECUTE_PROCESS(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
    WORKING_DIRECTORY ${REAL_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if (GIT_DESCRIBE_RESULT)
    set(PRODUCT_VERSION "unknown-not-git-tree")
    message(STATUS "Not a git repo, not embedding version information")
  else ()
    set(PRODUCT_VERSION ${GIT_DESCRIBE_VERSION})
    message(STATUS "Build version: ${PRODUCT_VERSION}")
  endif()
else(Git_FOUND)
  set(PRODUCT_VERSION "unknown-no-git-support")
  message(STATUS "Git not found. Install git to embed version information to the binary")
endif()

# Producing source file with the version data
configure_file(${REAL_SOURCE_DIR}/version.template ${CMAKE_CURRENT_SOURCE_DIR}/generated/version.c ESCAPE_QUOTES)
