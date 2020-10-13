#ifndef _CS_CANLIB_EVENT_H_
#define _CS_CANLIB_EVENT_H_

#include <windows.h>


#define CANLIB_EVENT_DEBUG
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

#define CANLIB_MUTEX_TIMEOUT (1000)




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables




// ----------------------------------- // -----------------------------------
// Local functions




// ----------------------------------- // -----------------------------------
// Public functions

int canlibEventEnable(int canlibChannel, int canlibFlags, void *callback);

int canlibEventDisable(void);

void canlibDumpThreadParameters(threadParameters_t *threadParameters);


#endif // _CS_CANLIB_EVENT_H_

