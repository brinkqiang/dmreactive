cmake_minimum_required(VERSION 3.21)

PROJECT(dmreactive)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
INCLUDE(cmake/ModuleImport.cmake)
INCLUDE(cmake/ModuleCompileOptions.cmake)
ModuleSetCompileOptions()

SET(DMREACTIVE_VERSION_MAJOR "1")
SET(DMREACTIVE_VERSION_MINOR "0")
SET(DMREACTIVE_VERSION_PATCH "1")
SET(DMREACTIVE_VERSION "${DMREACTIVE_VERSION_MAJOR}.${DMREACTIVE_VERSION_MINOR}.${DMREACTIVE_VERSION_PATCH}")

MESSAGE(STATUS "VERSION: ${DMREACTIVE_VERSION}")

OPTION(USE_DMREACTIVE "use dmreactive" OFF)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/dmreactive_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/dmreactive_config.h)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

enable_testing()

ModuleImportAll("thirdparty")

LibImport("libdmreactive" "src/libdmreactive")

if(PROJECT_IS_TOP_LEVEL)
    ExeImportAndTest("test" "libdmreactive;dmtest")
    ExeImportAndTest("examples" "libdmreactive")
endif()

AddInstall("libdmreactive" "${CMAKE_CURRENT_SOURCE_DIR}/include")
