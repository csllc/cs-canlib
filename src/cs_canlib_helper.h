/* N-API interface for Kvaser CANlib

   Collection of functions that print CANlib structures to stdout and look up
   string representations of constants

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/


#ifndef _CANLIB_HELPER_H_
#define _CANLIB_HELPER_H_

#include <windows.h> // provide BYTE


// ----------------------------------- // -----------------------------------
// Definitions




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables




// ----------------------------------- // -----------------------------------
// Local functions




// ----------------------------------- // -----------------------------------
// Public functions


// Return pointer to string representation of a CANlib status code, given a
// status code defined in canstat.h
char *canlibStatusLookup(int canlibStatus);


// Print the contents of a buffer (uint8_t array) to stdout in a human-readable
// format for debugging purposes
void canlibDumpBuffer(BYTE *buffer, size_t bufferLength);


// Translate an integer baud value to a constant defined in canstat.h
int canlibTranslateBaud(int baudInt);


// Print the contents of a canBusStatistics struct to stdout in a human-readable
// format for debugging purposes
int canlibDumpBusStatistics(canBusStatistics *busStats);


// Build a string containing text corresponding to each canSTAT_ flag set in status
int canlibGetFlagString(int status, char *text, size_t text_len);


#endif // _CANLIB_HELPER_H_


