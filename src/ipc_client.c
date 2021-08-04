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
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <errno.h>

#include <cluster_ipc.h>
#include "ipc_internal.h"

#define IPC_CLIENT_USAGE_MAX_NUM (4)
#define IPC_CLIENT_EPOLL_WAIT_NUM (IPC_CLIENT_USAGE_MAX_NUM + 1)
#define IPC_CLIENT_CONNECT_CHECK_TIME (500) // msec

// == Internal global values ==
static bool g_initedFlag = false;
static pthread_t g_clientThread;
static bool g_threadRunning = false;
static int g_threadCtlPipeFd[2] = {-1, -1};
static int g_epollFd = -1;

typedef struct {
    IPC_USAGE_TYPE_E usage;
    int serverFd;
    void *pDataPool;
    int poolSize;
    IPC_CHANGE_NOTIFY_CB changeNotifyCb;
} IPC_CLIENT_INFO_S;
static IPC_CLIENT_INFO_S g_clientInfo[IPC_CLIENT_USAGE_MAX_NUM];

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// == Prototype declaration
static void *ipcClientThread(void *arg);
static int ipcClientInit(void);
static int ipcClientDeinit(void);
static void ipcClientInfoClear(int index);
static int ipcGetClientInfoIndex(IPC_USAGE_TYPE_E usageType);
static int ipcClientCreateSocket(IPC_USAGE_TYPE_E usageType);
static void ipcCloseConnectFromServer(int eventFd);
static void ipcReceiveDataFromServer(int eventFd, int *pIndex, void *pLocalDataPool);
static void ipcCheckChangeAndCallback(int index, void *pLocalDataPool);
static void ipcWriteToDataPool(int index, void *pLocalDataPool);
static int ipcAddClient(IPC_USAGE_TYPE_E usageType);
static int ipcRemoveClient(IPC_USAGE_TYPE_E usageType);
static int ipcCountClient(void);

// == Thread function ==
static void *ipcClientThread(void *arg)
{
    int fdNum;
    struct epoll_event epEvents[IPC_CLIENT_USAGE_MAX_NUM];
    int i;
    int index;
    IPC_ALL_USAGE_DATA_POOL_U localDataPool;
    char dummy;
    int rc;

    while(g_threadRunning != false) {
        fdNum = epoll_wait(g_epollFd, epEvents, IPC_CLIENT_USAGE_MAX_NUM, -1);
        if (g_threadRunning == false) {
            break;
        }

        pthread_mutex_lock(&g_mutex);
        for (i = 0; i < fdNum; i++) {
            if (epEvents[i].data.fd == g_threadCtlPipeFd[0]) {
                // dummy notify from API function.
                rc = read(g_threadCtlPipeFd[0], &dummy, 1);
                if (rc < 0) {
                    printf("[##ERROR##] %s:%s:%d (%s) is false. (%s=%d)\n", __FILE__, __func__, __LINE__, "rc >= 0", "rc", (int)rc);
                    continue;
                }
            }
            else {
                if (epEvents[i].events & EPOLLRDHUP) {
                    ipcCloseConnectFromServer(epEvents[i].data.fd);
                }
                else if (epEvents[i].events & EPOLLIN) {
                    ipcReceiveDataFromServer(epEvents[i].data.fd, &index, (void *)&localDataPool);
                    if (index >= 0) {
                        ipcCheckChangeAndCallback(index, &localDataPool);
                        ipcWriteToDataPool(index, &localDataPool);
                    }
                }
            }
        }
        pthread_mutex_unlock(&g_mutex);
    }

    pthread_exit(NULL);
    return NULL;
}

// == Internal function ==
static int ipcClientInit(void)
{
    int ret = -1;
    int rc;
    int i;
    struct epoll_event epollEv;

    if (g_initedFlag == false) {
        for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
            ipcClientInfoClear(i);
        }
        g_threadRunning = false;
        rc = pipe(g_threadCtlPipeFd);
        IPC_E_CHECK(rc == 0, rc, end);

        g_epollFd = epoll_create(IPC_CLIENT_EPOLL_WAIT_NUM);
        IPC_E_CHECK(g_epollFd >= 0, g_epollFd, end);

        epollEv.events = EPOLLIN;
        epollEv.data.fd = g_threadCtlPipeFd[0];
        epoll_ctl(g_epollFd, EPOLL_CTL_ADD, epollEv.data.fd, &epollEv);

        g_initedFlag = true;
    }

    ret = 0;

end:
    if (ret == -1) {
        for (i = 0; i < 2; i++) {
            if (g_threadCtlPipeFd[i] >= 0) {
                close(g_threadCtlPipeFd[i]);
                g_threadCtlPipeFd[i] = -1;
            }
        }
    }
    return ret;
}

static int ipcClientDeinit(void)
{
    int i;

    if (g_initedFlag == true) {
        if (g_threadRunning == true) {
            g_threadRunning = false;
            pthread_cancel(g_clientThread);
            pthread_join(g_clientThread, NULL);
        }
        for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
            ipcClientInfoClear(i);
        }
        for (i = 0; i < 2; i++) {
            if (g_threadCtlPipeFd[i] >= 0) {
                close(g_threadCtlPipeFd[i]);
                g_threadCtlPipeFd[i] = -1;
            }
        }
        close(g_epollFd);
        g_epollFd = -1;

        g_initedFlag = false;
    }

    return 0;
}

static void ipcClientInfoClear(int index)
{
    IPC_E_CHECK(0 <= index && index < IPC_CLIENT_USAGE_MAX_NUM, index, end);

    g_clientInfo[index].usage = IPC_USAGE_TYPE_MAX;
    g_clientInfo[index].serverFd = -1;
    g_clientInfo[index].pDataPool = NULL;
    g_clientInfo[index].poolSize = 0;
    g_clientInfo[index].changeNotifyCb = NULL;

end:
    return;
}

static int ipcGetClientInfoIndex(IPC_USAGE_TYPE_E usageType)
{
    int index = -1;
    int i;

    for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
        if (g_clientInfo[i].usage == usageType) {
            index = i;
            break;
        }
    }

    return index;
}

static int ipcClientCreateSocket(IPC_USAGE_TYPE_E usageType)
{
    int rc;
    int fd = -1;
    struct sockaddr_un unixAddr;
    int len;
    char domainName[IPC_DOMAIN_PATH_MAX] = "";
    int domainLen = IPC_DOMAIN_PATH_MAX;

    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, err);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    IPC_E_CHECK(fd >= 0, fd, err);

    rc = ipcCreateDomainName(usageType, domainName, &domainLen);
    IPC_E_CHECK(rc == 0, rc, err);

    rc = ipcCreateUnixDomainAddr(domainName, &unixAddr, &len);
    IPC_E_CHECK(rc == 0, rc, err);

    rc = connect(fd, (struct sockaddr *)&unixAddr, len);
    IPC_E_CHECK(rc == 0, rc, err);

    return fd;

err:
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    return -1;
}

static void ipcCloseConnectFromServer(int eventFd)
{
    int index;
    IPC_CLIENT_INFO_S *pInfo;
    struct epoll_event epollEv;

    for (index = 0; index < IPC_CLIENT_USAGE_MAX_NUM; index++) {
        pInfo = &(g_clientInfo[index]);
        if (pInfo->usage == IPC_USAGE_TYPE_MAX) {
            continue;
        }

        if (pInfo->serverFd == eventFd) {
            shutdown(pInfo->serverFd, SHUT_RDWR);
            close(pInfo->serverFd);
            if (pInfo->pDataPool != NULL) {
                free(pInfo->pDataPool);
                pInfo->pDataPool = NULL;
            }
            memset(&epollEv, 0, sizeof(epollEv));
            epoll_ctl(g_epollFd, EPOLL_CTL_DEL, pInfo->serverFd, &epollEv);
            ipcClientInfoClear(index);
        }
    }
}

static void ipcReceiveDataFromServer(int eventFd, int *pIndex, void *pLocalDataPool)
{
    int rc;
    int i;
    IPC_CLIENT_INFO_S *pInfo = NULL;

    *pIndex = -1;

    // check fd
    for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
        if (g_clientInfo[i].serverFd == eventFd) {
            pInfo = &(g_clientInfo[i]);
            break;
        }
    }

    IPC_E_CHECK(pInfo != NULL, eventFd, end);
    IPC_E_CHECK(pInfo->pDataPool != NULL, i, end);
    IPC_E_CHECK(pInfo->poolSize > 0, i, end);

    // receive from server and write to data pool.
    rc = recv(eventFd, pLocalDataPool, pInfo->poolSize, 0);
    if ((rc == 0)
        || (rc >= 0 && errno == ECONNREFUSED)) {
        ipcCloseConnectFromServer(eventFd);
    }
    IPC_E_CHECK(rc >= 0, errno, end);

    *pIndex = i;

end:
    return;
}

static void ipcCheckChangeAndCallback(int index, void *pLocalDataPool)
{
    IPC_CLIENT_INFO_S *pInfo = NULL;
    IPC_CHECK_CHANGE_INFO_TABLE_S *pChangeInfoTbl = NULL;
    IPC_CHECK_CHANGE_INFO_S *pChangeInfo = NULL;
    int i;
    void *pMemCmpData, *pMemCmpLocal;

    pInfo = &(g_clientInfo[index]);
    if (pInfo->changeNotifyCb == NULL) {
        goto end;
    }

    pChangeInfoTbl = &(g_ipcCheckChangeInfoTbl[pInfo->usage]);

    // Check for changes in the data pool.
    for (i = 0; i < pChangeInfoTbl->num; i++) {
        pChangeInfo = &(pChangeInfoTbl->pInfo[i]);
        pMemCmpData = pInfo->pDataPool + pChangeInfo->offset;
        pMemCmpLocal = pLocalDataPool + pChangeInfo->offset;

        if (0 != memcmp(pMemCmpData, pMemCmpLocal, pChangeInfo->size)) {
            pInfo->changeNotifyCb(pMemCmpLocal, pChangeInfo->size, pChangeInfo->kind);
        }
    }

end:
    return;
}

static void ipcWriteToDataPool(int index, void *pLocalDataPool)
{
    IPC_CLIENT_INFO_S *pInfo = NULL;

    pInfo = &(g_clientInfo[index]);

    memcpy(pInfo->pDataPool, pLocalDataPool, pInfo->poolSize);

    return;
}

static int ipcAddClient(IPC_USAGE_TYPE_E usageType)
{
    int ret = -1;
    int index = -1;
    int i;
    IPC_CLIENT_INFO_S *pInfo;
    int fd;
    void *pDataPool = NULL;
    int dataPoolSize;
    struct epoll_event epollEv;

    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    // check if the usageType is already used
    index = ipcGetClientInfoIndex(usageType);
    IPC_E_CHECK(index == -1, usageType, end);

    // find empty index
    for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
        pInfo = &(g_clientInfo[i]);
        if (pInfo->usage == IPC_USAGE_TYPE_MAX) {
            index = i;
            break;
        }
    }

    IPC_E_CHECK(index >= 0, i, end);
    pInfo = &(g_clientInfo[index]);

    dataPoolSize = g_ipcDomainInfoList[usageType].size;
    pDataPool = malloc(dataPoolSize);
    IPC_E_CHECK(pDataPool != NULL, 0, end);
    memset(pDataPool, 0, dataPoolSize);

    fd = ipcClientCreateSocket(usageType);

    IPC_E_CHECK(fd >= 0, usageType, end);

    pInfo->usage = usageType;
    pInfo->serverFd = fd;
    pInfo->pDataPool = pDataPool;
    pInfo->poolSize = dataPoolSize;

    memset(&epollEv, 0, sizeof(epollEv));
    epollEv.events = EPOLLIN | EPOLLRDHUP;
    epollEv.data.fd = fd;
    epoll_ctl(g_epollFd, EPOLL_CTL_ADD, epollEv.data.fd, &epollEv);

    ret = 0;
end:
    if (ret != 0 && pDataPool != NULL) {
        free(pDataPool);
    }
    return ret;
}

static int ipcRemoveClient(IPC_USAGE_TYPE_E usageType)
{
    int ret = -1;
    int index;
    IPC_CLIENT_INFO_S *pInfo;
    struct epoll_event epollEv;

    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    index = ipcGetClientInfoIndex(usageType);
    IPC_E_CHECK(index >= 0, usageType, end);

    pInfo = &(g_clientInfo[index]);

    shutdown(pInfo->serverFd, SHUT_RDWR);
    close(pInfo->serverFd);
    if (pInfo->pDataPool != NULL) {
        free(pInfo->pDataPool);
        pInfo->pDataPool = NULL;
    }

    memset(&epollEv, 0, sizeof(epollEv));
    epoll_ctl(g_epollFd, EPOLL_CTL_DEL, pInfo->serverFd, &epollEv);

    ipcClientInfoClear(index);

    ret = 0;

end:
    return ret;
}

static int ipcCountClient(void)
{
    int count = 0;
    int i;

    for (i = 0; i < IPC_CLIENT_USAGE_MAX_NUM; i++) {
        if (g_clientInfo[i].usage != IPC_USAGE_TYPE_MAX) {
            count++;
        }
    }

    return count;
}

// == API function for client ==
IPC_RET_E ipcClientStart(IPC_USAGE_TYPE_E usageType)
{
    IPC_RET_E ret;
    int rc;
    char dummy = 's';
    int index;

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    ret = IPC_ERR_OTHER;
    rc = ipcClientInit();
    IPC_E_CHECK(rc == 0, rc, end);

    pthread_mutex_lock(&g_mutex);
    rc = ipcAddClient(usageType);
    pthread_mutex_unlock(&g_mutex);

    ret = IPC_ERR_NO_RESOURCE;
    IPC_E_CHECK(rc == 0, rc, end);

    if (g_threadRunning == false) {
        rc = pthread_create(&g_clientThread, NULL, ipcClientThread, NULL);
        IPC_E_CHECK(rc == 0, rc, end);

        g_threadRunning = true;
    }

    rc = write(g_threadCtlPipeFd[1], &dummy, 1); // for wakeup epoll_wait
    IPC_E_CHECK(rc >= 0, rc, end);

    // Check to if the connection is rejected.
    usleep(IPC_CLIENT_EPOLL_WAIT_NUM * 1000);
    pthread_mutex_lock(&g_mutex);
    index = ipcGetClientInfoIndex(usageType);
    pthread_mutex_unlock(&g_mutex);
    ret = IPC_ERR_NO_RESOURCE;
    IPC_E_CHECK(index >= 0, usageType, end);

    ret = IPC_RET_OK;

end:
    if (ret != IPC_RET_OK && g_threadRunning == true) {
        pthread_mutex_lock(&g_mutex);
        if (ipcCountClient() == 0) {
            pthread_mutex_unlock(&g_mutex);
            ipcClientDeinit();
        }
        else {
            pthread_mutex_unlock(&g_mutex);
        }
    }
    return ret;
}

IPC_RET_E ipcReadDataPool(IPC_USAGE_TYPE_E usageType, void* pData, signed int* pSize)
{
    IPC_RET_E ret;
    int index = -1;
    IPC_CLIENT_INFO_S *pInfo;

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);
    IPC_E_CHECK(pData != NULL, 0, end);
    IPC_E_CHECK(pSize != NULL, 0, end);

    pthread_mutex_lock(&g_mutex);
    index = ipcGetClientInfoIndex(usageType);

    ret = IPC_ERR_SEQUENCE;
    IPC_E_CHECK(index >= 0, usageType, end_with_unlock);
    pInfo = &(g_clientInfo[index]);
    IPC_E_CHECK(pInfo->pDataPool != NULL, usageType, end_with_unlock);
    IPC_E_CHECK(*pSize >= pInfo->poolSize, *pSize, end_with_unlock);

    memcpy(pData, pInfo->pDataPool, pInfo->poolSize);

    ret = IPC_RET_OK;

end_with_unlock:
    pthread_mutex_unlock(&g_mutex);

end:
    return ret;
}

IPC_RET_E ipcRegisterCallback(IPC_USAGE_TYPE_E usageType, IPC_CHANGE_NOTIFY_CB changeNotifyCb)
{
    IPC_RET_E ret;
    int index = -1;

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);
    IPC_E_CHECK(changeNotifyCb != NULL, 0, end);

    pthread_mutex_lock(&g_mutex);
    index = ipcGetClientInfoIndex(usageType);

    ret = IPC_ERR_SEQUENCE;
    IPC_E_CHECK(index >= 0, usageType, end_with_unlock);

    g_clientInfo[index].changeNotifyCb = changeNotifyCb;

    ret = IPC_RET_OK;

end_with_unlock:
    pthread_mutex_unlock(&g_mutex);

end:
    return ret;
}

IPC_RET_E ipcClientStop(IPC_USAGE_TYPE_E usageType)
{
    IPC_RET_E ret;
    int rc;
    char dummy = 'e';

    ret = IPC_ERR_SEQUENCE;
    IPC_E_CHECK(g_initedFlag != false, g_initedFlag, end);

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    pthread_mutex_lock(&g_mutex);
    rc = ipcRemoveClient(usageType);
    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(rc == 0, rc, end_with_unlock);

    rc = write(g_threadCtlPipeFd[1], &dummy, 1); // for wakeup epoll_wait
    IPC_E_CHECK(rc >= 0, rc, end_with_unlock);

    if (ipcCountClient() == 0) {
        pthread_mutex_unlock(&g_mutex);
        ipcClientDeinit();
    }
    else {
        pthread_mutex_unlock(&g_mutex);
    }

    ret = IPC_RET_OK;

end:
    return ret;

end_with_unlock:
    pthread_mutex_unlock(&g_mutex);
    return ret;
}

