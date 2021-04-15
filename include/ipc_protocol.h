#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

// TODO: Write something

typedef enum {
    IPC_USAGE_TYPE_IC_SERVICE = 0,
    IPC_USAGE_TYPE_FOR_TEST,
    IPC_USAGE_TYPE_MAX
} IPC_USAGE_TYPE_E;

// for IPC_USAGE_TYPE_IC_SERVICE
typedef enum {
    IPC_KIND_ICS_TURN_R = 0,
    IPC_KIND_ICS_TURN_L,
    IPC_KIND_ICS_BRAKE,
    IPC_KIND_ICS_SEATBELT,
    IPC_KIND_ICS_HIGHBEAM,
    IPC_KIND_ICS_DOOR,
    IPC_KIND_ICS_EPS,
    IPC_KIND_ICS_SRS_AIRBAG,
    IPC_KIND_ICS_ABS,
    IPC_KIND_ICS_LOW_BATTERY,
    IPC_KIND_ICS_OIL_PRESS,
    IPC_KIND_ICS_ENGINE,
    IPC_KIND_ICS_FUEL,
    IPC_KIND_ICS_IMMOBI,
    IPC_KIND_ICS_TM_FAIL,
    IPC_KIND_ICS_ESP_ACT,
    IPC_KIND_ICS_ESP_OFF,
    IPC_KIND_ICS_ADAPTING_LIGHTING,
    IPC_KIND_ICS_AUTO_STOP,
    IPC_KIND_ICS_AUTO_STOP_FAIL,
    IPC_KIND_ICS_PARKING_LIGHTS,
    IPC_KIND_ICS_FRONT_FOG,
    IPC_KIND_ICS_EXTERIOR_LIGHT_FAULT,
    IPC_KIND_ICS_ACC_FAIL,
    IPC_KIND_ICS_LDW_OFF,
    IPC_KIND_ICS_HILL_DESCENT,
    IPC_KIND_ICS_AUTO_HI_BEAM_GREEN,
    IPC_KIND_ICS_AUTO_HI_BEAM_AMBER,
    IPC_KIND_ICS_LDW_OPERATE,
    IPC_KIND_ICS_GENERAL_WARN,
    IPC_KIND_ICS_SPORTS_MODE,
    IPC_KIND_ICS_DRIVING_POWER_MODE,
    IPC_KIND_ICS_HOT_TEMP,
    IPC_KIND_ICS_LOW_TEMP
} IPC_KIND_IC_SERVICE_E;

typedef struct {
    // Telltale
    signed int turnR;
    signed int turnL;
    signed int brake;
    signed int seatbelt;
    signed int frontRightSeatbelt;
    signed int frontCenterSeatbelt;
    signed int frontLeftSeatbelt;
    signed int mid1RightSeatbelt;
    signed int mid1CenterSeatbelt;
    signed int mid1LeftSeatbelt;
    signed int mid2RightSeatbelt;
    signed int mid2CenterSeatbelt;
    signed int mid2LeftSeatbelt;
    signed int rearRightSeatbelt;
    signed int rearCenterSeatbelt;
    signed int rearLeftSeatbelt;
    signed int highbeam;
    signed int door;
    signed int frontRightDoor;
    signed int frontLeftDoor;
    signed int rearRightDoor;
    signed int rearLeftDoor;
    signed int trunkDoor;
    signed int hoodDoor;
    signed int eps;
    signed int srsAirbag;
    signed int abs;
    signed int lowBattery;
    signed int oilPress;
    signed int engine;
    signed int fuel;
    signed int immobi;
    signed int tmFail;
    signed int espAct;
    signed int espOff;
    signed int adaptingLighting;
    signed int autoStop;
    signed int autoStopFail;
    signed int parkingLights;
    signed int frontFog;
    signed int exteriorLightFault;
    signed int accFail;
    signed int ldwOff;
    signed int hillDescent;
    signed int autoHiBeamGreen;
    signed int autoHiBeamAmber;
    signed int sportsMode;
    signed int ldwOperate;
    signed int generalWarn;
    signed int drivingPowerMode;
    signed int hotTemp;
    signed int lowTemp;

    // ShiftPosition
    signed int gearAtVal;
    signed int gearMtVal;

    // Speed
    unsigned long spAnalogVal;
    signed int spAnaDigUnitVal;

    // Tacho
    unsigned long taAnalogVal;

    // TripComputer
    unsigned long trcomTripAVal;
    unsigned long trcomTripBVal;
    unsigned long trcomOdoVal;
    signed int trcomUnitVal;
    unsigned short avgSpeedAVal;
    unsigned short avgSpeedBVal;
    unsigned short hourAVal;
    unsigned short hourBVal;
    unsigned char minuteAVal;
    unsigned char minuteBVal;
    unsigned char secondAVal;
    unsigned char secondBVal;
    signed short oTempVal;
    signed int oTempUnitVal;
    unsigned short cruRangeVal;
    unsigned short avgFuelAVal;
    unsigned short avgFuelBVal;
    unsigned short insFuelAVal;
    unsigned short insFuelBVal;
    signed int fuelEconomyUnitVal;
} IPC_DATA_IC_SERVICE_S;

// for IPC_USAGE_TYPE_FOR_TEST
typedef enum {
    IPC_KIND_TEST_TEST = 0
} IPC_KIND_FOR_TEST_E;

typedef struct {
    signed int test;
} IPC_DATA_FOR_TEST_S;

#endif // IPC_PROTOCOL_H
