#project(sepVolePSI)

#############################################
#               Build sepVolePSI            #
#############################################

file(GLOB_RECURSE SRC_SEPVOLEPSI ${CMAKE_SOURCE_DIR}/sepVolePSI/*.cpp)

add_library(sepVolePSI ${SRC_SEPVOLEPSI})

target_include_directories(sepVolePSI PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 
target_include_directories(sepVolePSI PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(sepVolePSI visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(sepVolePSI oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(sepVolePSI oc::libOTe)

target_link_libraries(sepVolePSI sockets)