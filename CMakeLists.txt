cmake_minimum_required(VERSION 3.28.3)

project(map)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

aux_source_directory(src SRC_LIST)

add_compile_options(-Wall -Wextra -O2)

find_package(rapidfuzz REQUIRED)

add_executable(process ${SRC_LIST})

target_link_libraries(process rapidfuzz::rapidfuzz jsoncpp boost_system pthread)