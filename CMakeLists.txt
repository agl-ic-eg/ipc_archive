# Copyright (c) 2021, Nippon Seiki Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

project(ipc C)

cmake_minimum_required(VERSION 2.8)

# set gcc flags
set(CMAKE_C_FLAGS "-Wall -O2")

# Build option for DEBUG
option(DEBUG_BUILD "This is debug build." OFF)
if(DEBUG_BUILD)
    add_definitions(-DDEBUG)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/installdir/usr CACHE PATH "..." FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

include(GNUInstallDirs)
set(IPC_LIBRARY_VERSION "1.0.0")

# Name of library API
set(TARGET_NAME ipc)

# Name of IC-service server API
#set(SERVER_API_NAME cluster_server_api)

# Subdirectories
add_subdirectory(src)
add_subdirectory(ipc_unit_test)

configure_file(ipc.pc.in ipc.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ipc.pc
    DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
