#ifndef _CANLIB_HELPER_H_
#define _CANLIB_HELPER_H_

char *canlibStatusLookup(int canlibStatus);

void canlibDumpBuffer(BYTE *buffer, size_t bufferLength);

int canlibTranslateBaud(int baudInt);

#endif // _CANLIB_HELPER_H_
