cmake_minimum_required (VERSION 2.6)
project (alpr_test)

include_directories(${CMAKE_SOURCE_DIR}/../include/)
link_directories(${CMAKE_SOURCE_DIR}/../)

add_executable(alpr_test alpr_test.cpp)
target_link_libraries (alpr_test  libopenalpr)

add_executable(alprbatch_test alprbatch_test.c)
target_link_libraries (alprbatch_test  libopenalpr)

add_executable(alprstream_test alprstream.cpp)
target_link_libraries (alprstream_test  libopenalpr libalprstream)
