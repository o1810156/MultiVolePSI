#project(monoVolePSI)

#############################################
#               Build monoVolePSI           #
#############################################

file(GLOB_RECURSE SRC_MONOVOLEPSI ${CMAKE_SOURCE_DIR}/monoVolePSI/*.cpp)

add_library(monoVolePSI ${SRC_MONOVOLEPSI})

target_include_directories(monoVolePSI PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 
target_include_directories(monoVolePSI PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(monoVolePSI visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(monoVolePSI oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(monoVolePSI oc::libOTe)

target_link_libraries(monoVolePSI sockets)