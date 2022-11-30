#ifndef FLASHCFG_H
#define FLASHCFG_H

#include <windows.h>
#include "sw_header.h"

#define qspisize     ((ULONGLONG)2 * 1024 *1024)//2MByte for qspi flash
#define mtpsize      ((ULONGLONG)64 * 1024)
#define mtpsecsize   ((ULONGLONG)8 * 1024)
#define filesize64   ceil((float)flashCtl.fileSize / 8)
//#define mtpoffset    (flashCtl.address - STATION_SDIO_MTP_ADDR)
#define mtpoffset    (flashCtl.address & 0xffffff)
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

    flashtimer = 0x2c
};

struct flashCtrl
{
    BOOL eraseFlag = false;
    BOOL writeFlag = false;
    BOOL verifyFlag = false;
    BOOL readFlag = false;
    BOOL autoEraseFlag = true;
    BOOL autoWriteFlag = true;
    BOOL autoVerifyFlag = true;
    BOOL autoFlag = false;

    //char fileName[100] = ".\\test_case\\freertos_mtp_i2c0_32MHz.bin";
    char* fileName;
    BYTE connectStatus = false;
    ULONGLONG fileSize = mtpsize;
    ULONGLONG currentfileSize = mtpsize;
    ULONGLONG address = STATION_SDIO_MTP_ADDR;
    BYTE mtpWriteBuffer[mtpsize] = {0xff};
    BYTE qspiWriteBuffer[qspisize] = {0xff};
    BYTE mtpReadBuffer[mtpsize] = {0xff};
    BYTE qspiReadBuffer[qspisize] = {0xff};
    BYTE readOutBuffer[qspisize] = {0xff};
};

extern flashCtrl flashCtl;

void flashctl_reset(void);

#endif // FLASHCFG_H
