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
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <cluster_ipc.h>

#include "ipc_internal.h"

#define IPC_SERVER_USAGE_MAX_NUM (1)
#define IPC_LISTEN_CLIENT_NUM (4)
#define IPC_SERVER_EPOLL_WAIT_NUM (IPC_SERVER_USAGE_MAX_NUM * IPC_LISTEN_CLIENT_NUM + 1)

// == Internal global values ==
static bool g_initedFlag = false;
static pthread_t g_serverThread;
static bool g_threadRunning = false;
static int g_threadCtlPipeFd[2] = {-1, -1};
static int g_epollFd = -1;

typedef struct {
    IPC_USAGE_TYPE_E usage;
    int fd;
    int clientFd[IPC_LISTEN_CLIENT_NUM];
} IPC_SERVER_INFO_S;
static IPC_SERVER_INFO_S g_serverInfo[IPC_SERVER_USAGE_MAX_NUM];

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// == Prototype declaration
static void *ipcServerThread(void *arg);
static int ipcServerInit(void);
static int ipcServerDeinit(void);
static void ipcServerInfoClear(int index);
static int ipcGetServerInfoIndex(IPC_USAGE_TYPE_E usageType);
static int ipcServerCreateSocket(IPC_USAGE_TYPE_E usageType);
static void ipcAcceptClient(int eventFd);
static void ipcCloseClient(int eventFd);
static int ipcAddServer(IPC_USAGE_TYPE_E usageType);
static int ipcAddConnectClient(int index, int clientFd);
static int ipcRemoveServer(IPC_USAGE_TYPE_E usageType);
static int ipcCountServer(void);

// == Thread function ==
static void *ipcServerThread(void *arg)
{
    int fdNum;
    struct epoll_event epEvents[IPC_SERVER_EPOLL_WAIT_NUM];
    int i;
    char dummy;
    int rc;

    while(g_threadRunning != false) {
        fdNum = epoll_wait(g_epollFd, epEvents, IPC_SERVER_USAGE_MAX_NUM, -1);
        if (g_threadRunning == false) {
            break;
        }

        pthread_mutex_lock(&g_mutex);
        for (i = 0; i < fdNum; i++) {
            if (epEvents[i].data.fd == g_threadCtlPipeFd[0]) {
                // dummy notify from API function.
                rc = read(g_threadCtlPipeFd[0], &dummy, 1);
                if (rc < 0) {
                    printf("[##ERROR##] %s:%s:%d (%s) is false. (%s=%d)\n", __FILE__, __func__, __LINE__, "rc < 0", "rc", (int)rc);
                    continue;
                }
            }
            else {
                if (epEvents[i].events & EPOLLRDHUP) {
                    ipcCloseClient(epEvents[i].data.fd);
                }
                else if (epEvents[i].events & EPOLLIN) {
                    ipcAcceptClient(epEvents[i].data.fd);
                }
            }
        }
        pthread_mutex_unlock(&g_mutex);
    }

    pthread_exit(NULL);
    return NULL;
}

// == Internal function ==
static int ipcServerInit(void)
{
    int ret = -1;
    int rc;
    int i;
    struct epoll_event epollEv;

    if (g_initedFlag == false) {
        for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
            ipcServerInfoClear(i);
        }
        g_threadRunning = false;
        rc = pipe(g_threadCtlPipeFd);
        IPC_E_CHECK(rc == 0, rc, end);

        g_epollFd = epoll_create(IPC_SERVER_EPOLL_WAIT_NUM);
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

static int ipcServerDeinit(void)
{
    int i;

    if (g_initedFlag == true) {
        if (g_threadRunning == true) {
            g_threadRunning = false;
            pthread_cancel(g_serverThread);
            pthread_join(g_serverThread, NULL);
        }
        for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
            ipcServerInfoClear(i);
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

static void ipcServerInfoClear(int index)
{
    int i;

    IPC_E_CHECK(0 <= index && index < IPC_SERVER_USAGE_MAX_NUM, index, end);

    g_serverInfo[index].usage = IPC_USAGE_TYPE_MAX;
    g_serverInfo[index].fd = -1;
    for (i = 0; i < IPC_LISTEN_CLIENT_NUM; i++) {
        g_serverInfo[index].clientFd[i] = -1;
    }

end:
    return;
}

static int ipcGetServerInfoIndex(IPC_USAGE_TYPE_E usageType)
{
    int index = -1;
    int i;

    for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
        if (g_serverInfo[i].usage == usageType) {
            index = i;
            break;
        }
    }

    return index;
}

static int ipcServerCreateSocket(IPC_USAGE_TYPE_E usageType)
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

    rc = bind(fd, (struct sockaddr *)&unixAddr, len);
    IPC_E_CHECK(rc == 0, rc, err);

    rc = listen(fd, IPC_LISTEN_CLIENT_NUM);
    IPC_E_CHECK(rc == 0, rc, err);

    return fd;

err:
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        unlink(domainName);
    }
    return -1;
}

static void ipcAcceptClient(int eventFd)
{
    int rc;
    int clientFd;
    char domainName[IPC_DOMAIN_PATH_MAX] = "";
    int domainLen = IPC_DOMAIN_PATH_MAX;
    struct sockaddr_un unixAddr;
    int len;
    IPC_SERVER_INFO_S *pInfo;
    int index = -1;
    int i;
    struct epoll_event epollEv;

    for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
        if (eventFd == g_serverInfo[i].fd) {
            index = i;
            break;
        }
    }
    IPC_E_CHECK(0 <= index && index < IPC_SERVER_USAGE_MAX_NUM, eventFd, end);
    pInfo = &(g_serverInfo[index]);

    rc = ipcCreateDomainName(pInfo->usage, domainName, &domainLen);
    IPC_E_CHECK(rc == 0, rc, end);

    rc = ipcCreateUnixDomainAddr(domainName, &unixAddr, &len);
    IPC_E_CHECK(rc == 0, rc, end);

    // check connect client
    clientFd = accept(pInfo->fd, (struct sockaddr*)&unixAddr, (socklen_t *)&len);
    if (clientFd >= 0) {
        rc = ipcAddConnectClient(index, clientFd);
        if (rc == 0) {
            memset(&epollEv, 0, sizeof(epollEv));
            epollEv.events = EPOLLRDHUP;
            epollEv.data.fd = clientFd;
            epoll_ctl(g_epollFd, EPOLL_CTL_ADD, clientFd, &epollEv);
        }
        else { // The number of connections is already limited.
            shutdown(clientFd, SHUT_RDWR);
            close(clientFd);
        }
    }

end:
    return;
}

static void ipcCloseClient(int eventFd)
{
    int i;
    int index;
    IPC_SERVER_INFO_S *pInfo;
    struct epoll_event epollEv;

    for (index = 0; index < IPC_SERVER_USAGE_MAX_NUM; index++) {
        pInfo = &(g_serverInfo[index]);
        if (pInfo->usage == IPC_USAGE_TYPE_MAX) {
            continue;
        }

        for (i = 0; i < IPC_LISTEN_CLIENT_NUM; i++) {
            if (pInfo->clientFd[i] == eventFd) {
                pInfo->clientFd[i] = -1;

                shutdown(eventFd, SHUT_RDWR);
                close(eventFd);

                memset(&epollEv, 0, sizeof(epollEv));
                epoll_ctl(g_epollFd, EPOLL_CTL_DEL, eventFd, &epollEv);

                break;
            }
        }
    }
}

static int ipcAddServer(IPC_USAGE_TYPE_E usageType)
{
    int ret = -1;
    int index = -1;
    int i;
    int fd;
    IPC_SERVER_INFO_S *pInfo;
    struct epoll_event epollEv;

    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    // check if the usageType is used
    index = ipcGetServerInfoIndex(usageType);
    IPC_E_CHECK(index == -1, usageType, end);

    // find empty index
    for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
        pInfo = &(g_serverInfo[i]);
        if (pInfo->usage == IPC_USAGE_TYPE_MAX) {
            pInfo->usage = usageType;
            index = i;
            break;
        }
    }
    IPC_E_CHECK(index >= 0, i, end);
    pInfo = &(g_serverInfo[index]);

    fd = ipcServerCreateSocket(usageType);

    IPC_E_CHECK(fd >= 0, usageType, end);

    pInfo->fd = fd;
    memset(&epollEv, 0, sizeof(epollEv));
    epollEv.events = EPOLLIN;
    epollEv.data.fd = fd;
    epoll_ctl(g_epollFd, EPOLL_CTL_ADD, epollEv.data.fd, &epollEv);

    ret = 0;

end:
    if (ret == -1 && index >= 0) {
        ipcServerInfoClear(index);
    }
    return ret;
}

static int ipcAddConnectClient(int index, int clientFd)
{
    int ret = -1;
    int i;
    IPC_SERVER_INFO_S *pInfo;

    if (clientFd < 0) {
        // do nothing
        goto end;
    }

    pInfo = &(g_serverInfo[index]);

    // find empty index
    for (i = 0; i < IPC_LISTEN_CLIENT_NUM; i++) {
        if (pInfo->clientFd[i] == -1) {
            pInfo->clientFd[i] = clientFd;
            ret = 0;
            break;
        }
    }

end:
    return ret;
}

static int ipcRemoveServer(IPC_USAGE_TYPE_E usageType)
{
    int ret = -1;
    int rc;
    int index;
    char domainName[IPC_DOMAIN_PATH_MAX] = "";
    int domainLen = IPC_DOMAIN_PATH_MAX;
    IPC_SERVER_INFO_S *pInfo;
    struct epoll_event epollEv;

    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    index = ipcGetServerInfoIndex(usageType);
    IPC_E_CHECK(index >= 0, usageType, end);

    pInfo = &(g_serverInfo[index]);

    rc = ipcCreateDomainName(pInfo->usage, domainName, &domainLen);
    IPC_E_CHECK(rc == 0, rc, end);

    shutdown(pInfo->fd, SHUT_RDWR);
    close(pInfo->fd);
    unlink(domainName);

    memset(&epollEv, 0, sizeof(epollEv));
    epoll_ctl(g_epollFd, EPOLL_CTL_DEL, pInfo->fd, &epollEv);

    ipcServerInfoClear(index);

    ret = 0;

end:
    return ret;
}

static int ipcCountServer(void)
{
    int count = 0;
    int i;

    for (i = 0; i < IPC_SERVER_USAGE_MAX_NUM; i++) {
        if (g_serverInfo[i].usage != IPC_USAGE_TYPE_MAX) {
            count++;
        }
    }

    return count;
}

// == API function for server ==
IPC_RET_E ipcServerStart(IPC_USAGE_TYPE_E usageType)
{
    IPC_RET_E ret;
    int rc;
    char dummy = 's';

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    ret = IPC_ERR_OTHER;
    rc = ipcServerInit();
    IPC_E_CHECK(rc == 0, rc, end);

    pthread_mutex_lock(&g_mutex);
    rc = ipcAddServer(usageType);
    pthread_mutex_unlock(&g_mutex);

    ret = IPC_ERR_NO_RESOURCE;
    IPC_E_CHECK(rc == 0, rc, end);

    if (g_threadRunning == false) {
        rc = pthread_create(&g_serverThread, NULL, ipcServerThread, NULL);
        IPC_E_CHECK(rc == 0, rc, end);

        g_threadRunning = true;
    }

    rc = write(g_threadCtlPipeFd[1], &dummy, 1); // for wakeup epoll_wait
    IPC_E_CHECK(rc >= 0, rc, end);
    ret = IPC_RET_OK;

end:
    return ret;
}

IPC_RET_E ipcSendMessage(IPC_USAGE_TYPE_E usageType, const void* pData, signed int size)
{
    IPC_RET_E ret;
    int rc;
    int index;
    IPC_SERVER_INFO_S *pInfo = NULL;
    int i;
    int clientFd;

    ret = IPC_ERR_SEQUENCE;
    IPC_E_CHECK(g_initedFlag != false, g_initedFlag, end);

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);
    IPC_E_CHECK(pData != NULL, 0, end);
    IPC_E_CHECK(g_ipcDomainInfoList[usageType].size >= size, size, end);

    pthread_mutex_lock(&g_mutex);
    index = ipcGetServerInfoIndex(usageType);

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(index >= 0, usageType, end_with_unlock);
    pInfo = &(g_serverInfo[index]);

    IPC_E_CHECK(pInfo->fd >= 0, usageType, end_with_unlock);

    // Send to All Client
    for (i = 0; i < IPC_LISTEN_CLIENT_NUM; i++) {
        clientFd = pInfo->clientFd[i];
        if (clientFd == -1) {
            continue;
        }
        rc = write(clientFd, pData, size);
        IPC_E_CHECK(rc >= 0, rc, end_with_unlock);
    }

    ret = IPC_RET_OK;
end_with_unlock:
    pthread_mutex_unlock(&g_mutex);

end:
    return ret;
}

IPC_RET_E ipcServerStop(IPC_USAGE_TYPE_E usageType)
{
    IPC_RET_E ret;
    int rc;
    char dummy = 'e';

    ret = IPC_ERR_SEQUENCE;
    IPC_E_CHECK(g_initedFlag != false, g_initedFlag, end);

    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(CHECK_VALID_USAGE(usageType), usageType, end);

    pthread_mutex_lock(&g_mutex);
    rc = ipcRemoveServer(usageType);
    ret = IPC_ERR_PARAM;
    IPC_E_CHECK(rc == 0, rc, end_with_unlock);

    rc = write(g_threadCtlPipeFd[1], &dummy, 1); // for wakeup epoll_wait
    IPC_E_CHECK(rc >= 0, rc, end_with_unlock);

    if (ipcCountServer() == 0) {
        pthread_mutex_unlock(&g_mutex);
        ipcServerDeinit();
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

