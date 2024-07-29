cmake_minimum_required(VERSION 3.10)

project(mps-show)

set (ProdDir "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ProdDir}/lib/static")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProdDir}/lib/shared")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProdDir}/bin")

include_directories(
    "."
    "include"
    "${PROJECT_NAME}/include"
)

if(NOT TARGET ${PROJECT_NAME})
    add_subdirectory(${PROJECT_NAME})
endif()

set (common_sources
    "tools/util.c"
)

set (bmp_sources
    "tools/pal-tools.c"
    "quickbmp/bmp.c"
)

set (executables
    mpsexplore
    mpsextract
    palextract
)

add_library(quickbmp ${bmp_sources})

foreach(executable IN LISTS executables)
    add_executable(${executable} "executables/${executable}.c" ${common_sources})
    target_link_libraries(${executable} "mpsshow")
endforeach(executable IN LISTS executables)

target_link_libraries(mpsexplore quickbmp)