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


#include <ipc.h>
#include "ipc_internal.h"

#define DEFINE_OFFSET_SIZE(struct_name, member, kind) \
    {kind, offsetof(struct_name, member), sizeof(((struct_name *)0)->member)}

#define DEFINE_CHANGE_INFO_TABLE(changeInfoName) \
    {changeInfoName, sizeof(changeInfoName) / sizeof(changeInfoName[0])}

// == check change table ==
//   for IPC_USAGE_TYPE_IC_SERVICE
static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeIcService[] = {
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnR, IPC_KIND_ICS_TURN_R),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnL, IPC_KIND_ICS_TURN_L),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, brake, IPC_KIND_ICS_BRAKE),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, seatbelt, IPC_KIND_ICS_SEATBELT),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, highbeam, IPC_KIND_ICS_HIGHBEAM),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, door, IPC_KIND_ICS_DOOR),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, eps, IPC_KIND_ICS_EPS),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, srsAirbag, IPC_KIND_ICS_SRS_AIRBAG),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, abs, IPC_KIND_ICS_ABS),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, lowBattery, IPC_KIND_ICS_LOW_BATTERY),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, oilPress, IPC_KIND_ICS_OIL_PRESS),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, engine, IPC_KIND_ICS_ENGINE),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, fuel, IPC_KIND_ICS_FUEL),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, immobi, IPC_KIND_ICS_IMMOBI),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, tmFail, IPC_KIND_ICS_TM_FAIL),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, espAct, IPC_KIND_ICS_ESP_ACT),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, espOff, IPC_KIND_ICS_ESP_OFF),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, adaptingLighting, IPC_KIND_ICS_ADAPTING_LIGHTING),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, autoStop, IPC_KIND_ICS_AUTO_STOP),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, autoStopFail, IPC_KIND_ICS_AUTO_STOP_FAIL),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, parkingLights, IPC_KIND_ICS_PARKING_LIGHTS),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, frontFog, IPC_KIND_ICS_FRONT_FOG),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, exteriorLightFault, IPC_KIND_ICS_EXTERIOR_LIGHT_FAULT),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, accFail, IPC_KIND_ICS_ACC_FAIL),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, ldwOff, IPC_KIND_ICS_LDW_OFF),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, hillDescent, IPC_KIND_ICS_HILL_DESCENT),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, autoHiBeamGreen, IPC_KIND_ICS_AUTO_HI_BEAM_GREEN),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, autoHiBeamAmber, IPC_KIND_ICS_AUTO_HI_BEAM_AMBER),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, ldwOperate, IPC_KIND_ICS_LDW_OPERATE),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, generalWarn, IPC_KIND_ICS_GENERAL_WARN),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, sportsMode, IPC_KIND_ICS_SPORTS_MODE),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, drivingPowerMode, IPC_KIND_ICS_DRIVING_POWER_MODE),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, hotTemp, IPC_KIND_ICS_HOT_TEMP),
    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, lowTemp, IPC_KIND_ICS_LOW_TEMP)
};

//   for IPC_USAGE_TYPE_FOR_TEST
static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeForTest[] = {
    DEFINE_OFFSET_SIZE(IPC_DATA_FOR_TEST_S, test, IPC_KIND_TEST_TEST)
};

// == usage info table ==
//   index of [] is IPC_USAGE_TYPE_E
IPC_DOMAIN_INFO_S g_ipcDomainInfoList[] =
{
    {sizeof(IPC_DATA_IC_SERVICE_S), "ipcIcService"},
    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"}
};

IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[] = {
    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeIcService),
    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest)
};

