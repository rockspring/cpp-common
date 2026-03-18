cmake_minimum_required(VERSION 3.20)
project(storage_tool)
set(CMAKE_CXX_STANDARD 20)

find_package(cppcommon CONFIG REQUIRED)
find_package(objectstorage CONFIG REQUIRED)

add_executable(storage_tool storage_tool.cc)
target_link_libraries(storage_tool cppcommon::objectstorage)
