#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <ipc.h>

#include "ipc_unit_test_common.h"

static void helpPrint(void);
static void helpWritePrint(void);
static void listPrint(void);
static void writeData(void);
static void writeDataToStruct(int id, long value);

IPC_DATA_IC_SERVICE_S g_dataIcService;

int main(void)
{
    IPC_RET_E ret;
    char command[10];
    char dummy[10];
    bool isRunning = true;
    char *pRetStr;

    memset(&g_dataIcService, 0, sizeof(g_dataIcService));

    ret = ipcServerStart(IPC_USAGE_TYPE_IC_SERVICE);
    if (ret != IPC_RET_OK) {
        printf("ipcServerStart Error:%d\n", ret);
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
        case 'l':
            listPrint();
            break;
        case 'w':
            writeData();
            break;
        case 's':
            ret = ipcSendMessage(IPC_USAGE_TYPE_IC_SERVICE, (const void*)&g_dataIcService, sizeof(g_dataIcService));
            printf("ipcSendMessage return:%d\n", ret);
            break;
        default:
            break;
        }
    }

end:
    ipcServerStop(IPC_USAGE_TYPE_IC_SERVICE);
    printf("bye...\n");
    sleep(1);

    return 0;
}

static void helpPrint(void)
{
    printf("\n-----------\n");
    printf("'h' : help\n");
    printf("'q' : quit\n");
    printf("'l' : list of Server data\n");
    printf("'w' : write to Server data\n");
    printf("'s' : send data to Client\n");
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

static void writeData(void)
{
    char command[40];
    char dummy[40];
    bool isRunning = true;
    int i;

    int id;
    long value;

    char *pRetStr;

    while(isRunning) {
        printf("write command (h=help q=goto main menu):");
        pRetStr = fgets(command, 40, stdin);
        if (pRetStr == NULL) {
            continue;
        }
        memcpy(dummy, command, 40);

        while(strlen(dummy) == 39 && dummy[38] != '\n') {
            pRetStr = fgets(dummy, 40, stdin);
            if (pRetStr == NULL) {
                break;
            }
        }

        command[strlen(command)-1] = '\0';
        switch(command[0]) {
        case 'h':
            helpWritePrint();
            break;
        case 'q':
            isRunning = false;
            break;
        case 'l':
            listPrint();
            break;
        default:
            // write data
            id = (int)strtol(command, NULL, 0);
            value = 0;
            for (i = 0; i < strlen(command); i++) {
                if (command[i] == ' ') {
                    value = (int)strtol(&command[i], NULL, 0);
                }
            }
            writeDataToStruct(id, value);
        }
    }
}

static void helpWritePrint(void)
{
    printf("\n-----------\n");
    printf("'h' : help\n");
    printf("'q' : goto main menu\n");
    printf("'l' : list of Server data\n");
    printf("<ID> <value>: write data\n");
    printf("  ex)\n");
    printf("    write command 2 4\n");
    printf("      -> 2: brake = 4\n");
    printf("-----------\n\n");
}

static void writeDataToStruct(int id, long value)
{
    void *pValue;

    if (id < 0 || IC_SERVICE_LIST_NUM < id) {
        return;
    }

    pValue = (void *)&g_dataIcService + IcServiceList[id].offset;
    memcpy(pValue, (void *)&value, IcServiceList[id].size);
}

