cmake_minimum_required(VERSION 3.10)

# set the project name
project(mpsshow)

# set the output directories
set (ProdDir "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ProdDir}/lib/static")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProdDir}/lib/shared")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProdDir}/bin")

include_directories("include")

# add our project library
add_library (${PROJECT_NAME} "src/mps-show.c")

