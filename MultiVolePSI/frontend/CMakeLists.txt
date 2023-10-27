#project(fronend)

#############################################
#               Build fronend               #
#############################################

file(GLOB_RECURSE SRC_FRONTEND ${CMAKE_SOURCE_DIR}/frontend/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/frontend/)

add_executable(frontend ${SRC_FRONTEND})

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(frontend visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(frontend oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(frontend oc::libOTe)

target_link_libraries(frontend sepVolePSI)