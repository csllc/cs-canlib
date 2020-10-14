#ifndef _CS_CANLIB_EVENT_H_
#define _CS_CANLIB_EVENT_H_

#include <windows.h>


//#define CANLIB_EVENT_DEBUG
// Uncomment to enable debugging messages, which may affect timing or
// performance

// ----------------------------------- // -----------------------------------
// Definitions


// Structure of parameters passed from main thread to worker thread
typedef struct threadParameters_s
{
    int canChannel;
    int canFlags;
    void (*callback) (int);
} threadParameters_t;

// Security access requested for event objects and thread
#define EVENT_PERMISSIONS (EVENT_MODIFY_STATE | SYNCHRONIZE)

// Indices of events when packed into struct passed to WaitForMultipleObjects
// in pcanEventThreadProc
#define EVENT_INDEX_READ   (0)
#define EVENT_INDEX_EXIT   (1)

// Timeout, in ms, for accessing canlibThreadHandleMutex
#define CANLIB_MUTEX_TIMEOUT (1000)




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables




// ----------------------------------- // -----------------------------------
// Local functions




// ----------------------------------- // -----------------------------------
// Public functions

// Enable Win32 event signaling when data is received on CAN bus and available
// in buffer. Accepts callback function that is called when event is signaled:
// void (*callback) (int)
// where the int argument is the thread's handle for the open CAN channel,
// obtained when canOpenChannel is called during thread startup
// Returns negative value on failure
// Returns thread's CAN handle on success
int canlibEventEnable(int canlibChannel, int canlibFlags, void *callback);

// Disable the previously enabled Win32 event
// Returns 1 on failure
// Retruns 0 on success
int canlibEventDisable(void);

// Print the contents of a threadParameters_t structure to stdout in a
// human-readable format for debugging purposes
void canlibDumpThreadParameters(threadParameters_t *threadParameters);


#endif // _CS_CANLIB_EVENT_H_

