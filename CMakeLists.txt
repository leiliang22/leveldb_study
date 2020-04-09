cmake_minimum_required(VERSION 3.16)
project (leveldb-demo VERSION 1.22.0 LANGUAGES C CXX)

set(INC_DIR ./leveldb/include)
set(LINK_DIR ./leveldb)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

aux_source_directory(. DIR_SRCS)
add_subdirectory(leveldb)

add_executable(leveldb-demo ${DIR_SRCS})
#target_link_libraries(leveldb-demo leveldb)
