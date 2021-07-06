/*
 * Copyright (c) 2021, Nippon Seiki Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IPC_H
#define IPC_H

#include <ipc_protocol.h>

// Environment Variable for unix-domain-socket file path
#define IPC_ENV_DOMAIN_SOCKET_PATH "IPC_DOMAIN_PATH"

// return value for API
typedef enum {
    IPC_RET_OK = 0,
    IPC_ERR_PARAM,
    IPC_ERR_SEQUENCE,
    IPC_ERR_NO_RESOURCE,
    IPC_ERR_OTHER
} IPC_RET_E;

// format of callback function
typedef void (*IPC_CHANGE_NOTIFY_CB)(void* pData, signed int size, int kind);

// for Server Function
IPC_RET_E ipcServerStart(IPC_USAGE_TYPE_E usageType);
IPC_RET_E ipcSendMessage(IPC_USAGE_TYPE_E usageType, const void* pData, signed int size);
IPC_RET_E ipcServerStop(IPC_USAGE_TYPE_E usageType);

// for Client Function
IPC_RET_E ipcClientStart(IPC_USAGE_TYPE_E usageType);
IPC_RET_E ipcReadDataPool(IPC_USAGE_TYPE_E usageType, void* pData, signed int* pSize);
IPC_RET_E ipcRegisterCallback(IPC_USAGE_TYPE_E usageType, IPC_CHANGE_NOTIFY_CB changeNotifyCb);
IPC_RET_E ipcClientStop(IPC_USAGE_TYPE_E usageType);

#endif // IPC_H
