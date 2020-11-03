/* N-API interface for Kvaser CANlib

   Collection of functions that print CANlib structures to stdout and look up
   string representations of constants

   Copyright (c) 2020 Control Solutions LLC. All rights reserved.
*/


#include <stdio.h>       // provide printf
#include <windows.h>     // provide MAX_PATH
#include <canlib.h>      // provide CANlib API functions
#include <canstat.h>     // provide CANlib status constants

#include "common_helper.h" // provide lookupString and lookup_t


// ----------------------------------- // -----------------------------------
// Definitions


const lookup_t canlibFlags[] = {
    { 0,                     "OK" },
    { canSTAT_ERROR_PASSIVE, "canSTAT_ERROR_PASSIVE" },
    { canSTAT_BUS_OFF,       "canSTAT_BUS_OFF" },
    { canSTAT_ERROR_WARNING, "canSTAT_ERROR_WARNING" },
    { canSTAT_ERROR_ACTIVE,  "canSTAT_ERROR_ACTIVE" },
    { canSTAT_TX_PENDING,    "canSTAT_TX_PENDING" },
    { canSTAT_RX_PENDING,    "canSTAT_RX_PENDING" },
    { canSTAT_RESERVED_1,    "canSTAT_RESERVED_1" },
    { canSTAT_TXERR,         "canSTAT_TXERR" },
    { canSTAT_RXERR,         "canSTAT_RXERR" },
    { canSTAT_HW_OVERRUN,    "canSTAT_HW_OVERRUN" },
    { canSTAT_SW_OVERRUN,    "canSTAT_SW_OVERRUN" },
    { canSTAT_OVERRUN,       "canSTAT_OVERRUN" },
    { 0, 0 }
};




// ----------------------------------- // -----------------------------------
// Global variables




// ----------------------------------- // -----------------------------------
// Local variables


char canlibStatusLookupBuffer[MAX_PATH] = { 0 };




// ----------------------------------- // -----------------------------------
// Local functions




// ----------------------------------- // -----------------------------------
// Public functions


char *canlibStatusLookup(int canlibStatus)
{
    canStatus status;

    status = canGetErrorText(canlibStatus, canlibStatusLookupBuffer,
                             sizeof(canlibStatusLookupBuffer));
    if (status != canOK)
    {
        memset(canlibStatusLookupBuffer, 0, sizeof(canlibStatusLookupBuffer));
        printf("canlibStatusLookup: Error at canGetErrorText for %i: %i",
               canlibStatus, status);
    }

    return canlibStatusLookupBuffer;
}




void canlibDumpBuffer(BYTE *buffer, size_t bufferLength)
{
    unsigned int i;

    printf("{");

    for (i = 0; i < bufferLength; i++)
    {
        printf("0x%02X", buffer[i]);
        if (i != (bufferLength - 1))
        {
            printf(", ");
        }
        // Print comma following data byte only if not second-to-last
        // byte in byte
    }

    printf("}");

    return;
}



int canlibTranslateBaud(int baudInt)
{
    int translated = 0;

    switch (baudInt) {
    case 1000000: // 1M
        translated = canBITRATE_1M;
        break;
    case 500000: // 500k
        translated = canBITRATE_500K;
        break;
    case 250000: // 250k
        translated = canBITRATE_250K;
        break;
    case 125000: // 125k
        translated = canBITRATE_125K;
        break;
    case 100000: // 100k
        translated = canBITRATE_100K;
        break;
    case 83000: // 83k
        translated = canBITRATE_83K;
        break;
    case 62000: // 62k
        translated = canBITRATE_62K;
        break;
    case 50000: // 50k
        translated = canBITRATE_50K;
        break;
    case 10000: // 10k
        translated = canBITRATE_10K;
        break;
    default:
        translated = 0; // Unknown
        break;
    }

    return translated;
}




int canlibDumpBusStatistics(canBusStatistics *busStats)
{
    printf("canlibDumpBusStatistics:\n"
           "  stdData = %lu\n"
           "  stdRemote = %lu\n"
           "  extData = %lu\n"
           "  extRemote = %lu\n"
           "  errFrame = %lu\n"
           "  busLoad = %lu\n"
           "  overruns = %lu\n",
           busStats->stdData,
           busStats->stdRemote,
           busStats->extData,
           busStats->extRemote,
           busStats->errFrame,
           busStats->busLoad,
           busStats->overruns);

    return 0;
}




int canlibGetFlagString(int status, char *text, size_t text_len)
{
    int i;
    int offset = 0;

    offset = snprintf(text, text_len, "canlib status: ");

    for (i = 0; canlibFlags[i].string; i++)
    {
        if (status & canlibFlags[i].value)
        {
            offset += snprintf(text+offset, text_len-offset, "%s (0x%04X) ",
                               canlibFlags[i].string, canlibFlags[i].value);
        }
    }

    return 0;
}

