cmake_minimum_required(VERSION 3.20)
project(cppcommon)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-Wall)
include(GNUInstallDirs) # CMAKE_INSTALL_INCLUDEDIR / CMAKE_INSTALL_LIBDIR / ...
option(WITH_OBJECT_STORAGE "build object storage" OFF)

if(BUILD_TESTING)
  # set testing install prefix
  set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build/install/)
endif()

# 3rd packages
find_package(spdlog CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

set(THIRD_LIBRARIES spdlog::spdlog rapidjson absl::status absl::statusor
                    httplib::httplib)

include_directories(.)
file(GLOB_RECURSE LIB_HREADERS cppcommon/*.h)
file(GLOB_RECURSE LIB_SRCS cppcommon/*.cc)
add_library(${PROJECT_NAME} ${LIB_HREADERS} ${LIB_SRCS})
target_link_libraries(${PROJECT_NAME} ${THIRD_LIBRARIES})

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# install files
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  DIRECTORY cppcommon
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING
  PATTERN "*.h")
install(
  EXPORT ${PROJECT_NAME}
  DESTINATION share/${PROJECT_NAME}
  FILE ${PROJECT_NAME}Config.cmake
  NAMESPACE ${PROJECT_NAME}::)

# submodules
if(WITH_OBJECT_STORAGE)
  add_subdirectory(modules/object-storage)
  add_subdirectory(cmd/storage_tool)
endif()
