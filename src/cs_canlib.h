/* N-API interface for Kvaser CANlib

   N-API callable functions that directly correspond to CANlib functions, 
   and additional wrapper functions

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/


#ifndef _CS_CANLIB_H_
#define _CS_CANLIB_H_

#include <node_api.h> // provide N-API types

//#define CANLIB_DEBUG
// Uncomment to enable debugging messages, which may affect timing or
// performance


// ----------------------------------- // -----------------------------------
// Definitions

#define CANINITIALIZELIBRARY_ARGC   (0)
#define CANOPENCHANNEL_ARGC         (2)
#define CANCLOSE_ARGC               (1)
#define CANSETBUSOUTPUTCONTROL_ARGC (2)
#define CANBUSON_ARGC               (1)
#define CANBUSOFF_ARGC              (1)
#define CANWRITE_ARGC               (2)
#define CANWRITESYNC_ARGC           (2)
#define CANREAD_ARGC                (1)
#define CANREADWAIT_ARGC            (2)
#define CANCHANNELINFO_ARGC         (0)
#define CANTRANSLATEBAUD_ARGC       (1)
#define CANSETBUSPARAMS_ARGC        (2)
#define CANGETBUSPARAMS_ARGC        (1)
#define CANFLUSHTRANSMITQUEUE_ARGC  (1)
#define CANFLUSHRECEIVEQUEUE_ARGC   (1)
#define CANREADSTATUS_ARGC          (1)
#define CANRESETBUS_ARGC            (1)
#define CANSETACCEPTANCEFILTER_ARGC (4)
#define CANENABLEEVENT_ARGC         (3)
#define CANDISABLEEVENT_ARGC        (1)
#define CANSETLOOPBACK_ARGC         (2)
#define CANCLEARERRORCOUNTERS_ARGC  (1)




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables




// ----------------------------------- // -----------------------------------
// Local functions

// Initialize N-API module
napi_value Init(napi_env env, napi_value exports);


// Wrapper callback function that calls the napi_threadsafe_function specified
// in canlib_canEnableEvent
void canlib_canEventCallback(int canlibHandle);


// Dummy finalize callback for the threadsafe function used by
// canlib_canEventCallback
void canlib_canEventFinalize(napi_env env, void* finalize_data, void* finalize_hint);




// ----------------------------------- // -----------------------------------
// Public functions


// Initialize CANlib library
// Arguments passed through N-API:
// - (none)
// Returns nothing; any failure encountered by this function will be reported
// by subsequent calls to canOpenChannel
napi_value canlib_canInitializeLibrary(napi_env env, napi_callback_info info);


// Opens instance of CAN channel for the calling thread
// Arguments passed through N-API:
// - int channel
// - int flags
// Returns thread-specific handle (int), wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canOpenChannel(napi_env env, napi_callback_info info);


// Closes instance of CAN channel for the calling thread
// Arguments pasesd through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canClose(napi_env env, napi_callback_info info);


// Brings a thread's handle to its CAN channel on-bus
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canBusOn(napi_env env, napi_callback_info info);


// Takes a thread's handle to its CAN channel off-bus
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canBusOff(napi_env env, napi_callback_info info);


// Translates a CANlib API baud constant to corresponding bus parameter values
// Arguments passed through N-API:
// - int32_t freq
// This function actually is a wrapper for CANlib's canTranslateBaud. It
// accepts an integer frequency and uses a helper function to translate it
// to one of the API's constants before passing it to canTranslateBaud
// Returns N-API object containing the following properties:
// - freq (int32)
// - tseg1 (uint32)
// - tseg2 (uint32)
// - sjw (uint32)
// - noSamp (uint32)
// Note that syncmode is omitted. Error is thrown upon failure
napi_value canlib_canTranslateBaud(napi_env env, napi_callback_info info);


// Sets bus timing parameters for the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// - BusParams (N-API object)
// BusParams is an object containing the following properties:
// - freq (int32)
// - tseg1 (uint32)
// - tseg2 (uint32)
// - sjw (uint32)
// - noSamp (uint32)
// Note that syncmod is omitted. Returns canStatus from CANlib API call, wrapped in
// napi_value, and error is thrown upon failure.
napi_value canlib_canSetBusParams(napi_env env, napi_callback_info info);


// Retrieves current bus timing parameters for the specified CAN handle
// Arguments passed through N-API:
// - int handle
// Returns N-API object containing the following properties:
// - freq (int32)
// - tseg1 (uint32)
// - tseg2 (uint32)
// - sjw (uint32)
// - noSamp (uint32)
// - syncmode (uint32)
// Error is thrown upon failure
napi_value canlib_canGetBusParams(napi_env env, napi_callback_info info);


// Queues message for transmission on CAN bus using the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// - msg (N-API object)
// msg is an object containing the following propertie:
// - id (int32)
// - buf (Buffer)
// - dlc (uint32)
// - flag (uint32)
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canWrite(napi_env env, napi_callback_info info);


// Blocks until all queued messages have been written to the CAN bus
// Arguments passed through N-API:
// - int handle
// - unsigned int timeout
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canWriteSync(napi_env env, napi_callback_info info);


// Attempt to read a single message from the CAN bus using the thread-specific
// CAN handle
// Arguments passed through N-API:
// - int handle
// Returns N-API object containing the following properties:
// - id (uint32)
// - buf (Buffer)
// - dlc (uint32)
// - flag (uint32)
// - time (uint32)
// Error is thrown upon failure. Particularly, err.code == "No messages available"
// when the read is unsucessful because there are no messages in the receive
// buffer.
napi_value canlib_canRead(napi_env env, napi_callback_info info);


// Attempt to read a single message from the CAN bus using the thread-specific
// CAN handle, waiting until a messages is received or the a timeout expires
// Arguments passed through N-API:
// - int handle
// - int timeout
// Returns N-API object containing the following properties:
// - id (uint32)
// - buf (Buffer)
// - dlc (uint32)
// - flag (uint32)
// - time (uint32)
// Error is thrown upon failure. Particularly, err.code == "No messages available"
// when the read is unsucessful because there are no messages in the receive
// buffer.
napi_value canlib_canReadWait(napi_env env, napi_callback_info info);


// Configure acceptance filter for the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// - unsigned int code
// - unsigned int mask
// - bool isExtended
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canSetAcceptanceFilter(napi_env env, napi_callback_info info);


// Flush the transmit buffer using the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canFlushTransmitQueue(napi_env env, napi_callback_info info);


// Flush the receive buffer using the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canFlushReceiveQueue(napi_env env, napi_callback_info info);


// Read the CAN controller's status flags using the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// Returns unsigned long int containing status flags, wrapped in napi_value, and
// error is thrown upon failure
napi_value canlib_canReadStatus(napi_env env, napi_callback_info info);


// Attempt to reset the CAN bus. Hardware only resets if the handle passed to the
// function is the only open handle for the channel.
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canResetBus(napi_env env, napi_callback_info info);


// Retrieve information for all available CAN channels
// Arguments passed through N-API:
// - (none)
// Returns N-API array containing objects with the following properties:
// - channel (int)
// - channel_type (int)
// Array indicies correspond to channel numbers as reported by the API.
// Error is thrown upon failure.
napi_value canlib_canChannelInfo(napi_env env, napi_callback_info info);


// Given an N-API callback function, enable the message receive Win32 event
// and spawn a worker thread that calls the callback function when a new message
// is received.
// Arguments passed through N-API:
// - int channel
// - unsigned int flags
// - callback (function)
// channel and flags should match the values passed to OpenChannel when the
// module is initialized
// Returns result of canlibEventEnable, wrapped in napi_value, and error is
// thrown upon failure. 0 is also returned upon failure.
napi_value canlib_canEnableEvent(napi_env env, napi_callback_info info);


// Disable a previously enabled message receive Win32 event.
// Arguments passed through N-API:
// - int channel
// Returns result of canlibEventDisable, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canDisableEvent(napi_env env, napi_callback_info info);


// Sets the loopback mode for the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// - int loopbackSetting
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canSetLoopback(napi_env env, napi_callback_info info);


// Clears error counters for the thread-specific CAN handle
// Arguments passed through N-API:
// - int handle
// Returns canStatus from CANlib API call, wrapped in napi_value, and error is
// thrown upon failure
napi_value canlib_canClearErrorCounters(napi_env env, napi_callback_info info);


#endif // _CS_CANLIB_H_

