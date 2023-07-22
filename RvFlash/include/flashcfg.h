#ifndef FLASHCFG_H
#define FLASHCFG_H

#ifdef _WIN32
#include <windows.h>
#else
#include "WinTypes.h"
#endif
#include "sw_header.h"

#define bufsize      ((ULONGLONG)500 * 1024 * 1024) //100MByte
#define qspisize     ((ULONGLONG)2 * 1024 *1024)//2MByte for qspi flash
#define mtpsize      ((ULONGLONG)64 * 1024)
#define mtpsecsize   ((ULONGLONG)8 * 1024)
#define filesize64   ceil((float)flashCtl.fileSize / 8)
//#define mtpoffset    (flashCtl.address - STATION_SDIO_MTP_ADDR)
#define mtpoffset    (flashCtl.address & 0xffffff)
#define dramOffset   (flashCtl.address & 0xfffffff)
#define dramLenth    (bufsize - dramOffset)
#define mtplenth     (mtpsize - mtpoffset)
#define fillbyte     (0xff)
#define filluint     (0xffffffff)


enum flashtype
{
    erasetype  = 0x0,
    writetype  = 0x4,
    readtype   = 0x8,
    readouttype = 0xc,
    verifytype = 0x10,
    autotype   = 0x14,
    verifyerror = 0x18,
    eraseerror = 0x1c,
    writeerror = 0x20,
    readerror  = 0x24,
    readouterror = 0x28,

    flashtimer = 0x2c,
    holdReset  = 0x30,
    releaseReset = 0x34,
    setPc      = 0x38
};

struct flashCtrl
{
    BOOL eraseFlag = false;
    BOOL writeFlag = false;
    BOOL verifyFlag = false;
    BOOL readFlag   = false;
    BOOL runFlag    = false;
    BOOL autoEraseFlag  = false;
    BOOL autoWriteFlag  = true;
    BOOL autoVerifyFlag = false;
    BOOL autoRunFlag    = false;
    BOOL autoFlag = false;

    //char fileName[100] = ".\\test_case\\freertos_mtp_i2c0_32MHz.bin";
    char* fileName;
    BYTE connectStatus = false;
    BYTE proj = 0;
    ULONGLONG fileSize = mtpsize;
    ULONGLONG currentfileSize = mtpsize;
    ULONGLONG *writeBufferAddr;
    ULONGLONG *readBufferAddr;
    ULONGLONG address = STATION_SDIO_MTP_ADDR;
};

struct sysCtrl
{
    ULONGLONG pcAddr          = 0xffff0000;
    ULONGLONG rstAddr         = 0xffff0004;
    ULONGLONG pcValue         = 0x80000000;
    ULONGLONG holdRstValue    = 0x8000001f;
    ULONGLONG releaseRstValue = 0x80000000;
};

extern sysCtrl sysCtl;

extern flashCtrl flashCtl;

void flashctl_reset(void);

#endif // FLASHCFG_H
