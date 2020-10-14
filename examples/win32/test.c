#include <stdio.h>
#include <windows.h>
#include <canlib.h>
#include <canstat.h>

typedef struct threadParamters_s {
    int channel;
    int flags;
} threadParameters_t;



DWORD WINAPI eventThreadProc(void* lpParams)
{
    int i;
    long id;
    BYTE msg[8];
    unsigned int dlc;
    unsigned int flag;
    unsigned int time;

    canStatus canlibStatus;

    int canlibHandleThread;
    int canlibChannel;
    int canlibFlags;

    canlibChannel = ((threadParameters_t*)lpParams)->channel;
    canlibFlags = ((threadParameters_t*)lpParams)->flags;

    HANDLE canlibEventHandle;

    printf("eventThreadProc running\n");

    canlibHandleThread = canOpenChannel(canlibChannel, canlibFlags);
    printf("canOpenChannel:\n"
           "  canlibHandleThread  = 0x%02X\n"
           "  canlibChannel       = 0x%02X\n"
           "  canlibFlags         = 0x%02X\n",
           canlibHandleThread,
           canlibChannel,
           canlibFlags);

    canlibStatus = canBusOn(canlibHandleThread);
    printf("canBusOn:\n"
           "  canlibHandleThread = 0x%02X\n"
           "  canlibStatus = %i\n",
           canlibHandleThread,
           canlibStatus);

    canlibStatus = canIoCtl(canlibHandleThread,
                            canIOCTL_GET_EVENTHANDLE,
                            &canlibEventHandle,
                            sizeof(canlibEventHandle));

    printf("canIoCtl:\n"
           "  canlibEventHandle = 0x%p\n",
           canlibEventHandle);

    BYTE echoMode = 0;
    
    canIoCtl(canlibHandleThread,
             canIOCTL_SET_LOCAL_TXECHO,
             &echoMode,
             sizeof(echoMode));

    while (1)
    {
        printf("waiting for event...\n");
        canlibStatus = WaitForSingleObject(canlibEventHandle, INFINITE);

        if (canlibStatus == canOK)
        {
            while (canlibStatus != canERR_NOMSG)
            {
                canlibStatus = canRead(canlibHandleThread,
                                       &id,
                                       &msg,
                                       &dlc,
                                       &flag,
                                       &time);

                if (canlibStatus == canOK)
                {
                    printf("canRead:\n"
                           "  canlibHandleThread = 0x%02X\n"
                           "  canlibStatus = %i\n"
                           "  id   = 0x%02X\n"
                           "  dlc  = 0x%02X\n"
                           "  flag = 0x%02X\n"
                           "  time = 0x%02X\n"
                           "  msg  = ",
                           canlibHandleThread,
                           canlibStatus,
                           id,
                           dlc,
                           flag,
                           time);

                    for (i = 0; i < dlc; i++)
                    {
                        printf("0x%02X ", msg[i]);
                    }
                    printf("\n");
                }
                else
                {
                    printf("canRead: canlibStatus = %i\n", canlibStatus);
                }
            }
        }
        else
        {
            printf("canWaitForEvent: 0x%02X\n", canlibStatus);
        }
    }

    return 0;
}




int main(void)
{
    int status;
    canStatus canlibStatus;

    canInitializeLibrary();

    int canlibHandle = 0;
    int canlibChannel = 0;
    int canlibFlags = 0;

    canlibHandle = canOpenChannel(canlibChannel, canlibFlags);
    printf("canOpenChannel:\n"
           "  canlibHandle  = 0x%02X\n"
           "  canlibChannel = 0x%02X\n"
           "  canlibFlags   = 0x%02X\n",
           canlibHandle,
           canlibChannel,
           canlibFlags);

    long freq;
    unsigned int tseg1;
    unsigned int tseg2;
    unsigned int sjw;
    unsigned int noSamp;
    unsigned int syncmode;

    canlibStatus = canGetBusParams(canlibHandle,
                                   &freq,
                                   &tseg1,
                                   &tseg2,
                                   &sjw,
                                   &noSamp,
                                   &syncmode);

    printf("canGetBusParams:\n"
           "  canlibHandle = 0x%02X\n"
           "  canlibStatus = %i\n"
           "  freq         = %i\n"
           "  tseg1        = 0x%02X\n"
           "  tseg2        = 0x%02X\n"
           "  sjw          = 0x%02X\n"
           "  noSamp       = 0x%02X\n"
           "  syncmode     = 0x%02X\n",
           canlibHandle,
           canlibStatus,
           freq,
           tseg1,
           tseg2,
           sjw,
           noSamp,
           syncmode);

    freq = 250000;
    tseg1 = 0x0B;
    tseg2 = 0x04;
    sjw = 0x04;
    noSamp = 0x03;
    syncmode = 0;

    canlibStatus = canSetBusParams(canlibHandle,
                                   freq,
                                   tseg1,
                                   tseg2,
                                   sjw,
                                   noSamp,
                                   syncmode);
    printf("canSetBusParams:\n"
           "  canlibHandle = 0x%02X\n"
           "  canlibStatus = %i\n"
           "  freq         = %i\n"
           "  tseg1        = 0x%02X\n"
           "  tseg2        = 0x%02X\n"
           "  sjw          = 0x%02X\n"
           "  noSamp       = 0x%02X\n"
           "  syncmode     = 0x%02X\n",
           canlibHandle,
           canlibStatus,
           freq,
           tseg1,
           tseg2,
           sjw,
           noSamp,
           syncmode);

    canlibStatus = canBusOn(canlibHandle);
    printf("canBusOn:\n"
           "  canlibHandle = 0x%02X\n"
           "  canlibStatus = %i\n",
           canlibHandle,
           canlibStatus);

    threadParameters_t threadParameters = {
        canlibChannel,
        canlibFlags
    };

    int canlibEventThreadId;
    HANDLE canlibEventThread;
    
    canlibEventThread = CreateThread(0,
                                     0,
                                     eventThreadProc,
                                     &threadParameters,
                                     0,
                                     &canlibEventThreadId);

    int canlibId = 1234;
    BYTE canlibMsg[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int canlibDlc = 8;
    int canlibFlag = 0x04;

    while (1) {
        canlibStatus = canWrite(canlibHandle,
                                canlibId,
                                canlibMsg,
                                canlibDlc,
                                canlibFlag);
        printf("canWrite:\n"
               "  canlibStatus = %i\n"
               "  canlibHandle = 0x%02X\n"
               "  canlibId     = 0x%02X\n"
               "  canlibMsg    = {...}\n"
               "  canlibDlc    = 0x%02X\n"
               "  canlibFlag   = 0x%02X\n",
               canlibStatus,
               canlibHandle,
               canlibId,
               canlibDlc,
               canlibFlag);

        Sleep(500);
    };

    return 0;
}



