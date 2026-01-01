set(PROJECT_NAME ContainerProject)

if (TARGET ${PROJECT_NAME})
    message(STATUS "# ${PROJECT_NAME} is already defined. Skipping build configuration.")
    
    return()
endif()

cmake_minimum_required(VERSION 3.20)
project(${PROJECT_NAME} LANGUAGES CXX)

# ------ Create the library ------- #
include(${CMAKE_SOURCE_DIR}/external/CMake/Library.cmake)

set(LIBRARY_TYPE "INTERFACE" STRING "Build a header only library instead of a static one")
set(INCLUDE_TYPE "INTERFACE" STRING)
set(TARGET_TYPE "INTERFACE" STRING)

set(MAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADER_DIR ${MAIN_DIR}/include)

set(INCLUDE_DIR_LIST
    ${MAIN_DIR}/include
)

set(HEADER_LIST
	${HEADER_DIR}/Arena.h
    ${HEADER_DIR}/LinearArena.h
	${HEADER_DIR}/DynamicArray.h
    ${HEADER_DIR}/StaticArray.h
	${HEADER_DIR}/List.h
    ${HEADER_DIR}/HashTable.h
	${HEADER_DIR}/HashMap.h
	${HEADER_DIR}/HashSet.h
    ${HEADER_DIR}/Variant.h
)

source_group(TREE "${HEADER_DIR}" PREFIX "include" FILES ${HEADER_LIST})

create_library(
    LIBRARY_TYPE ${LIBRARY_TYPE}
    TARGET_NAME ${PROJECT_NAME}
    PUBLIC_HEADERS ${HEADER_LIST}
)

install_library(
    TARGET_NAME ${PROJECT_NAME}
    INCLUDE_PROPERTY ${INCLUDE_TYPE}
    INCLUDE_DIRS ${INCLUDE_DIR_LIST}
)

# ------ Set the project options ------ #
include(${CMAKE_SOURCE_DIR}/external/CMake/Project.cmake)

project_options(
    TARGET_TYPE ${TARGET_TYPE}
)

# ------ Create Demofile ------ #
option(CONTAINER_DEMO_FILE "Build the demo file" ON)

if (CONTAINER_DEMO_FILE)
    message(STATUS "# Create the demofile project")

    set(DEMOFILE_DIR ${MAIN_DIR}/demofile)
    
    add_subdirectory(${DEMOFILE_DIR})
endif()