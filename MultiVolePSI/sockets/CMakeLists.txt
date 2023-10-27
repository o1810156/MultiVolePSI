#project(sockets)

#############################################
#               Build sockets               #
#############################################

file(GLOB_RECURSE SRC_SOCKETS ${CMAKE_SOURCE_DIR}/sockets/*.cpp)

add_library(sockets ${SRC_SOCKETS})

target_include_directories(sockets PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                    $<INSTALL_INTERFACE:include>)
target_include_directories(sockets PUBLIC 
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
                    $<INSTALL_INTERFACE:include>) 

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(sockets visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(sockets oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(sockets oc::libOTe)