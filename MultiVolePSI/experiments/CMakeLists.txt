#project(experiments)

#############################################
#               Build experiments           #
#############################################

file(GLOB_RECURSE SRC_EXPERIMENTS ${CMAKE_SOURCE_DIR}/experiments/*.cpp)

add_executable(experiments ${SRC_EXPERIMENTS})

include_directories(${CMAKE_SOURCE_DIR}/)

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(experiments visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(experiments oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(experiments oc::libOTe)

target_link_libraries(experiments sepVolePSI)

target_link_libraries(experiments monoVolePSI)