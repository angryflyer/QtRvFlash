#ifndef FLASHROM_H
#define FLASHROM_H

#include "sw_header.h"
#include "flashcfg.h"
#include "rvlink_ftd2xx.h"

int flashrom(int argc, const char * argv[]);

void process_reset(void);

BOOL chipConnect(void);

//BOOL flashErase(ULONGLONG dwAddr, ULONGLONG dwLegth);

//BOOL flashWrite(ULONGLONG dwAddr, BYTE* dwData, ULONGLONG dwLegth);

//BOOL flashRead(ULONGLONG dwAddr, BYTE* dwGetData, ULONGLONG dwLegth);

BOOL rv_write(ULONGLONG addr, ULONGLONG data);
BOOL rv_read(ULONGLONG addr, ULONGLONG *data);

#endif // FLASHROM_H
