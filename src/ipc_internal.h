#ifndef IPC_INTERNAL_H
#define IPC_INTERNAL_H

#include <ipc_protocol.h>
#include <stddef.h>
#include <sys/un.h>

#define IPC_E_CHECK(condition, value, label) \
    do { \
        if (!(condition)) { \
            printf("[##ERROR##] %s:%s:%d (%s) is false. (%s=%d)\n", __FILE__, __func__, __LINE__, #condition, #value, (int)value); \
            goto label; \
        } \
    } while(0)

#define CHECK_VALID_USAGE(usageType) \
    (0 <= usageType && usageType < IPC_USAGE_TYPE_MAX)

#define IPC_DOMAIN_PATH_MAX (108) // defined in sun_path[] of sys/un.h

typedef struct {
    signed long size;
    char *domainName;
} IPC_DOMAIN_INFO_S;

typedef struct {
    int kind;
    int offset;
    int size;
} IPC_CHECK_CHANGE_INFO_S;

typedef struct {
    IPC_CHECK_CHANGE_INFO_S* pInfo;
    int num;
} IPC_CHECK_CHANGE_INFO_TABLE_S;

// the union to know the maximum size of the data pool.
typedef union {
    IPC_DATA_IC_SERVICE_S icService;
} IPC_ALL_USAGE_DATA_POOL_U;

extern IPC_DOMAIN_INFO_S g_ipcDomainInfoList[];
extern IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[];

int ipcCreateDomainName(IPC_USAGE_TYPE_E usageType, char *pOutName, int *pSize);
int ipcCreateUnixDomainAddr(const char *domainName, struct sockaddr_un *pOutUnixAddr, int *pOutLen);

#endif // IPC_INTERNAL_H
