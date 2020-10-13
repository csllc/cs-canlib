#include <stdio.h>
#include <windows.h>
#include <canlib.h>
#include <canstat.h>

int main(void)
{
    int status;
    canStatus canlibStatus;

    canInitializeLibrary();

    int canlibChannel = 0;
    int canlibFlags = 0;

    status = canOpenChannel(canlibChannel, canlibFlags);
    printf("canOpenChannel:\n"
           "  status        = 0x%02X\n"
           "  canlibChannel = 0x%02X\n"
           "  canlibFlags   = 0x%02X\n",
           status,
           canlibChannel,
           canlibFlags);


    long freq;
    unsigned int tseg1;
    unsigned int tseg2;
    unsigned int sjw;
    unsigned int noSamp;
    unsigned int syncmode;

    canlibStatus = canGetBusParams(0,
                                   &freq,
                                   &tseg1,
                                   &tseg2,
                                   &sjw,
                                   &noSamp,
                                   &syncmode);

    printf("canGetBusParams:\n"
           "  canlibStatus = %i\n"
           "  freq = %i\n"
           "  tseg1 = 0x%02X\n"
           "  tseg2 = 0x%02X\n"
           "  sjw = 0x%02X\n"
           "  noSamp = 0x%02X\n"
           "  syncmode = 0x%02X\n",
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

    canlibStatus = canSetBusParams(0,
                                   freq,
                                   tseg1,
                                   tseg2,
                                   sjw,
                                   noSamp,
                                   syncmode);
    printf("canSetBusParams:\n"
           "  canlibStatus = %i\n"
           "  freq = %i\n"
           "  tseg1 = 0x%02X\n"
           "  tseg2 = 0x%02X\n"
           "  sjw = 0x%02X\n"
           "  noSamp = 0x%02X\n"
           "  syncmode = 0x%02X\n",
           canlibStatus,
           freq,
           tseg1,
           tseg2,
           sjw,
           noSamp,
           syncmode);

    canlibStatus = canBusOn(0);
    printf("canBusOn:\n"
           "  canlibStatus = %i\n",
           canlibStatus);

    int i;
    long id;
    BYTE msg[8];
    unsigned int dlc;
    unsigned int flag;
    unsigned int time;

    
    HANDLE canlibEventHandle;

    canlibStatus = canIoCtl(0,
                            canIOCTL_GET_EVENTHANDLE,
                            &canlibEventHandle,
                            sizeof(canlibEventHandle));

    printf("canIoCtl:\n"
           "  canlibEventHandle = 0x%p\n",
           canlibEventHandle);


    while (1)
    {
        printf("waiting for event...\n");
        canlibStatus = WaitForSingleObject(canlibEventHandle, INFINITE);

        if (canlibStatus == canOK)
        {
            while (canlibStatus != canERR_NOMSG)
            {
                canlibStatus = canRead(0,
                                       &id,
                                       &msg,
                                       &dlc,
                                       &flag,
                                       &time);

                if (canlibStatus == canOK)
                {
                    printf("canRead:\n"
                           "  canlibStatus = %i\n"
                           "  id   = 0x%02X\n"
                           "  dlc  = 0x%02X\n"
                           "  flag = 0x%02X\n"
                           "  time = 0x%02X\n"
                           "  msg  = ",
                           canlibStatus,
                           id,
                           dlc,
                           flag,
                           time);

                    for (i = 0; i < dlc; i++)
                    {
                        printf("0x%02X ",
                               msg[i]);
                    }
                    printf("\n");
                }
                else
                {
                    printf("canRead: canlibStatus = %i\n", canlibStatus);
                }
            }
        } else {
            printf("canWaitForEvent: 0x%02X\n", canlibStatus);
        }
    }

    return 0;
}
