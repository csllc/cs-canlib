/* N-API interface for Kvaser CANlib

   N-API callable functions that directly correspond to CANlib functions, 
   and additional wrapper functions

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/


#ifndef _CS_CANLIB_H_
#define _CS_CANLIB_H_


#define CANLIB_DEBUG
// Uncomment to enable debugging messages, which may affect timing or
// performance

//#define CANLIB_NO_NAPI
// Uncomment to disable declarations for functions that use N-API.
// This is required when compiling test programs that rely solely on the Win32
// and PCAN-Basic APIs.


#ifndef CANLIB_NO_NAPI
#include <node_api.h> // provide N-API types
#endif




// ----------------------------------- // -----------------------------------
// Definitions

#define CAN_FLAGS (0)

#define CANINITIALIZELIBRARY_ARGC (0)
#define CANOPENCHANNEL_ARGC (2)
#define CANCLOSE_ARGC (1)
#define CANSETBUSOUTPUTCONTROL_ARGC (2)
#define CANBUSON_ARGC (1)
#define CANBUSOFF_ARGC (1)
#define CANWRITE_ARGC (2)
#define CANWRITESYNC_ARGC (2)
#define CANREAD_ARGC (1)
#define CANREADWAIT_ARGC (2)
#define CANCHANNELINFO_ARGC (0)
#define CANTRANSLATEBAUD_ARGC (1)
#define CANSETBUSPARAMS_ARGC (2)
#define CANGETBUSPARAMS_ARGC (1)
#define CANFLUSHTRANSMITQUEUE_ARGC (1)
#define CANFLUSHRECEIVEQUEUE_ARGC (1)
#define CANENABLEEVENT_ARGC (3)
#define CANDISABLEEVENT_ARGC (1)
#define CANSETLOOPBACK_ARGC (2)



// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables




// ----------------------------------- // -----------------------------------
// Local functions


napi_value Init(napi_env env, napi_value exports);

void canlib_canEventCallback(int canlibHandle);

void canlib_canEventFinalize(napi_env env, void* finalize_data, void* finalize_hint);




// ----------------------------------- // -----------------------------------
// Public functions


napi_value canlib_canInitializeLibrary(napi_env env, napi_callback_info info);

napi_value canlib_canOpenChannel(napi_env env, napi_callback_info info);

napi_value canlib_canClose(napi_env env, napi_callback_info info);

napi_value canlib_canSetBusParams(napi_env env, napi_callback_info info);

napi_value canlib_canGetBusParams(napi_env env, napi_callback_info info);

napi_value canlib_canSetBusOutputControl(napi_env env, napi_callback_info info);

napi_value canlib_canBusOn(napi_env env, napi_callback_info info);

napi_value canlib_canBusOff(napi_env env, napi_callback_info info);

napi_value canlib_canWrite(napi_env env, napi_callback_info info);

napi_value canlib_canWriteSync(napi_env env, napi_callback_info info);

napi_value canlib_canRead(napi_env env, napi_callback_info info);

napi_value canlib_canReadWait(napi_env env, napi_callback_info info);

napi_value canlib_canChannelInfo(napi_env env, napi_callback_info info);

napi_value canlib_canTranslateBaud(napi_env env, napi_callback_info info);

napi_value canlib_canFlushTransmitQueue(napi_env env, napi_callback_info info);

napi_value canlib_canFlushReceiveQueue(napi_env env, napi_callback_info info);

napi_value canlib_canEnableEvent(napi_env env, napi_callback_info info);

napi_value canlib_canDisableEvent(napi_env env, napi_callback_info info);

napi_value canlib_canSetLoopback(napi_env env, napi_callback_info info);

#endif // _CS_CANLIB_H_

