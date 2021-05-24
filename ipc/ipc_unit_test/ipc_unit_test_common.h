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
