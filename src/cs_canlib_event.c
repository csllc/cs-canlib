/* N-API interface for Kvaser CANlib

   Functions that control the use of a Win32 event provided by the CANlib
   library to signal that received data is available for reading. When enabled,
   a worker thread is spawned that waits on this event and invokes a callback
   function when its state changes to signaled, which presumably calls canread()
   to retreive the newly received data.

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/


#include <stdio.h>       // provide printf
#include <windows.h>     // provide Win32 types, constants, and functions
#include <canlib.h>      // provide CANlib API functions
#include <canstat.h>     // provide CANlib status constants
#include <stdbool.h>     // provide true and false

#include "cs_canlib_event.h"


// ----------------------------------- // -----------------------------------
// Definitions


// Names used both in CreateEvent calls in main thread and OpenEvent calls
// in worker thread
const char canlibEventSpawnName[] = "canlibEventSpawn";
const char canlibEventExitName[] = "canlibEventExit";
const char canlibThreadHandleMutexName[] = "canlibThreadHandleMutex";




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables


// Handle for CAN bus read event, signaled my main thread (CANlib API) when
// a message has been received and waited on by worker thread
HANDLE canlibEventRead = INVALID_HANDLE_VALUE;


// Handle for worker thread spawn event, signaled by worker shortly after thread
// starts and waited on by main thread
HANDLE canlibEventSpawn = INVALID_HANDLE_VALUE;


// Handle for worker thread exit event, signaled by main thread during exit
// routine, and waited on by worker thread
HANDLE canlibEventExit = INVALID_HANDLE_VALUE;


// Worker thread handle, populated by canlibEventEnable
HANDLE canlibThread = INVALID_HANDLE_VALUE;


// Worker thread ID, populated by canlibEventEnable
DWORD canlibThreadId = 0;


// Worker thread's CAN interface handle, shared by main and worker thread
int canlibThreadHandle = 0;


// Mutex for worker thread's CAN interface handle, shared by main and worker thread
HANDLE canlibThreadHandleMutex = INVALID_HANDLE_VALUE;




// ----------------------------------- // -----------------------------------
// Local functions


DWORD WINAPI canlibEventThreadProc(LPVOID lpParam)
{
    int ret = 0;
    canStatus canlibStatus;

    // Copy parameters passed from the main thread
    threadParameters_t threadParams = { 0 };
    memcpy(&threadParams, (threadParameters_t*)lpParam, sizeof(threadParams));

#ifdef CANLIB_EVENT_DEBUG
    canlibDumpThreadParameters(&threadParams);
#endif

    // Open thread-local handles for events created in canlibEventEnable
    HANDLE canlibEventSpawn = INVALID_HANDLE_VALUE;
    HANDLE canlibEventExit = INVALID_HANDLE_VALUE;
    
    canlibEventSpawn = OpenEvent(EVENT_PERMISSIONS, // dwDesiredAccess
                                 false, // bInheritHandle
                                 canlibEventSpawnName);

    if (canlibEventSpawn == 0)
    {
        printf("canlibEventThreadProc: Error at OpenEvent(canlibEventSpawn): %i\n",
               GetLastError());
        return 1;
    }

    canlibEventExit = OpenEvent(EVENT_PERMISSIONS, // dwDesiredAccess
                                false, // bInheritHandle
                                canlibEventExitName);

    if (canlibEventExit == 0)
    {
        printf("canlibEventThreadProc: Error at OpenEvent(canlibEventExit): %i\n",
               GetLastError());
        return 1;
    }

    // Grab the mutex for the CAN interface handle global variable
    ret = WaitForSingleObject(canlibThreadHandleMutex, CANLIB_MUTEX_TIMEOUT);

    switch (ret)
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_ABANDONED:
        printf("canlibEventThreadProc: Error at WaitForSingleObject: WAIT_ABANDONED\n");
        return 1;
    case WAIT_TIMEOUT:
        printf("canlibEventThreadProc: Error at WaitForSingleObject: WAIT_TIMEOUT\n");
        return 1;
    case WAIT_FAILED:
        printf("canlibEventThreadProc: Error at WaitForSingleObject: WAIT_FAILED (0x%02X)\n",
               GetLastError());
        return 1;
    default:
        printf("canlibEventThreadProc: Error at WaitForSingleObject: ret = 0x%02X\n", ret);
        return 1;
    }

    // Open bus and get thread-specific handle, storing in shared global variable
    canlibThreadHandle = canOpenChannel(threadParams.canChannel,
                                        threadParams.canFlags);
    if (canlibThreadHandle < 0)
    {
        printf("canlibEventThreadProc: Error at canOpenChannel: 0x%02X\n",
               canlibThreadHandle);
        return 1;
    }
    else
    {
#ifdef CANLIB_EVENT_DEBUG
        printf("canlibThreadHandle = %i\n", canlibThreadHandle);
#endif
    }

    // Release the mutex
    ret = ReleaseMutex(canlibThreadHandleMutex);

    if (ret == 0)
    {
        printf("canlibEventThreadProc: Error at ReleaseMutex: 0x%02X\n",
               GetLastError());
        return 1;
    }

    // Go on-bus in the thread
    canlibStatus = canBusOn(canlibThreadHandle);
    if (canlibStatus < 0)
    {
        printf("canlibEventThreadProc: Error at canBusOn: 0x%02X\n",
               canlibStatus);
        return 1;
    }

    // Retrieve event handle that the CANlib API will use to communicate
    // CAN read events (and others) with the worker thread
    HANDLE canlibEventRead;
    canlibStatus = canIoCtl(canlibThreadHandle,
                            canIOCTL_GET_EVENTHANDLE,
                            &canlibEventRead,
                            sizeof(canlibEventRead));
    if (canlibStatus < 0)
    {
        printf("canlibEventThreadProc: Error at canIoCtl: 0x%02X\n",
               canlibStatus);
        return 1;
    }
    else
    {
#ifdef CANLIB_EVENT_DEBUG
        printf("canlibEventThreadProc: canlibEventRead = 0x%p\n", canlibEventRead);
#endif
    }

    // Signal the spawn event, indicating that the worker thread is running
    ret = SetEvent(canlibEventSpawn);
    if (ret == 0)
    {
        printf("canlibEventThreadProc: Error at SetEvent: 0x%02X\n",
               GetLastError());
        return 1;
    }
    
    // Wait for canlib or exit events to be signaled
    int threadExit = 0;
    HANDLE events[2] = { 0 };
    int eventIndex = 0;
    
    events[EVENT_INDEX_READ] = canlibEventRead;
    events[EVENT_INDEX_EXIT] = canlibEventExit;

    while (threadExit == 0)
    {
#ifdef CANLIB_EVENT_DEBUG
        printf("canlibEventThreadProc: Waiting for canlibEventRead or "
               "canlibEventExit...\n");
#endif
        ret = WaitForMultipleObjects(2, // nCount
                                     events, // lpHandles
                                     false, // bWaitAll
                                     INFINITE); // dwMilliseconds

        if (ret & WAIT_ABANDONED_0)
        {
            eventIndex = (ret ^ WAIT_ABANDONED_0);
            printf("canlibEventThreadProc: WAIT_ABANDONED_0: %i\n", eventIndex);
            return 1;
        }
        else if (ret == WAIT_TIMEOUT)
        {
            printf("canlibEventThreadProc: WAIT_TIMEOUT\n");
            return 1;
        }
        else if (ret == WAIT_FAILED)
        {
            printf("canlibEventThreadProc: WAIT_FAILED: %i\n", GetLastError());
            return 1;
        }
        else
        {
            // ret is (WAIT_OBJECT_0 | event_index), and WAIT_OBJECT_0 = 0,
            switch (ret)
            {
            case EVENT_INDEX_READ: // Execute callback
                (threadParams.callback)(canlibThreadHandle);
                break;
            case EVENT_INDEX_EXIT: // Clean up and exit thread
                threadExit = 1;
                break;
            }
        }
    }

#ifdef CANLIB_EVENT_WIN32_DEBUG
    printf("canlibEventThreadProc: Exiting thread\n");
#endif

    // Go off-bus in the thread
    canlibStatus = canBusOff(canlibThreadHandle);
    if (canlibStatus < 0)
    {
        printf("canlibEventThreadProc: Error at canBusOff: 0x%02X\n",
               canlibStatus);
        return 1;
    }

    // Close CAN channel
    canlibStatus = canClose(canlibThreadHandle);
    if (canlibStatus < 0)
    {
        printf("canlibEventThreadProc: Error at canClose: 0x%02X\n",
               canlibStatus);
        return 1;
    }

    // Close open events and mutex
    CloseHandle(canlibEventSpawn);
    CloseHandle(canlibEventExit);
    CloseHandle(canlibThreadHandleMutex);

    return 0;
}




// ----------------------------------- // -----------------------------------
// Public functions


int canlibEventEnable(int canlibChannel, int canlibFlags, void *callback)
{
    int ret = 0;

    // Create security attributes structure for events and worker thread
    SECURITY_DESCRIPTOR canlibSecurityDesc = { 0 };

    ret = InitializeSecurityDescriptor(&canlibSecurityDesc,
                                       SECURITY_DESCRIPTOR_REVISION);
    if (ret == 0)
    {
        printf("canlibEventEnable: Error at InitializeSecurityDescriptor: %i\n",
               GetLastError());
        return -1;
    }
    
    ret = SetSecurityDescriptorDacl(&canlibSecurityDesc,
                                    true,
                                    0,
                                    false);
    if (ret == 0)
    {
        printf("canlibEventEnable: Error at SetSecurityDescriptorDacl: %i\n",
               GetLastError());
        return -1;
    }

    SECURITY_ATTRIBUTES canlibSecurityAttr = { 0 };
    canlibSecurityAttr.nLength = sizeof(canlibSecurityAttr);
    canlibSecurityAttr.lpSecurityDescriptor = &canlibSecurityDesc;
    canlibSecurityAttr.bInheritHandle = true;

    // Create event object that worker thread will use to indicate it has started
    canlibEventSpawn = CreateEvent(&canlibSecurityAttr, // lpEventAttributes
                                   false, // bManualReset
                                   false, // bInitialState
                                   "canlibEventSpawn");

    if (canlibEventSpawn == 0)
    {
        printf("canlibEventEnable: Error at CreateEvent (canlibEventSpawn): 0x%02X\n",
               GetLastError());
        return -1;
    }

    // Create event object that main thread will use to stop worker thread
    canlibEventExit = CreateEvent(&canlibSecurityAttr, // lpEventAttributes
                                  false, // bManualReset
                                  false, // bInitialState
                                  "canlibEventExit");

    if (canlibEventExit == 0)
    {
        printf("canlibEventEnable: Error at CreateEvent (canlibEventExit): 0x%02X\n",
               GetLastError());
        return -1;
    }

    // Create mutex that will control access to a variable containing the thread's
    // own Canlib handle, canlibThreadHandle
    canlibThreadHandleMutex = CreateMutex(&canlibSecurityAttr, // libMutexAttributes
                                          true, // bInitialOwner
                                          canlibThreadHandleMutexName); // lpName

    if (canlibThreadHandleMutex == 0)
    {
        printf("canlibEventEnable: Error at CreateMutex: 0x%02X\n",
               GetLastError());
        return -1;
    }

    // Release the mutex immediately
    ret = ReleaseMutex(canlibThreadHandleMutex);

    if (ret == 0)
    {
        printf("canlibEventEnable: Error at ReleaseMutex: 0x%02X\n",
               GetLastError());
        return -1;
    }

    // Pack parameters into struct to be passed to worker thread
    threadParameters_t canlibEventThreadParams = {
        canlibChannel,
        canlibFlags,
        callback
    };

    // Spawn the worker thread that will monitor the event 
    canlibThread = CreateThread(&canlibSecurityAttr, //lpThreadAttributes
                                     0, // dwStackSize
                                     &canlibEventThreadProc, // lpStartAddress
                                     &canlibEventThreadParams, // lpParameter
                                     0, // dwCreationFlags
                                     &canlibThreadId); // lpThreadId

    if (canlibThread == 0)
    {
        printf("canlibEventEnable: Error at CreateThread (0x%02X)", GetLastError());
        return -1;
    }

    // Wait for both canlibEventSpawn to be set and for canlibThreadHandleMutex
    // to be released. Realistically, they should both happen in short succession
    // and waiting on canlibEventSpawn alone should be enough.
#ifdef CANLIB_EVENT_DEBUG
    printf("canlibEventEnable: Waiting for worker thread to start...\n");
#endif

    int eventIndex = 0;
    HANDLE events[2] = { canlibEventSpawn,
                         canlibThreadHandleMutex };

    ret = WaitForMultipleObjects(2, // nCount
                                 events, // lpHandles
                                 true, // bWaitAll
                                 INFINITE); // dwMilliseconds

    if (ret & WAIT_ABANDONED_0)
    {
        eventIndex = (ret ^ WAIT_ABANDONED_0);
        printf("canlibEventEnable: WAIT_ABANDONED_0: %i\n", eventIndex);
        return -1;
    }
    else if (ret == WAIT_TIMEOUT)
    {
        printf("canlibEventEnable: WAIT_TIMEOUT\n");
        return -1;
    }
    else if (ret == WAIT_FAILED)
    {
        printf("canlibEventEnable: WAIT_FAILED: %i\n", GetLastError());
        return -1;
    }
    else
    {
        // Otherwise, (ret & WAIT_OBJECT_0), indicating that both
        // canlibThreadEvent was signaled, and the mutex was released
    }

    return canlibThreadHandle;
}




int canlibEventDisable(void)
{
    int ret = 0;

    // Stop the worker thread
    ret = SetEvent(canlibEventExit);
    if (ret == 0)
    {
        printf("canlibEventDisable: Error at SetEvent: 0x%02X\n",
               GetLastError());
        return 1;
    }

    // Wait for the worker thread to exit
    int canlibThreadExitCode = 0;
    
#ifdef CANLIB_EVENT_DEBUG
    printf("canlibEventDisable: Waiting for worker thread to exit...\n");
#endif

    ret = WaitForSingleObject(canlibThread, INFINITE);

    switch (ret)
    {
    case WAIT_OBJECT_0:
        ret = GetExitCodeThread(canlibThread, &canlibThreadExitCode);

        if (ret == 0)
        {
            printf("canlibEventDisable: Error at GetExitCodeThread: 0x%02X\n",
                   GetLastError());
            return 1;
        }

        if (canlibThreadExitCode == STILL_ACTIVE)
        {
            printf("canlibEventDisable: Thread still active.\n");
            return 1;
        }
        else
        {
#ifdef CANLIB_EVENT_DEBUG
            printf("canlibEventDisable: Worker thread exited with code %i\n",
                   canlibThreadExitCode);
#endif
        }
        break;
    case WAIT_ABANDONED:
        printf("canlibEventDisable: Error at WaitForSingleObject: WAIT_ABANDONDED\n");
        return 1;
    case WAIT_TIMEOUT:
        printf("canlibEventDisable: Error at WaitForSingleObject: WAIT_TIMEOUT\n");
        return 1;
    case WAIT_FAILED:
        printf("canlibEventDisable: Error at WaitForSingleObject: WAIT_FAILED: 0x%02X\n",
               GetLastError());
        return 1;
    default:
        printf("canlibEventDisable: Error at WaitForSingleObject: ret = 0x%02X\n", ret);
        return 1;
    }

    // Close all thread and event handles that were created in EnableEvent (TODO)
    ret = CloseHandle(canlibEventSpawn);
    if (ret == 0)
    {
        printf("canlibEventDisable: Error at CloseHandle(canlibEventSpawn): %i\n",
               GetLastError());
        return 1;
    }

    ret = CloseHandle(canlibEventExit);
    if (ret == 0)
    {
        printf("canlibEventDisable: Error at CloseHandle(canlibEventExit): %i\n",
               GetLastError());
        return 1;
    }

    ret = CloseHandle(canlibThread);
    if (ret == 0)
    {
        printf("canlibEventDisable: Error at CloseHandle(canlibThread): %i\n",
               GetLastError());
        return 1;
    }

    return 0;
}




void canlibDumpThreadParameters(threadParameters_t *threadParameters)
{
    printf("threadParameters {\n"
           "  canChannel  = %i\n"
           "  canFlags    = 0x%02X\n"
           "  callback    = 0x%p\n"
           "}\n",
           threadParameters->canChannel,
           threadParameters->canFlags,
           threadParameters->callback);

    return;
}

