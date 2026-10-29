/* N-API interface for Kvaser CANlib

   N-API callable functions that directly correspond to CANlib functions, 
   and additional wrapper functions

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/
   

#include <assert.h>      // provide assert
#include <node_api.h>    // provide N-API types, constants, and functions
#include <stdio.h>       // provide printf
#include <stdint.h>      // provide uintX_t

#include <windows.h>     // provide Win32 types, constants, and functions

#include <canlib.h>      // provide CANlib API functions
#include <canstat.h>     // provide CANlib status constants

#include "cs_canlib.h"
#include "cs_canlib_event.h"  // provide canlibEvent* functions
#include "cs_canlib_helper.h" // provide canlibStatusLookup, canlibDumpBuffer
#include "napi_helper.h"      // provide DECLARE_NAPI_METHOD


// ----------------------------------- // -----------------------------------
// Definitions


// Length of N-API module descriptor list, used in Init
enum { descriptors_len = 26 };




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables


// Data read callback function, created in main thread and called from worker
// thread
napi_threadsafe_function canlibCallback = { 0 };




// ----------------------------------- // -----------------------------------
// Local functions


// Register N-API module
NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);




napi_value Init(napi_env env, napi_value exports)
{
    napi_status status = napi_generic_failure;

    // Turn off buffering on stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    // Build array of property descriptors
    // If modifying this list, remember to update descriptors_len, above 
    napi_property_descriptor descriptors[descriptors_len] = {
        // CANlib functions
        DECLARE_NAPI_METHOD("InitializeLibrary", canlib_canInitializeLibrary),
        DECLARE_NAPI_METHOD("OpenChannel", canlib_canOpenChannel),
        DECLARE_NAPI_METHOD("Close", canlib_canClose),
        DECLARE_NAPI_METHOD("BusOn", canlib_canBusOn),
        DECLARE_NAPI_METHOD("BusOff", canlib_canBusOff),
        DECLARE_NAPI_METHOD("TranslateBaud", canlib_canTranslateBaud),
        DECLARE_NAPI_METHOD("SetBusParams", canlib_canSetBusParams),
        DECLARE_NAPI_METHOD("GetBusParams", canlib_canGetBusParams),
        DECLARE_NAPI_METHOD("Write", canlib_canWrite),
        DECLARE_NAPI_METHOD("WriteSync", canlib_canWriteSync),
        DECLARE_NAPI_METHOD("Read", canlib_canRead),
        DECLARE_NAPI_METHOD("ReadWait", canlib_canReadWait),
        DECLARE_NAPI_METHOD("SetAcceptanceFilter", canlib_canSetAcceptanceFilter),
        DECLARE_NAPI_METHOD("FlushTransmitQueue", canlib_canFlushTransmitQueue),
        DECLARE_NAPI_METHOD("FlushReceiveQueue", canlib_canFlushReceiveQueue),
        DECLARE_NAPI_METHOD("ReadStatus", canlib_canReadStatus),
        DECLARE_NAPI_METHOD("ResetBus", canlib_canResetBus),
        DECLARE_NAPI_METHOD("GetErrorText", canlib_canGetErrorText),
        DECLARE_NAPI_METHOD("ReadErrorCounters", canlib_canReadErrorCounters),
        DECLARE_NAPI_METHOD("RequestChipStatus", canlib_canRequestChipStatus),
        // Additional functions
        DECLARE_NAPI_METHOD("ChannelInfo", canlib_canChannelInfo),
        DECLARE_NAPI_METHOD("EnableEvent", canlib_canEnableEvent),
        DECLARE_NAPI_METHOD("DisableEvent", canlib_canDisableEvent),
        DECLARE_NAPI_METHOD("SetLoopback", canlib_canSetLoopback),
        DECLARE_NAPI_METHOD("ClearErrorCounters", canlib_canClearErrorCounters),
        DECLARE_NAPI_METHOD("GetFlagText", canlib_canGetFlagText),
    };

    status = napi_define_properties(env, exports, descriptors_len, descriptors);
    assert(status == napi_ok);

    return exports;
}




void canlib_canEventCallback(int canlibHandle)
{
    napi_status status = napi_generic_failure;

#ifdef PCAN_DEBUG
    printf("canlib_canEventCallback()\n");
#endif

    status = napi_call_threadsafe_function(canlibCallback, 0, true);
    assert(status == napi_ok);

    return;
}




void canlib_canEventFinalize(napi_env env, void* finalize_data, void* finalize_hint)
{
#ifdef PCAN_DEBUG
    printf("canlib_canEventFinalize\n");
#endif
}




// ----------------------------------- // -----------------------------------
// Public functions


napi_value canlib_canInitializeLibrary(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANINITIALIZELIBRARY_ARGC;
    napi_value argv;

    status = napi_get_cb_info(env, info, &argc, &argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANINITIALIZELIBRARY_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Initialize CAN library
    canInitializeLibrary();

    // Assume success for lack of return value from the API function
    napi_value result;
    status = napi_create_uint32(env, 0, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canOpenChannel(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANOPENCHANNEL_ARGC;
    napi_value argv[CANOPENCHANNEL_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANOPENCHANNEL_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibChannel;
    status = napi_get_value_int32(env, argv[0], &canlibChannel);
    assert(status == napi_ok);

    int32_t canlibFlags;
    status = napi_get_value_int32(env, argv[1], &canlibFlags);
    assert(status == napi_ok);

    // Open CAN channel
    int32_t canlibHandle;
    canlibHandle = canOpenChannel(canlibChannel, canlibFlags);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canOpenChannel:\n"
           "  canlibHandle  = 0x%02X (%s)\n"
           "  canlibChannel = %i\n"
           "  canlibFlags   = 0x%02X\n",
           canlibHandle, canlibStatusLookup(canlibHandle),
           canlibChannel,
           canlibFlags);
#endif

    // Throw error, if any
    if (canlibHandle < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibHandle),
                         "canlib_canOpenChannel: Error at canOpenChannel.");
        return 0;
    }

    // If all went well, we now have the handle to the open CAN channel.
    // Create an N-API value for it and return it.
    napi_value handle;
    status = napi_create_int32(env, canlibHandle, &handle);
    assert(status == napi_ok);

    return handle;
}




napi_value canlib_canClose(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANCLOSE_ARGC;
    napi_value argv[CANCLOSE_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANCLOSE_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Close CAN channel
    canStatus canlibStatus;
    canlibStatus = canClose(canlibHandle);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canClose:\n"
           "  canlibStatus      = %i (%s)\n"
           "  canlibHandle      = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canClose: Error at canClose.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canBusOn(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANBUSON_ARGC;
    napi_value argv[CANBUSON_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANBUSON_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Attempt to go on-bus
    canStatus canlibStatus;
    canlibStatus = canBusOn(canlibHandle);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canBusOn:\n"
           "  canlibStatus      = %i (%s)\n"
           "  canlibHandle      = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canBusOn: Error at canBusOn.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canBusOff(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANBUSOFF_ARGC;
    napi_value argv[CANBUSOFF_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANBUSOFF_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Attempt to go off-bus
    canStatus canlibStatus;
    canlibStatus = canBusOn(canlibHandle);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canBusOff:\n"
           "  canlibStatus      = %i (%s)\n"
           "  canlibHandle      = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canBusOff: Error at canBusOff.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canTranslateBaud(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANTRANSLATEBAUD_ARGC;
    napi_value argv[CANTRANSLATEBAUD_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANTRANSLATEBAUD_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canTranslateBaud accepts 1 argument.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibFreq;
    status = napi_get_value_int32(env, argv[0], &canlibFreq);
    assert(status == napi_ok);

    // Translate integer frequency into library constant
    int32_t canlibFreqTranslated;
    canlibFreqTranslated = canlibTranslateBaud(canlibFreq);

#ifdef CANLIB_DEBUG
    printf("canlibTranslateBaud: %i -> %i\n",
           canlibFreq, canlibFreqTranslated);
#endif

    // Create additional variables to receive data into
    unsigned int canlibTseg1;
    unsigned int canlibTseg2;
    unsigned int canlibSjw;
    unsigned int canlibNoSamp;

    // Translate frequency constant into bus parameters and (back into)
    // integer frequency
    int32_t canlibStatus;
    canlibStatus = canTranslateBaud(&canlibFreqTranslated,
                                    &canlibTseg1,
                                    &canlibTseg2,
                                    &canlibSjw,
                                    &canlibNoSamp,
                                    0); // syncMode, unsupported

    // Print the result to console
#ifdef CANLIB_DEBUG
    printf("canlibTranslateBaud:\n"
           "  canlibStatus         = %i (%s)\n"
           "  canlibFreqTranslated = %i\n"
           "  canlibTseg1          = %i\n"
           "  canlibTseg2          = %i\n"
           "  canlibSjw            = %i\n"
           "  canlibNoSamp         = %i\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibFreqTranslated,
           canlibTseg1,
           canlibTseg2,
           canlibSjw,
           canlibNoSamp);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, 0, "canlib_canTranslateBaud: Error at canTranslateBaud.");
        return 0;
    }

    // Create N-API object to return results in
    napi_value busParams;
    status = napi_create_object(env, &busParams);
    assert(status == napi_ok);

    napi_value freq;
    status = napi_create_int32(env, canlibFreqTranslated, &freq);
    assert(status == napi_ok);
    status = napi_set_named_property(env, busParams, "freq", freq);
    assert(status == napi_ok);

    napi_value tseg1;
    status = napi_create_int32(env, canlibTseg1, &tseg1);
    assert(status == napi_ok);
    status = napi_set_named_property(env, busParams, "tseg1", tseg1);
    assert(status == napi_ok);

    napi_value tseg2;
    status = napi_create_int32(env, canlibTseg2, &tseg2);
    assert(status == napi_ok);
    status = napi_set_named_property(env, busParams, "tseg2", tseg2);
    assert(status == napi_ok);

    napi_value sjw;
    status = napi_create_int32(env, canlibSjw, &sjw);
    assert(status == napi_ok);
    status = napi_set_named_property(env, busParams, "sjw", sjw);
    assert(status == napi_ok);

    napi_value noSamp;
    status = napi_create_int32(env, canlibNoSamp, &noSamp);
    assert(status == napi_ok);
    status = napi_set_named_property(env, busParams, "noSamp", noSamp);
    assert(status == napi_ok);

    // Return the object
    return busParams;
}




napi_value canlib_canSetBusParams(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANSETBUSPARAMS_ARGC;
    napi_value argv[CANSETBUSPARAMS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANSETBUSPARAMS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canSetBusParams accepts 2 arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    napi_valuetype canlibBusParamsType;
    status = napi_typeof(env, argv[1], &canlibBusParamsType);
    assert(status == napi_ok);

    if (canlibBusParamsType != napi_object)
    {
        napi_throw_type_error(env, 0, "Argument 1 (BusParams) is not an object.");
        return 0;
    }

    napi_value freq;
    int32_t canlibFreq;
    status = napi_get_named_property(env, argv[1], "freq", &freq);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, freq, &canlibFreq);

    napi_value tseg1;
    int32_t canlibTseg1;
    status = napi_get_named_property(env, argv[1], "tseg1", &tseg1);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, tseg1, &canlibTseg1);
    assert(status == napi_ok);

    napi_value tseg2;
    int32_t canlibTseg2;
    status = napi_get_named_property(env, argv[1], "tseg2", &tseg2);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, tseg2, &canlibTseg2);
    assert(status == napi_ok);

    napi_value sjw;
    int32_t canlibSjw;
    status = napi_get_named_property(env, argv[1], "sjw", &sjw);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, sjw, &canlibSjw);
    assert(status == napi_ok);

    napi_value noSamp;
    int32_t canlibNoSamp;
    status = napi_get_named_property(env, argv[1], "noSamp", &noSamp);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, noSamp, &canlibNoSamp);
    assert(status == napi_ok);

    // Set the bus parameters
    canStatus canlibStatus;
    canlibStatus = canSetBusParams(canlibHandle,
                                   canlibFreq,
                                   canlibTseg1,
                                   canlibTseg2,
                                   canlibSjw,
                                   canlibNoSamp,
                                   0); // syncmode, unsupported

    // Print the result to console
#ifdef CANLIB_DEBUG
    printf("canlib_canSetBusParams:\n"
           "  canlibStatus  = %i (%s)\n"
           "  canlibHandle  = 0x%02X\n"
           "  canlibFreq    = %i\n"
           "  canlibTseg1   = %i\n"
           "  canlibTseg2   = %i\n"
           "  canlibSjw     = %i\n"
           "  canlibNoSamp  = %i\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibFreq,
           canlibTseg1,
           canlibTseg2,
           canlibSjw,
           canlibNoSamp);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, 0, "canlib_canSetBusParams: Error at canSetBusParams.");
        return 0;
    }

    // Create N-API object to return result in
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canGetBusParams(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANGETBUSPARAMS_ARGC;
    napi_value argv[CANGETBUSPARAMS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANGETBUSPARAMS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canGetBusParams accepts 1 argument.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Create variables to receive parameters
    long canlibFreq;
    unsigned int canlibTseg1;
    unsigned int canlibTseg2;
    unsigned int canlibSjw;
    unsigned int canlibNoSamp;
    unsigned int canlibSyncmode;

    // Retrieve bus parameters
    canStatus canlibStatus;
    canlibStatus = canGetBusParams(canlibHandle,
                                   &canlibFreq,
                                   &canlibTseg1,
                                   &canlibTseg2,
                                   &canlibSjw,
                                   &canlibNoSamp,
                                   &canlibSyncmode);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canGetBusParams:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n"
           "  canlibFreq     = %i\n"
           "  canlibTseg1    = %i\n"
           "  canlibTseg2    = %i\n"
           "  canlibSjw      = %i\n"
           "  canlibNoSamp   = %i\n"
           "  canlibSyncmode = %i\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibFreq,
           canlibTseg1,
           canlibTseg2,
           canlibSjw,
           canlibNoSamp,
           canlibSyncmode);
#endif

    // Create N-API object to contain the results
    napi_value params;
    status = napi_create_object(env, &params);
    assert(status == napi_ok);

    // Create N-API values for each of the results and set them as properties of
    // the container
    napi_value freq;
    status = napi_create_int32(env, canlibFreq, &freq);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "freq", freq);
    assert(status == napi_ok);

    napi_value tseg1;
    status = napi_create_int32(env, canlibTseg1, &tseg1);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "tseg1", tseg1);
    assert(status == napi_ok);

    napi_value tseg2;
    status = napi_create_int32(env, canlibTseg2, &tseg2);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "tseg2", tseg2);
    assert(status == napi_ok);

    napi_value sjw;
    status = napi_create_int32(env, canlibSjw, &sjw);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "sjw", sjw);
    assert(status == napi_ok);

    napi_value noSamp;
    status = napi_create_int32(env, canlibNoSamp, &noSamp);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "noSamp", noSamp);
    assert(status == napi_ok);

    napi_value syncmode;
    status = napi_create_int32(env, canlibSyncmode, &syncmode);
    assert(status == napi_ok);
    status = napi_set_named_property(env, params, "syncmode", syncmode);
    assert(status == napi_ok);

    return params;
}




napi_value canlib_canWrite(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANWRITE_ARGC;
    napi_value argv[CANWRITE_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANWRITE_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    napi_valuetype canlibMsgType;
    status = napi_typeof(env, argv[1], &canlibMsgType);
    assert(status == napi_ok);

    if (canlibMsgType != napi_object)
    {
        napi_throw_type_error(env, 0, "Argument 1 (Msg) is not an object.");
        return 0;
    }

    napi_value id;
    int32_t canlibId;
    status = napi_get_named_property(env, argv[1], "id", &id);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, id, &canlibId);
    assert(status == napi_ok);

    napi_value msg;
    uint8_t *canlibMsg;
    size_t canlibMsgLen;
    status = napi_get_named_property(env, argv[1], "buf", &msg);
    assert(status == napi_ok);
    status = napi_get_buffer_info(env, msg, (void**)&canlibMsg, &canlibMsgLen);
    assert(status == napi_ok);
    
    napi_value dlc;
    uint32_t canlibDlc;
    status = napi_get_named_property(env, argv[1], "dlc", &dlc);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, dlc, &canlibDlc);
    assert(status == napi_ok);

    napi_value flag;
    uint32_t canlibFlag;
    status = napi_get_named_property(env, argv[1], "flag", &flag);
    assert(status == napi_ok);
    status = napi_get_value_int32(env, flag, &canlibFlag);
    assert(status == napi_ok);


    if (canlibDlc != canlibMsgLen)
    {
        napi_throw_error(env, 0, "Mismatch between DLC and actual data buffer length");
        return 0;
    }

    // Attempt to write message
    canStatus canlibStatus;
    canlibStatus = canWrite(canlibHandle, canlibId, canlibMsg, canlibDlc, canlibFlag);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canWrite:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n"
           "  canlibId       = 0x%02X\n"
           "  canlibDlc      = 0x%02X\n"
           "  canlibFlag     = 0x%02X\n"
           "  canlibMsg      = ",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibId,
           canlibDlc,
           canlibFlag);
    canlibDumpBuffer(canlibMsg, canlibDlc);

    printf("\n");
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canWrite: Error at canWrite.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canWriteSync(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANWRITESYNC_ARGC;
    napi_value argv[CANWRITESYNC_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANWRITESYNC_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    uint32_t canlibTimeout;
    status = napi_get_value_uint32(env, argv[0], &canlibTimeout);
    assert(status == napi_ok);

    // Wait for CAN messages to be sent
    canStatus canlibStatus;
    canlibStatus = canWriteSync(canlibHandle, canlibTimeout);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canWriteSync:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n"
           "  canlibTimeout  = %i\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibTimeout);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canWriteSync: Error at canWriteSync.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canRead(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANREAD_ARGC;
    napi_value argv[CANREAD_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANREAD_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canRead accepts 1 arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Create variables to receive CAN data
    unsigned int canlibId;
    uint8_t canlibMsg[8];
    unsigned int canlibDlc;
    unsigned int canlibFlag;
    unsigned long canlibTime;

    // Read from CAN receive buffer
    canStatus canlibStatus;
    canlibStatus = canRead(canlibHandle,
                           &canlibId,
                           canlibMsg,
                           &canlibDlc,
                           &canlibFlag,
                           &canlibTime);

    // Print results to console
#ifdef CANLIB_DEBUG
    if (canlibStatus == canOK)
    {
        printf("canlib_canRead:\n"
               "  canlibStatus   = %i (%s)\n"
               "  canlibHandle   = 0x%02X\n"
               "  canlibId       = 0x%02X\n"
               "  canlibDlc      = 0x%02X\n"
               "  canlibFlag     = 0x%02X\n"
               "  canlibTime     = 0x%02X\n"
               "  canlibMsg      = ",
               canlibStatus, canlibStatusLookup(canlibStatus),
               canlibHandle,
               canlibId,
               canlibDlc,
               canlibFlag,
               canlibTime);

        canlibDumpBuffer(canlibMsg, canlibDlc);

        printf("\n");
    }
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canRead: Error at canRead.");
        return 0;
    }

    // Create N-API object to return
    napi_value message;
    status = napi_create_object(env, &message);
    assert(status == napi_ok);

    // Set properties of object
    napi_value id;
    status = napi_create_int32(env, canlibId, &id);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "id", id);
    assert(status == napi_ok);

    napi_value buf;
    void *bufResult;
    status = napi_create_buffer_copy(env, canlibDlc, canlibMsg, (void*)&bufResult, &buf);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "buf", buf);
    assert(status == napi_ok);

    napi_value dlc;
    status = napi_create_int32(env, canlibDlc, &dlc);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "dlc", dlc);
    assert(status == napi_ok);

    napi_value flag;
    status = napi_create_int32(env, canlibFlag, &flag);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "flag", flag);
    assert(status == napi_ok);

    napi_value time;
    status = napi_create_int32(env, canlibTime, &time);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "time", time);
    assert(status == napi_ok);

    // Return the object
    return message;
}




napi_value canlib_canReadWait(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANREADWAIT_ARGC;
    napi_value argv[CANREADWAIT_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANREADWAIT_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canReadWait accepts 2 arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    int32_t canlibTimeout;
    status = napi_get_value_int32(env, argv[1], &canlibTimeout);
    assert(status == napi_ok);

    // Create variables to receive CAN data
    unsigned int canlibId;
    uint8_t canlibMsg[8];
    unsigned int canlibDlc;
    unsigned int canlibFlag;
    unsigned long canlibTime;

    // Read from CAN receive buffer
    canStatus canlibStatus;
    canlibStatus = canReadWait(canlibHandle,
                               &canlibId,
                               canlibMsg,
                               &canlibDlc,
                               &canlibFlag,
                               &canlibTime,
                               canlibTimeout);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canRead:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n"
           "  canlibId       = 0x%02X\n"
           "  canlibDlc      = 0x%02X\n"
           "  canlibFlag     = 0x%02X\n"
           "  canlibTime     = 0x%02X\n"
           "  canlibTimeout  = %i\n"
           "  canlibMsg      = ",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibId,
           canlibDlc,
           canlibFlag,
           canlibTime,
           canlibTimeout);

    canlibDumpBuffer(canlibMsg, canlibDlc);

    printf("\n");
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canReadWait: Error at canReadWait.");
        return 0;
    }

    // Create N-API object to return
    napi_value message;
    status = napi_create_object(env, &message);
    assert(status == napi_ok);

    // Set properties of object
    napi_value id;
    status = napi_create_int32(env, canlibId, &id);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "id", id);
    assert(status == napi_ok);

    napi_value buf;
    void *bufResult;
    status = napi_create_buffer_copy(env, canlibDlc, canlibMsg, (void*)&bufResult, &buf);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "buf", buf);
    assert(status == napi_ok);

    napi_value dlc;
    status = napi_create_int32(env, canlibDlc, &dlc);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "dlc", dlc);
    assert(status == napi_ok);

    napi_value flag;
    status = napi_create_int32(env, canlibFlag, &flag);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "flag", flag);
    assert(status == napi_ok);

    napi_value time;
    status = napi_create_int32(env, canlibTime, &time);
    assert(status == napi_ok);
    status = napi_set_named_property(env, message, "time", time);
    assert(status == napi_ok);

    // Return the object
    return message;
}




napi_value canlib_canSetAcceptanceFilter(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANSETACCEPTANCEFILTER_ARGC;
    napi_value argv[CANSETACCEPTANCEFILTER_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANSETACCEPTANCEFILTER_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve or check arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    unsigned int canlibCode;
    status = napi_get_value_uint32(env, argv[1], &canlibCode);
    assert(status == napi_ok);

    unsigned int canlibMask;
    status = napi_get_value_uint32(env, argv[2], &canlibMask);
    assert(status == napi_ok);

    bool canlibIsExtended;
    status = napi_get_value_bool(env, argv[3], &canlibIsExtended);
    assert(status == napi_ok);

    // Apply filter
    canStatus canlibStatus;
    canlibStatus = canSetAcceptanceFilter(canlibHandle, canlibCode,
                                          canlibMask, canlibIsExtended);

    // Print result to console
#ifdef CANLIB_DEBUG
    printf("canSetAcceptanceFilter:\n"
           "  canlibStatus     = %i\n"
           "  canlibHandle     = 0x%02X\n"
           "  canlibCode       = 0x%08X\n"
           "  canlibMask       = 0x%08X\n"
           "  canlibIsExtended = %i\n",
           canlibStatus,
           canlibHandle,
           canlibCode,
           canlibMask,
           canlibIsExtended);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canSetAcceptanceFilter: "
                         "Error at canSetAcceptanceFilter.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canFlushTransmitQueue(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANFLUSHTRANSMITQUEUE_ARGC;
    napi_value argv[CANFLUSHTRANSMITQUEUE_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANFLUSHTRANSMITQUEUE_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Flush transmit queue
    canStatus canlibStatus;
    canlibStatus = canFlushTransmitQueue(canlibHandle);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canFlushTransmitQueue:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canFlushTransmitQueue: "
                         "Error at canFlushTransmitQueue.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canFlushReceiveQueue(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANFLUSHRECEIVEQUEUE_ARGC;
    napi_value argv[CANFLUSHRECEIVEQUEUE_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANFLUSHRECEIVEQUEUE_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Flush receive queue
    canStatus canlibStatus;
    canlibStatus = canFlushReceiveQueue(canlibHandle);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canReceiveQueue:\n"
           "  canlibStatus   = %i (%s)\n"
           "  canlibHandle   = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canFlushReceiveQueue: "
                         "Error at canFlushReceiveQueue.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canReadStatus(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANREADSTATUS_ARGC;
    napi_value argv[CANREADSTATUS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANREADSTATUS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Read status
    canStatus canlibStatus;
    unsigned long statusFlags = 0;
    canlibStatus = canReadStatus(canlibHandle, &statusFlags);

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canReadStatus: Error at canReadStatus.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, statusFlags, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canResetBus(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANRESETBUS_ARGC;
    napi_value argv[CANRESETBUS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANRESETBUS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Reset bus
    canStatus canlibStatus;
    canlibStatus = canResetBus(canlibHandle);

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canResetBus: Error at canResetBus.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}

    


napi_value canlib_canGetErrorText(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANGETERRORTEXT_ARGC;
    napi_value argv[CANGETERRORTEXT_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANGETERRORTEXT_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canGetErrorText accepts 1 argument.");
        return 0;
    }

    // Retreive argument
    uint32_t canErrorInt;
    status = napi_get_value_uint32(env, argv[0], &canErrorInt);
    assert(status == napi_ok);

    // Create buffer for error text
    char canErrorText[256] = { 0 };

    // Get error text
    canStatus canlibStatus;
    canlibStatus = canGetErrorText(canErrorInt, canErrorText, sizeof(canErrorText));

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canGetErrorText: Error at canGetErrorText.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_string_utf8(env, canErrorText, NAPI_AUTO_LENGTH, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canReadErrorCounters(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANREADERRORCOUNTERS_ARGC;
    napi_value argv[CANREADERRORCOUNTERS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANREADERRORCOUNTERS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canReadErrorCounters accepts 1 argument.");
        return 0;
    }

    // Retreive argument
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Create variables to receive error counts into
    int32_t canlibTxErr;
    int32_t canlibRxErr;
    int32_t canlibOvErr;

    // Read error counters
    canStatus canlibStatus;
    canlibStatus = canReadErrorCounters(canlibHandle, &canlibTxErr,
                                        &canlibRxErr, &canlibOvErr);

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canReadErrorCounters: Error at canReadErrorCounters.");
        return 0;
    }

    // Create container object for error counter values
    napi_value errorCounters;
    status = napi_create_object(env, &errorCounters);
    assert(status == napi_ok);

    // Create properties within container object
    napi_value txErr;
    napi_create_int32(env, canlibTxErr, &txErr);
    assert(status == napi_ok);
    status = napi_set_named_property(env, errorCounters, "txErr", txErr);
    assert(status == napi_ok);

    napi_value rxErr;
    napi_create_int32(env, canlibRxErr, &rxErr);
    assert(status == napi_ok);
    status = napi_set_named_property(env, errorCounters, "rxErr", rxErr);
    assert(status == napi_ok);

    napi_value ovErr;
    napi_create_int32(env, canlibOvErr, &ovErr);
    assert(status == napi_ok);
    status = napi_set_named_property(env, errorCounters, "ovErr", ovErr);
    assert(status == napi_ok);

    // Return the errorCounters object
    return errorCounters;
}




napi_value canlib_canRequestChipStatus(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANREQUESTCHIPSTATUS_ARGC;
    napi_value argv[CANREQUESTCHIPSTATUS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANREADERRORCOUNTERS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments. "
                              "This implementation of canReadErrorCounters accepts 1 argument.");
        return 0;
    }

    // Retreive argument
    int32_t canlibHandle;
    status = napi_get_value_int32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Request chip status
    canStatus canlibStatus;
    canlibStatus = canRequestChipStatus(canlibHandle);

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canReadErrorCounters: Error at canReadErrorCounters.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canChannelInfo(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANCHANNELINFO_ARGC;
    napi_value argv;

    status = napi_get_cb_info(env, info, &argc, &argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANCHANNELINFO_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Get number of channels
    int canlibChannelCount;
    canStatus canlibStatus;
    canlibStatus = canGetNumberOfChannels(&canlibChannelCount);

    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canChannelInfo: Error at canGetNumberOfChannels.");
        return 0;
    }

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canChannelInfo: %i channels found\n", canlibChannelCount);
#endif

    // Allocate array for channel types
    int32_t *canlibChannelTypes = 0;

    canlibChannelTypes = malloc(sizeof(*canlibChannelTypes) * canlibChannelCount);
    if (canlibChannelTypes == 0)
    {
        napi_throw_error(env, 0, "canlib_canChannelInfo: Error at malloc.");
        return 0;
    }

    // Retrieve channel type information
    int i;
    
    for (i = 0; i < canlibChannelCount; i++)
    {
        canlibStatus = canGetChannelData(i, // channel
                                         canCHANNELDATA_CARD_TYPE,
                                         &(canlibChannelTypes[i]),
                                         sizeof(canlibChannelTypes[i]));
        if (canlibStatus < 0)
        {
            napi_throw_error(env, canlibStatusLookup(canlibStatus),
                             "canlib_canChannelInfo: Error at canGetChannelData.");
            return 0;
        }
    }

    // Print result to console
#ifdef CANLIB_DEBUG
    for (i = 0; i < canlibChannelCount; i++)
    {
        printf("canlib_canChannelInfo: canlibChannelTypes[%i] = %i\n",
               i, canlibChannelTypes[i]);
    }
#endif

    // Create parent N-API object for channel info array
    napi_value channelInfo;
    status = napi_create_array(env, &channelInfo);
    assert(status == napi_ok);

    napi_value channelInfoElement;
    napi_value channelInfoElementChannel;
    napi_value channelInfoElementChannelType;

    // Add applicable channels (based on type) to N-API object
    uint32_t outputIndex = 0;
    for (i = 0; i < canlibChannelCount; i++)
    {
        // Only add Leaf / Leaf3 hardware series
        if ((canlibChannelTypes[i] == canHWTYPE_LEAF) ||
            (canlibChannelTypes[i] == canHWTYPE_LEAF3))
        {
            // Create N-API object for array element
            status = napi_create_object(env, &channelInfoElement);
            assert(status == napi_ok);

            // Assign properties
            // - Channel number
            status = napi_create_int32(env, i, &channelInfoElementChannel);
            assert(status == napi_ok);
            status = napi_set_named_property(env, channelInfoElement, "channel",
                                             channelInfoElementChannel);
            assert(status == napi_ok);

            // - Channel type
            status = napi_create_int32(env, canlibChannelTypes[i],
                                       &channelInfoElementChannelType);
            assert(status == napi_ok);
            status = napi_set_named_property(env, channelInfoElement, "channel_type",
                                             channelInfoElementChannelType);
            assert(status == napi_ok);

            // Add complete element to array
            status = napi_set_element(env, channelInfo, outputIndex++, channelInfoElement);
            assert(status == napi_ok);
        }
    }

    // Cleanup
    free(canlibChannelTypes);

    // Return N-API object
    return channelInfo;
}




napi_value canlib_canEnableEvent(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANENABLEEVENT_ARGC;
    napi_value argv[CANENABLEEVENT_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANENABLEEVENT_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve or check arguments
    int32_t canlibChannel;
    status = napi_get_value_int32(env, argv[0], &canlibChannel);
    assert(status == napi_ok);

    int32_t canlibFlags;
    status = napi_get_value_int32(env, argv[1], &canlibFlags);
    assert(status == napi_ok);

    napi_valuetype canlibCallbackType;
    status = napi_typeof(env, argv[2], &canlibCallbackType);
    assert(status == napi_ok);

    if (canlibCallbackType != napi_function)
    {
        napi_throw_type_error(env, 0, "Argument 2 (Callback) is not a function.");
        return 0;
    }

    // Create resource name string, used below
    napi_value asyncResourceName;
    status = napi_create_string_utf8(env, "canlibCallback",
                                     NAPI_AUTO_LENGTH, &asyncResourceName);
    assert(status == napi_ok);

    // Create thread-safe function to use as a callback
    status = napi_create_threadsafe_function(env,
                                             argv[2], // func
                                             0, // async_resource
                                             asyncResourceName,
                                             0, // max_queue_size
                                             1, // initial_thread_count
                                             0, // thread_finalize_data
                                             canlib_canEventFinalize,
                                             0, // context
                                             0, // call_js_cb
                                             &canlibCallback); // result
    assert(status == napi_ok);

    // Enable CAN Bus receive event, receive canlib handle used by thread
    canStatus canlibStatus;
    canlibStatus = canlibEventEnable(canlibChannel,
                                     canlibFlags,
                                     canlib_canEventCallback);

    // Print result to console
#ifdef CANLIB_DEBUG
    printf("canlib_canEnableEvent: 0x%02X\n", canlibStatus);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, 0,
                         "canlib_canEnableEvent: Error at canlibEventEnable");
        return 0;
    }

    // Create a N-API value for the handle (or error) and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}

    


napi_value canlib_canDisableEvent(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Get callback info and argument list
    size_t argc = CANDISABLEEVENT_ARGC;
    napi_value argv[CANDISABLEEVENT_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANDISABLEEVENT_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    uint32_t canlibChannel;
    status = napi_get_value_uint32(env, argv[0], &canlibChannel);
    assert(status == napi_ok);

    // Disable CAN Bus receive event
    canStatus canlibStatus;
    canlibStatus = canlibEventDisable();

    // Print result to console
#ifdef CANLIB_DEBUG
    printf("canlib_canDisableEvent: 0x%02X\n", canlibStatus);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canDisableEvent");
        return 0;
    }

    // Try to release the threadsafe function
    status = napi_unref_threadsafe_function(env, canlibCallback);
    assert(status == napi_ok);
    status = napi_release_threadsafe_function(canlibCallback, napi_tsfn_abort);
    assert(status == napi_ok);

    // Create a N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}


napi_value canlib_canSetLoopback(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Get callback info and argument list
    size_t argc = CANSETLOOPBACK_ARGC;
    napi_value argv[CANSETLOOPBACK_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANSETLOOPBACK_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    uint32_t canlibHandle;
    status = napi_get_value_uint32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    uint8_t canlibLoopbackSetting;
    status = napi_get_value_bool(env, argv[1], &canlibLoopbackSetting);
    assert(status == napi_ok);

    // Set message loopback
    canStatus canlibStatus;
    canlibStatus = canIoCtl(canlibHandle, canIOCTL_SET_LOCAL_TXECHO,
                            &canlibLoopbackSetting, sizeof(canlibLoopbackSetting));

    // Print result to console
#ifdef CANLIB_DEBUG
    printf("canlib_canSetLoopback:\n"
           "  canlibStatus          = %i (%s)\n"
           "  canlibHandle          = 0x%02X\n"
           "  canlibLoopbackSetting = %i\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibHandle,
           canlibLoopbackSetting);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canlibSetLoopback");
        return 0;
    }

    // Create a N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canClearErrorCounters(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Get callback info and argument list
    size_t argc = CANCLEARERRORCOUNTERS_ARGC;
    napi_value argv[CANCLEARERRORCOUNTERS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANCLEARERRORCOUNTERS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    uint32_t canlibHandle;
    status = napi_get_value_uint32(env, argv[0], &canlibHandle);
    assert(status == napi_ok);

    // Clear counter
    canStatus canlibStatus;
    canlibStatus = canIoCtl(canlibHandle, canIOCTL_CLEAR_ERROR_COUNTERS, 0, 0);

    // Print result to console
#ifdef CANLIB_DEBUG
    printf("canlib_canClearErrorCounters:\n"
           "  canlibStatus = %i (%s)\n",
           canlibStatus, canlibStatusLookup(canlibStatus));
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canlibSetLoopback");
        return 0;
    }

    // Create a N-API value for the result and return it
    napi_value result;
    status = napi_create_uint32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}




napi_value canlib_canGetFlagText(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANGETFLAGTEXT_ARGC;
    napi_value argv[CANGETFLAGTEXT_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANGETFLAGTEXT_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retreive argument
    uint32_t canFlagInt;
    status = napi_get_value_uint32(env, argv[0], &canFlagInt);
    assert(status == napi_ok);

    // Create buffer for error text
    char canFlagText[256] = { 0 };

    // Call helper function to generate string
    canStatus canlibStatus;
    canlibStatus = canlibGetFlagString(canFlagInt, canFlagText, sizeof(canFlagText));

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canGetFlagText: Error at canGetErrorText.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_string_utf8(env, canFlagText, NAPI_AUTO_LENGTH, &result);
    assert(status == napi_ok);

    return result;
}


