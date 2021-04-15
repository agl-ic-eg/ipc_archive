#ifndef IPC_UNIT_TEST_COMMON_H
#define IPC_UNIT_TEST_COMMON_H
#include <stddef.h>

#define DEFINE_STRUCT_DATA(struct_name, member) \
    {#member, offsetof(struct_name, member), sizeof(((struct_name *)0)->member)}
#define IC_SERVICE_LIST_NUM (77)

typedef struct {
    const char *name;
    int offset;
    int size;
} IPC_UNIT_TEST_DATA_LIST;

extern IPC_UNIT_TEST_DATA_LIST IcServiceList[];

#endif // IPC_UNIT_TEST_COMMON_H
