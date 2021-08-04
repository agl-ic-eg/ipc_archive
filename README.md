# IPC Diversion Section

## Overview

* It is a general-purpose implementation of Inter-processing communication between the Server and Client (IPC section).
* It consists mainly of the following:
  * IPC library implementation source: src, include
  * IPC unit test program: ipc_unit_test

# Building Method

* Building by following steps:
  ```bash
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  ```
  * The above commands can be executed with the following script:
    ```bash
    $ ./buildtest.sh
    ```

The installation described next will be installed on the host PC (/usr/local/). To change the installation destination, change the option passed to cmake.

```
Example 
$ cmake -DCMAKE_INSTALL_PREFIX=./install ..
```

# Installing Method

  ```bash
  $ cd build
  $ sudo make install
  ```

When succeeded, the following log will be output. 

```
[100%] Built target ipc
Install the project...
-- Install configuration: ""
-- Installing: /usr/local/lib/pkgconfig/ipc.pc
-- Installing: /usr/local/lib/libipc.so.1.0.0
-- Installing: /usr/local/lib/libipc.so.1
-- Installing: /usr/local/lib/libipc.so
-- Installing: /usr/local/include/ipc.h
-- Installing: /usr/local/include/ipc_protocol.h
```

# Building Product 

* At last, Building will generate the following:
  * \<installdir\>/include/
(External Public header files)
    ```bash
    ipc.h
    ipc_protocol.h
    ```
  * \<installdir\>/lib/
(Shared library files) 
    ```bash
    libipc.so   ( Symbolic link ) 
    libipc.so.1 ( Symbolic link )
    libipc.so.1.0.0
    ```
  * build/ipc_unit_test/
(Test program executable file)
    ```bash
    ipc_unit_test_client
    ipc_unit_test_server
    ```
<br>

# How to use

* This library contains functions for Server and Client; Each has different usage.

## Common of Server/Client 

* The user needs to link with the following libraries.
  * `libipc.so`
* User includes the library as following.
  * #include <ipc.h>
    * ipc_protocol.h description is not required; it described later within ipc.h (include <ipc.h>).
* The header files are used as follows.
  * ipc.h
    * Declare a list of available API functions.
  * ipc_protocol.h
    * Define IPC usage types and data structures for each usage.
* In libraries, Server and Client communicate using Unix Domain Socket.
  * Generate different communication files for each usage type. 
  * By default, a communication file is generated in the execution hierarchy of the Server.
  * For changing the location where generating the communication files, set the environment variable "IPC_DOMAIN_PATH" 
  ```
  Example)
  $ export IPC_DOMAIN_PATH="/tmp"
    →Unix Domain Socket communication files will be generated under /tmp.
  ```

## For IC-Service

* Using this library for IC-Service, use the following values and structures (see ipc_protocol.h). 
  * UsageType：IPC_USAGE_TYPE_IC_SERVICE
  * Sending data structure：IPC_DATA_IC_SERVICE_S
  * Changing type callback notification (enum)：IPC_KIND_IC_SERVICE_E
  * Unix Domain Socket communication File name：IpcIcService
* For IC-Service, the Cluster API library (libcluster_api.so) is the IPC Client. 
  * The API for Client is called from libcluster_api.so (described later)

##  Server API

* Server applied libipc.so can use the following APIs:
  * ipcServerStart(IPC_USAGE_TYPE_E usageType);
    * Starting the IPC Server for the specified _usageType_.
  * ipcSendMessage(IPC_USAGE_TYPE_E usageType, const void* pData, signed int size);
    * Sending data to the IPC Client for the specified _usageType_. 
    * Specifying address and size of the sending data by pData and size arguments. 
    * Sending data is stored in the Data Pool prepared on the IPC Client side.
  * ipcServerStop(IPC_USAGE_TYPE_E usageType);
    * Terminate the IPC Server for the specified usageType.

## Client API

* The Client applied libipc.so can use the following APIs:
  * ipcClientStart(IPC_USAGE_TYPE_E usageType);
    * Starting the IPC Client for the specified usageType.
    * Connecting with IPC Server for the same usageType.
  * ipcReadDataPool(IPC_USAGE_TYPE_E usageType, void* pData, signed int* pSize);
    * Reading all data in the Data Pool for the specified usageType.
    * The address where storing the read data is specified in pData. Moreover, the size of storing data is specified in pSize.
    * The contents of the Data Pool output to pData, and the actual read size output to pSize.
  * ipcRegisterCallback(IPC_USAGE_TYPE_E usageType, IPC_CHANGE_NOTIFY_CB changeNotifyCb);
    * When receiving data from the IPC Server, register the callback function for the specified usageType, which receiving notification of which data changed to what.
  * ipcClientStop(IPC_USAGE_TYPE_E usageType);
    * Terminate the IPC Client for the specified usageType.

# Unit test executing method

* Limitations
  * Currently 2020/12/25, this test program connects Server and Client for IC-Service.  
  * The Unix domain socket communication files are generated under /tmp/ with the file name ipcIcService.

* Since Inter processing communication takes place between ipc_unit_test_server and ipc_unit_test_client, so start each one in a separate terminal.
  Testing example as bellow (Manually operated): 

  1. **Starting Server and then starting Client**
      ```bash
      (Terminal 1)
      $ ./ipc_unit_test_server
      command (h=help, q=quit):
      ```
      ```bash
      (Terminal 2)
      $ ./ipc_unit_test_client
      command (h=help, q=quit):
      ```  
      At this point, the Server and Client connection for IC-Service is completed.
      (Executing ipcServerStart () and ipcClientStart ())

  2. **Editing and Sending Server data**
      ```bash
      (Terminal 1)
      command (h=help, q=quit):w ←★input w 
      write command (h=help q=goto main menu):2 1 ←★input 2 1
      write command (h=help q=goto main menu):70 50 ←★input 70 50
      write command (h=help q=goto main menu):l ←★input 1
      ★Sending data list is displayed, the result of input contents as below. 
        2: brake(4) = 1 ←★write command 2 1 result
        70: oTempUnitVal(4) = 50 ←★write command 70 50 result
      write command (h=help q=goto main menu):q ←★input q
      command (h=help, q=quit):s ←★input s (Executing ipcSendMessage())
      ipcSendMessage return:0
      command (h=help, q=quit):
      ```
      On the Client side, The callback function should be responding.
      ```bash
      (Terminal 2)
      command (h=help, q=quit):Enter changeNotifyCb ←★callback
      kind = 2, size = 4, data=1 ←★Notification of brake value changed to 1 
      Leave changeNotifyCb
      ```
      ★oFor TempUnitVal change as an IC-Service, there is no callback since it is not monitored.

  3. **Check Client side receiving.**  
      ```bash
      (Terminal 2)
      command (h=help, q=quit):r ←★input r
      ★Sending data list is displayed, Sending data contained as bellow. 
        2: brake(4) = 1
       70: oTempUnitVal(4) = 50
      ```

  4. **Exit Client then Server.** 
      ```bash
      (Terminal 2)
      command (h=help, q=quit):q
      bye...
      $
      ```
      ```bash
      (Terminal 1)
      command (h=help, q=quit):q
      bye...
      $
      ```

# Adding/Changing IPC usage type method
 
* First, the implementation only for IC-Service, but configured to add data for other services easily.
* Information for each usage type is managed in the following files:
  * include/ipc_protocol.h (External Public header)
  * src/ipc_usage_info_table.c (IPC Internal Source)
* Adding information for new service or changing information for existed service only by two files above.
  * No changes are required other than .c or .h files in ipc.
  * However, Regarding the application and test program used IPC, It is necessary to take measures according to the adding/changing of the ipc_protocol.h definition.
* Ideally, code can be generated automatically using tools and else. We do not consider that implementation this time.

## Sample code for adding/changing the usage type (Sample code difference)

First, the sample code for adding/changing two files as follow.

### Example 1: When adding a new usage type

The following example shows differences when adding the temporary usage type NEW_SERVICE.
(Parameters influenced when adding new usage type within the IPC)

```patch
diff --git a/include/ipc_protocol.h b/include/ipc_protocol.h
index c0ad861..2bc1115 100644
--- a/include/ipc_protocol.h
+++ b/include/ipc_protocol.h
@@ -6,6 +6,7 @@
 typedef enum {
     IPC_USAGE_TYPE_IC_SERVICE = 0,
     IPC_USAGE_TYPE_FOR_TEST,
+    IPC_USAGE_TYPE_NEW_SERVICE, // Adding usage type
     IPC_USAGE_TYPE_MAX
 } IPC_USAGE_TYPE_E;

@@ -145,4 +146,17 @@ typedef struct {
     signed int test;
 } IPC_DATA_FOR_TEST_S;

+// for IPC_USAGE_TYPE_NEW_SERVICE
+typedef enum { // Preparing only the type which we want to monitor/notify data change
+    IPC_KIND_NS_PARAM1 = 0,
+    IPC_KIND_NS_PARAM2
+} IPC_KIND_NEW_SERVICE_E;
+
+typedef struct { // This part for sending and receiving all data 
+    int param1;
+    int param2;
+    int param3;
+    int param4;
+} IPC_DATA_NEW_SERVICE_S;
+
 #endif // IPC_PROTOCOL_H
diff --git a/src/ipc_usage_info_table.c b/src/ipc_usage_info_table.c
index 976cc73..51264c6 100644
--- a/src/ipc_usage_info_table.c
+++ b/src/ipc_usage_info_table.c
@@ -51,16 +51,24 @@ static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeForTest[] = {
     DEFINE_OFFSET_SIZE(IPC_DATA_FOR_TEST_S, test, IPC_KIND_TEST_TEST)
 };

+//   for IPC_USAGE_TYPE_FOR_TEST
+static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeNewService[] = { // Describing only the type which we want to monitor/notify data change
+    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param1, IPC_KIND_NS_PARAM1),
+    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param2, IPC_KIND_NS_PARAM2)
+}; //This example not monitoring/notifying changes of param3, param4 data
+
 // == usage info table ==
 //   index of [] is IPC_USAGE_TYPE_E
 IPC_DOMAIN_INFO_S g_ipcDomainInfoList[] =
 {
     {sizeof(IPC_DATA_IC_SERVICE_S), "ipcIcService"},
-    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"}
+    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"},
+    {sizeof(IPC_DATA_NEW_SERVICE_S), "ipcNewService"} // add sending/receiving size information for new Service 
 };

 IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[] = {
     DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeIcService),
-    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest)
+    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest),
+    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeNewService) // Registering data change monitoring table for new Service
 };

```

### Example 2: Deleting part of the existing usage type data

The following example shows differences when deleting the member variable brake from usage sending/receiving data of existing IC-Service.
(Parameters influenced when a member variable is deleted within the IPC) 

```patch
diff --git a/include/ipc_protocol.h b/include/ipc_protocol.h
index c0ad861..7fed8bf 100644
--- a/include/ipc_protocol.h
+++ b/include/ipc_protocol.h
@@ -13,7 +13,6 @@ typedef enum {
 typedef enum {
     IPC_KIND_ICS_TURN_R = 0,
     IPC_KIND_ICS_TURN_L,
-    IPC_KIND_ICS_BRAKE,
     IPC_KIND_ICS_SEATBELT,
     IPC_KIND_ICS_HIGHBEAM,
     IPC_KIND_ICS_DOOR,
@@ -51,7 +50,6 @@ typedef struct {
     // Telltale
     signed int turnR;
     signed int turnL;
-    signed int brake;
     signed int seatbelt;
     signed int frontRightSeatbelt;
     signed int frontCenterSeatbelt;
diff --git a/src/ipc_usage_info_table.c b/src/ipc_usage_info_table.c
index 976cc73..40ac8df 100644
--- a/src/ipc_usage_info_table.c
+++ b/src/ipc_usage_info_table.c
@@ -12,7 +12,6 @@
 static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeIcService[] = {
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnR, IPC_KIND_ICS_TURN_R),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, turnL, IPC_KIND_ICS_TURN_L),
-    DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, brake, IPC_KIND_ICS_BRAKE),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, seatbelt, IPC_KIND_ICS_SEATBELT),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, highbeam, IPC_KIND_ICS_HIGHBEAM),
     DEFINE_OFFSET_SIZE(IPC_DATA_IC_SERVICE_S, door, IPC_KIND_ICS_DOOR),
```

## Common items regarding new addition of usage type

* Adding some new enumerations/structures, with addition to existing enumeration/structure. There are no restrictions on the names.

## Adding Information to include/ipc_protocol.h
* For one usage type, adding the following three items of information.
  * Add usage type name.
  * For new usage, Define enumeration for the change notification type.
  * For new usage, Define sending/receiving data structure.
* Adding usage type name
  * Sample code of this part will be as follow:
    ```patch
     typedef enum {
         IPC_USAGE_TYPE_IC_SERVICE = 0,
         IPC_USAGE_TYPE_FOR_TEST,
    +    IPC_USAGE_TYPE_NEW_SERVICE, // Adding usage type
         IPC_USAGE_TYPE_MAX
     } IPC_USAGE_TYPE_E;
    ```
  * Adding a member for the usage type in enum IPC_USAGE_TYPE_E.
  * Make sure to add it just before IPC_USAGE_TYPE_MAX (Avoiding effect on existing definitions)
  * The value defined here is used to specify the argument usageType such as ipcServerStart() defined in ipc.h.
* For new usage, Define enumeration for the change notification type.
  * Sample code of this part will be as follow:
    ```patch
    +typedef enum { // Preparing only the type which we want to monitor data change
    +    IPC_KIND_NS_PARAM1 = 0,
    +    IPC_KIND_NS_PARAM2
    +} IPC_KIND_NEW_SERVICE_E;
    ```
  * Adding an enumeration for the data change notification type. Related to the sending/receiving data structures will describe next.
  * This value is used to specify the third argument kind of callback function registered by ipcRegisterCallback().
  * There are no restrictions for naming enumeration and member.
* For new usage, Define sending/receiving data structure. 
  * Sample code of this part will be as follow:
    ```patch
    +typedef struct { // This part for sending and receiving all data
    +    int param1;
    +    int param2;
    +    int param3;
    +    int param4;
    +} IPC_DATA_NEW_SERVICE_S;
    ```
  * For new usage, adding send/receive data structures.
  * The IPC Server will send all the data in the defined structure to the IPC Client.

## Regarding adding src/ipc_usage_info_table.c
* For one usage type, adding the following three items of information.
  * Add a type mapping table for data change notification.
  * Add communication domain information (Communication size and file name).
  * Add mapping table for a relationship between usage and change type.
* Add a type mapping table for data change notification.
  * Sample code of this part will be as follow:
    ```
    +//   for IPC_USAGE_TYPE_FOR_TEST
    +static IPC_CHECK_CHANGE_INFO_S g_ipcCheckChangeNewService[] = { // Preparing only the type which we want to monitor data change
    +    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param1, IPC_KIND_NS_PARAM1),
    +    DEFINE_OFFSET_SIZE(IPC_DATA_NEW_SERVICE_S, param2, IPC_KIND_NS_PARAM2)
    +}; // This example not monitoring/notifying changes of param3, param4 data
    ```
  * For new usage, add a structure array of IPC_CHECK_CHANGE_INFO_S.
  * Describing a table that maps the definition of change notification type enumeration (defined in ipc_protocol.h) with the data structure members.
  * This table is used for Callback notification of the last receiving data type change when the IPC Client received data from the IPC Server. 
  * In structure array, describing multiple macros which defining matching as shown below.
    ```c
    DEFINE_OFFSET_SIZE(<Data structure name>, <Structure member name>, Change notification enumeration member name),
    ```
  * In the case of the above sample code, g_ipcCheckChangeNewService[] will be as follows. 
    * If the value of param1 is different from the value of the previous receiving, the callback change type IPC_KIND_NS_PARAM1 is notified to the IPC Client.
    * If the value of param2 is different from the value of the previous receiving, the callback change type IPC_KIND_NS_PARAM2 is notified to the IPC Client.
    * For param3 and param4 are not described, the callback does not notify even if the value of the previous receiving is different.

* Add communication domain information (Communication size and file name).
  * Sample code of this part will be as follow:
    ```patch
     IPC_DOMAIN_INFO_S g_ipcDomainInfoList[] =
     {
         {sizeof(IPC_DATA_IC_SERVICE_S), "ipcIcService"},
    -    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"}
    +    {sizeof(IPC_DATA_FOR_TEST_S), "ipcForTest"},
    +    {sizeof(IPC_DATA_NEW_SERVICE_S), "ipcNewService"} //add sending/receiving size information for new Service 
     };
    ```
  * In structure array g_ipcDomainInfoList[], adding the domain information for new usage.
  * This addition determines the sending/receiving data size used for the newly added usage type and Domain file name used for Unix Domain Socket communication. 
  * It is necessary to match the definition order of the enum IPC_USAGE_TYPE_E of ipc_protocol.h, so ensure adding it at the end.
  * Add communication data size structure and the domain filename information to the end of g_ipcDomainInfoList[], as follows:
    ```c
    {sizeof(<communication data structure name>), "domain file name"},
    ```
* Add mapping table for a relationship between usage and change type.  
  * Sample code of this part will be as follow:
    ```patch
     IPC_CHECK_CHANGE_INFO_TABLE_S g_ipcCheckChangeInfoTbl[] = {i
         DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeIcService),
    -    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest)
    +    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeForTest),
    +    DEFINE_CHANGE_INFO_TABLE(g_ipcCheckChangeNewService) // Registering data change monitoring table for new service
     };
    ```
  * In structure array g_ipcCheckChangeInfoTbl[], adding information about the mapping table for a relationship between new usage and change notification type.
  * It is necessary to match the definition order of the enum IPC_USAGE_TYPE_E of ipc_protocol.h, so ensure adding it at the end.
  * Describing the above-mentioned "change notification type mapping table structure" in the following macro, then add it to the end of g_ipcCheckChangeInfoTbl[].
    ```c
    DEFINE_CHANGE_INFO_TABLE(<change notification type mapping table structure name>), 
    ```

## Changing of sending data for existing usage
* When deleting or renaming a member variable in an existing sending data structure in ipc_protocol.h
  * Trying to build each ipc part and application that uses ipc for the service, then fix the part that causing the Compile error. 

* When adding a member variable in an existing sending data structure in ipc_protocol.h
  * Refer to [Adding/Changing IPC usage type method](#adding-changing-ipc-usage-type-method), add it to include/ipc_protocol.h and src/ipc_usage_info_table.c.

## Supplement
* In src/ipc_usage_info_table.c, the information is described in the DEFINE_OFFSET_SIZE() macro, which using offsetof() and sizeof() to get the offset and size of member variables from the head of the related structure.
  * In order to make it easier to add usage types, the IPC process does not directly specify variable names in data structures.
  * For each usage, by preparing an offset table of the data structure, it becomes possible to know what variables are in which byte of the sending data.
    * This structure makes it possible to check data change without directly specifying the member variable name inside the IPC process.
  * Adding the usage type according to [Adding/Changing IPC usage type method](#adding-changing-ipc-usage-type-method), the IPC inter processing can process new usage.
