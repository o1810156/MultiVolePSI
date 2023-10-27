cmake_minimum_required (VERSION 2.8)

project("MultiVolePSI")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)


#############################################
#            Flag and #defines              #
#############################################
add_definitions(-DSOLUTION_DIR=\"${CMAKE_SOURCE_DIR}\")
set(CMAKE_C_FLAGS "-ffunction-sections -Wall  -maes -msse2 -msse4.1 -mpclmul -Wfatal-errors -pthread")
set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14")

# Set a default build type for single-configuration
# CMake generators if no build type is set.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Release)
endif()

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
  message(WARNING "\nConfiguration NOT set to Release. Please call cmake with CMAKE_BUILD_TYPE=Release")
endif()

# Select flags.
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")


#############################################
#       Library/includes locations          #
#############################################

# default location for Boost
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/linux/boost/")
set(Boost_NO_SYSTEM_PATHS        ON)

add_subdirectory(sepVolePSI)

add_subdirectory(monoVolePSI)

add_subdirectory(sockets)

#############################################
#              Build Frontend               #
#############################################
add_subdirectory(frontend)

#############################################
#              Build experiments            #
#############################################
add_subdirectory(experiments)

add_subdirectory(sockets_pg)