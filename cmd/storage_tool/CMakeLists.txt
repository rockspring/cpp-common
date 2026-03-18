add_executable(storage_tool storage_tool.cc)
target_link_libraries(storage_tool objectstorage)
install(TARGETS storage_tool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
