set(OBJECT_STORAGE_LIB objectstorage)
# 3rd packages
find_package(spdlog CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(unofficial-aliyun-oss-cpp-sdk CONFIG REQUIRED)
find_package(google_cloud_cpp_common CONFIG REQUIRED)
find_package(google_cloud_cpp_rest_internal CONFIG REQUIRED)
find_package(google_cloud_cpp_storage CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)

set(OS_THIRD_LIBRARIES
    spdlog::spdlog
    absl::status
    unofficial::aliyun-oss-cpp-sdk::cpp-sdk
    google-cloud-cpp::common
    google-cloud-cpp::rest_internal
    CURL::libcurl
    google-cloud-cpp::storage
    unofficial::concurrentqueue::concurrentqueue
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"
    "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE OS_LIB_HREADERS ${CMAKE_CURRENT_SOURCE_DIR}/cppcommon/*.h)
file(GLOB_RECURSE OS_LIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cppcommon/*.cc)
add_library(${OBJECT_STORAGE_LIB} ${OS_LIB_HREADERS} ${OS_LIB_SRCS})
target_link_libraries(${OBJECT_STORAGE_LIB} ${PROJECT_NAME}
                      ${OS_THIRD_LIBRARIES})

if(BUILD_TESTING)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

# install files
install(
  TARGETS ${OBJECT_STORAGE_LIB}
  EXPORT ${OBJECT_STORAGE_LIB}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cppcommon
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING
  PATTERN "*.h")
install(
  EXPORT ${OBJECT_STORAGE_LIB}
  DESTINATION share/${OBJECT_STORAGE_LIB}
  FILE ${OBJECT_STORAGE_LIB}Config.cmake
  NAMESPACE ${PROJECT_NAME}::)
