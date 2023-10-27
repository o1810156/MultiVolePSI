#project(sockets_pg)

#############################################
#               Build sockets_pg            #
#############################################

file(GLOB_RECURSE SRC_SOCKETSPG ${CMAKE_SOURCE_DIR}/sockets_pg/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/sockets_pg/)

add_executable(sockets_pg ${SRC_SOCKETSPG})

#############################################
#                Link libraries             #
#############################################

find_package(volePSI REQUIRED)
target_link_libraries(sockets_pg visa::volePSI)

find_package(cryptoTools REQUIRED)
target_link_libraries(sockets_pg oc::cryptoTools)

find_package(libOTe REQUIRED)
target_link_libraries(sockets_pg oc::libOTe)

target_link_libraries(sockets_pg sockets)