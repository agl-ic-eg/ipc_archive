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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <cluster_ipc.h>

#include "ipc_unit_test_common.h"

IPC_DATA_IC_SERVICE_S g_dataIcService;

static void changeNotifyCb(void* pData, signed int size, int kind);
static void helpPrint(void);
static void listPrint(void);

int main(void)
{
    IPC_RET_E ret;
    char command[10];
    char dummy[10];
    bool isRunning = true;
    signed int size;
    char *pRetStr;

    memset(&g_dataIcService, 0, sizeof(g_dataIcService));

    ret = ipcClientStart(IPC_USAGE_TYPE_IC_SERVICE);
    if (ret != IPC_RET_OK) {
        printf("ipcClientStart Error:%d\n", ret);
        goto end;
    }

    ret = ipcRegisterCallback(IPC_USAGE_TYPE_IC_SERVICE, changeNotifyCb);
    if (ret != IPC_RET_OK) {
        printf("ipcRegisterCallback Error:%d\n", ret);
        goto end;
    }

    while(isRunning) {
        printf("command (h=help, q=quit):");
        pRetStr = fgets(command, 10, stdin);
        if (pRetStr == NULL) {
            continue;
        }
        memcpy(dummy, command, 10);

        while(strlen(dummy) == 9 && dummy[8] != '\n') {
            pRetStr = fgets(dummy, 10, stdin);
            if (pRetStr == NULL) {
                break;
            }
        }

        command[strlen(command)-1] = '\0';

        switch(command[0]) {
        case 'h':
            helpPrint();
            break;
        case 'q':
            isRunning = false;
            break;
        case 'r':
            size = sizeof(g_dataIcService);
            ret = ipcReadDataPool(IPC_USAGE_TYPE_IC_SERVICE, &g_dataIcService, &size);
            printf("ipcReadDataPool return:%d\n", ret);
            listPrint();
            break;
        default:
            break;
        }
    }

end:
    ipcClientStop(IPC_USAGE_TYPE_IC_SERVICE);
    printf("bye...\n");
    sleep(1);

    return 0;
}

static void changeNotifyCb(void* pData, signed int size, int kind)
{
    signed long longVal;
    signed int intVal;
    signed short shortVal;
    signed char charVal;

    printf("Enter %s\n", __func__);

    switch(size) {
    case 8:
        longVal = *((signed long*)pData);
        printf("kind = %d, size = %d, data=%ld\n", kind, size, longVal);
        break;
    case 4:
        intVal = *((signed int*)pData);
        printf("kind = %d, size = %d, data=%d\n", kind, size, intVal);
        break;
    case 2:
        shortVal = *((signed short *)pData);
        printf("kind = %d, size = %d, data=%d\n", kind, size, shortVal);
        break;
    default:
        charVal = *((signed char *)pData);
        printf("kind = %d, size = %d, data=%d\n", kind, size, charVal);
        break;
    }
    printf("Leave %s\n", __func__);
    return;
}

static void helpPrint(void)
{
    printf("\n-----------\n");
    printf("'h' : help\n");
    printf("'q' : quit\n");
    printf("'r' : read from Client data pool\n");
    printf("-----------\n\n");
}

static void listPrint(void)
{
    int i;
    signed long longVal;
    signed int intVal;
    signed short shortVal;
    signed char charVal;
    void *pValue;

    for (i = 0; i < IC_SERVICE_LIST_NUM; i++) {
        pValue = (void *)&g_dataIcService + IcServiceList[i].offset;
        switch(IcServiceList[i].size) {
        case 8:
            longVal = *((signed long*)pValue);
            printf("%3d: %s(%d) = %ld\n", i, IcServiceList[i].name, IcServiceList[i].size, longVal);
            break;
        case 4:
            intVal = *((signed int*)pValue);
            printf("%3d: %s(%d) = %d\n", i, IcServiceList[i].name, IcServiceList[i].size, intVal);
            break;
        case 2:
            shortVal = *((signed short *)pValue);
            printf("%3d: %s(%d) = %d\n", i, IcServiceList[i].name, IcServiceList[i].size, shortVal);
            break;
        default:
            charVal = *((signed char *)pValue);
            printf("%3d: %s(%d) = %d\n", i, IcServiceList[i].name, IcServiceList[i].size, charVal);
            break;
        }
    }
}

