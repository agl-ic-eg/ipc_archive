# Copyright (c) 2021, Nippon Seiki Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Define project Targets
set(TEST_CLIENT_NAME ipc_unit_test_client)
set(TEST_SERVER_NAME ipc_unit_test_server)

add_executable(${TEST_CLIENT_NAME} ipc_unit_test_client.c ipc_unit_test_common.c)
target_link_libraries(${TEST_CLIENT_NAME} ${TARGET_NAME})
target_include_directories(${TEST_CLIENT_NAME} PRIVATE
    ./
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_executable(${TEST_SERVER_NAME} ipc_unit_test_server.c ipc_unit_test_common.c)
target_link_libraries(${TEST_SERVER_NAME} ${TARGET_NAME})
target_include_directories(${TEST_SERVER_NAME} PRIVATE
    ./
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)


