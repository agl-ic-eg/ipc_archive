
# Define project Targets
add_library(${TARGET_NAME} SHARED
    ipc_client.c
    ipc_server.c
    ipc_internal.c
    ipc_usage_info_table.c
)

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ./
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:/include>
)

# find thread library
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} INTERFACE Threads::Threads)
#target_link_libraries(${TARGET_NAME} INTERFACE ${SERVER_API_NAME})

set_target_properties(${TARGET_NAME}
    PROPERTIES
    VERSION ${IPC_LIBRARY_VERSION}
    SOVERSION 1
)

# make install
# Generate config file
install(
    TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
#    PUBLIC_HEADER DESTINATION include
)
install(FILES ../include/ipc.h ../include/ipc_protocol.h DESTINATION include)
