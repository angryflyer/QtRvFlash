#include "flashthread.h"
#include "flashrom.h"
#include <cmath>
#include <QString>

FlashThread::FlashThread(MainWindow* creator, QObject* parent) : QThread(parent)
{
	m_thread_creator = creator;
}

void FlashThread::run()
{
    emit progress(0, 0, "", (flashtimer | threadstart));
    if(flashCtl.proj != PEP)
    {
        ft_open();
        sysHoldReset();
        ft_close();
    }

    if(true == flashCtl.eraseFlag)
    {
        flashErase();
    }
    if(true == flashCtl.writeFlag)
    {
        // if datasize <= 64KB, single write
        if(filesize64 <= 8192)
        {
            flashWrite();
        } else
        {
            flashWriteBurst();
        }
    }
    if(true == flashCtl.readFlag)
    {
        flashReadOut();
    }
    if(true == flashCtl.verifyFlag)
    {
        flashVerify();
    }
    if(true == flashCtl.autoFlag)
    {
        flashAuto();
    }

    flashctl_reset();
    //recover filesize value
    flashCtl.fileSize = flashCtl.currentfileSize;

    if(flashCtl.proj != PEP)
    {
        ft_open();
        sysSetPc();
        sysReleaseReset();
        ft_close();
    }

    ft_open();
    process_reset();
    ft_close();

    emit progress(filesize64, 0, "", (flashtimer | threadend));
}

void FlashThread::flashErase()
{
    ft_open();
    ULONGLONG sector = 0;
    ULONGLONG rdata = 0xffffffff;
    ULONGLONG dwLegth = 0;
    ULONGLONG eraseprogress = 0;
    DWORD sectorlocat = 0;
    DWORD i = 0;
    float  speed = 0;
    BOOL   rvStatus = FALSE;
    static time_t ltime;

    dwLegth = filesize64;

    sector = ceil((float)flashCtl.fileSize / (8 * 1024));

    sectorlocat = (float)(flashCtl.address - STATION_SDIO_MTP_ADDR) / mtpsecsize;

    sectorlocat = sectorlocat + 1;

    eraseprogress = ceil((float)flashCtl.fileSize / (8 * sector));

    ltime = time(NULL);
    fprintf(stderr,  "Start Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));

    //emit progress(0, speed, "Start Erasing MTP All Sector @ %s\n", (erasetype | threadstart));

    rvStatus = rv_read(flashCtl.address, &rdata);
    //usleep(100 * 1000);
    do
    {
        rvStatus = rv_read(STATION_SDIO_MTP_DEEP_STANDBY_ADDR, &rdata);
    }
    while (0x2 == (0x2 & rdata));

    //erase mtp sector
    if((sectorlocat == 1) && (sector > 4))
    {
        //erase all mtp sector
        rvStatus = rv_write(STATION_SDIO_MTP_CHIP_ERASE_ADDR, 0x3);
        rvStatus = rv_read(STATION_SDIO_MTP_CHIP_ERASE_ADDR, &rdata);
        fprintf(stderr,  "do read reg mtp_erase_all=0x%llx\n", rdata);
        while (rdata != 0x1) {
            rvStatus = rv_read(STATION_SDIO_MTP_CHIP_ERASE_ADDR, &rdata);
        }
        fprintf(stderr,  "do read reg mtp_erase_all=0x%llx\n", rdata);
    }
    else
    {
        for (i = sectorlocat; i < (sectorlocat + sector); i++)
        {
            rvStatus = rv_write(STATION_SDIO_MTP_SEC_CONFIG_ADDR, i - 1);
            rvStatus = rv_write(STATION_SDIO_MTP_SEC_ERASE_ADDR, 0x03);
            do
            {
                fprintf(stderr,  "erasing mtp sector %lu...\r\n", i - 1);
                emit progress((i * eraseprogress), speed, "Done Erasing MTP All Sector @ %s\n", (erasetype | threadworking));
                rvStatus = rv_read(STATION_SDIO_MTP_SEC_ERASE_ADDR, &rdata);
            }while (rdata == 0x03);
        }
    }
    //  fprintf(stderr,  "Do get erase register=0x%x\n",rdata);
    ltime = time(NULL);
    fprintf(stderr,  "Done Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));
    emit progress(dwLegth, speed, "Done Erasing MTP All Sector @ %s\n", (erasetype | threadend));
    ft_close();
}

BOOL FlashThread::flashWrite()
{
    ULONGLONG dwLegth = filesize64;
    ULONGLONG i = 0;
    BOOL rvStatus = FALSE;
    struct timeval tvpre, tvafter;
    long   elapsed_time = 0;
    float  speed   = 0;

    ft_open();

    emit progress(i, speed, "Start Writing file.bin to memory @ %s\n", (writetype | threadstart));
    for (i = 0; i < dwLegth; i++)
    {
        // get pre time
        gettimeofday(&tvpre, NULL);
        // run rv_write
        rvStatus = rv_write(flashCtl.address + i * 8, flashCtl.writeBufferAddr[i]);
        // get after time
        gettimeofday(&tvafter, NULL);
        // get elapsed time/us
        elapsed_time = tvafter.tv_usec-tvpre.tv_usec;
        // calc speed -> KB/s
        speed = ((float)8 * 1000 * 1000 / 1024) / elapsed_time;
//        fprintf(stderr, "Timediff=%ldus, speed=%dKHz/s\n", (tvafter.tv_usec-tvpre.tv_usec), speed);
        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Writing data to memory @ addr 0x%llx, wdata @ 0x%llx\n", flashCtl.address + i * 8,  wdata_buffer[i]);
        emit progress(i, speed, "Working...\n", (writetype | threadworking));
    }

    ft_close();

    if(FALSE == rvStatus) {
        emit progress(i, speed, "Working...\n", (writeerror | threadworking));
        return FALSE;
    } else {
        emit progress(i, speed, "Done Writing file.bin to memory @ %s\n", (writetype | threadend));
        return TRUE;
    }
}

BOOL FlashThread::flashWriteBurst()
{
    // 32bit(4Byte) per burst
    ULONGLONG dwBurstLen = 1024;
    ULONGLONG dwBlockLen = ceil((float)flashCtl.fileSize / (4 * dwBurstLen));
    ULONGLONG dwAddrOffset;
    ULONGLONG i = 0;
    BOOL rvStatus = FALSE;
    struct timeval tvpre, tvafter;
    long   elapsed_time = 0;
    float  speed   = 0;

    ft_open();
    emit progress(i, speed, "Start Writing file.bin to memory @ %s\n", (writetype | threadstart));
    for (i = 0; i < dwBlockLen; i++)
    {
        dwAddrOffset = i * dwBurstLen * 4;
        // get pre time
        gettimeofday(&tvpre, NULL);
        // run rv_write
        rvStatus = rv_write_burst(flashCtl.address + dwAddrOffset, &flashCtl.writeBufferAddr[dwAddrOffset/8], dwBurstLen);
        // get after time
        gettimeofday(&tvafter, NULL);
        // get elapsed time/us
        elapsed_time = tvafter.tv_usec-tvpre.tv_usec;
        // calc speed -> KB/s
        speed = ((float)dwBurstLen * 4 * 1000 * 1000 / 1024) / elapsed_time;
//        fprintf(stderr, "Timediff=%ldus\n", (tvafter.tv_usec-tvpre.tv_usec));
//        fprintf(stderr, "speed=%.1fKB/s\n", speed);
        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Writing data to memory @ addr 0x%llx, wdata @ 0x%llx\n", flashCtl.address + i * 8,  wdata_buffer[i]);
        emit progress(dwAddrOffset/8, speed, "Working...\n", (writetype | threadworking));
    }

    ft_close();

    if(FALSE == rvStatus) {
        emit progress(dwAddrOffset/8, speed, "Working...\n", (writeerror | threadworking));
        return FALSE;
    } else {
        emit progress(dwAddrOffset/8, speed, "Done Writing file.bin to memory @ %s\n", (writetype | threadend));
        return TRUE;
    }
}

BOOL FlashThread::flashReadOut()
{
    ULONGLONG dwLegth = filesize64;

    ULONGLONG i = 0;

    BOOL rvStatus = FALSE;

    ULONGLONG rdata;

    struct timeval tvpre, tvafter;
    long   elapsed_time = 0;
    float  speed   = 0;

    ft_open();

    emit progress(i, speed, "Start Reading data out from memory @ %s\n", (readouttype | threadstart));

//    if(STATION_FLASH_FLASH_ADDR == flashCtl.address) {
//        do_write(STATION_FLASH_AUTO_MODE_DEEP_POWER_DOWN_ADDR,0x0);
//        usleep(300);
//    }
    if(STATION_FLASH_FLASH_ADDR == (flashCtl.address & STATION_FLASH_FLASH_ADDR)) {
        rvStatus = rv_read(STATION_FLASH_FLASH_ADDR, &rdata);
        usleep(300);
    }
    for (i = 0; i < dwLegth; i++)
    {
        // get pre time
        gettimeofday(&tvpre, NULL);
        // run rv_write
        rvStatus = rv_read(flashCtl.address + i * 8,&flashCtl.readBufferAddr[i]);
        // get after time
        gettimeofday(&tvafter, NULL);
        // get elapsed time/us
        elapsed_time = tvafter.tv_usec-tvpre.tv_usec;
        // calc speed -> KB/s
        speed = ((float)8 * 1000 * 1000 / 1024) / elapsed_time;
        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Reading data from memory @ addr 0x%llx, rdata @ 0x%llx\n", flashCtl.address + i * 8,  rdata_buffer[i]);
        emit progress(i, speed, "Working...\n", readouttype | threadworking);
    }

    ft_close();

    if(FALSE == rvStatus) {
        emit progress(i, speed, "Working...\n", readouterror | threadworking);
        usleep(20 * 1000);
        return FALSE;
    }
    else
    {
        emit progress(dwLegth, speed, "Done Reading data out from memory @ %s\n", (readouttype | threadend));
    }

    return TRUE;
}

BOOL FlashThread::flashRead()
{
    ULONGLONG dwLegth = filesize64;;

    ULONGLONG i = 0;

    BOOL rvStatus = FALSE;

    ULONGLONG rdata;

    struct timeval tvpre, tvafter;
    long   elapsed_time = 0;
    float  speed   = 0;

    ft_open();

    //emit progress(i, speed, "Start Reading data from memory @ %s\n", readtype | threadstart);
//    if(STATION_FLASH_FLASH_ADDR == flashCtl.address) {
//        do_write(STATION_FLASH_AUTO_MODE_DEEP_POWER_DOWN_ADDR,0x0);
//        usleep(300);
//    }
    if(STATION_FLASH_FLASH_ADDR == (flashCtl.address & STATION_FLASH_FLASH_ADDR)) {
        rvStatus = rv_read(STATION_FLASH_FLASH_ADDR, &rdata);
        usleep(300);
    }
    for (i = 0; i < dwLegth; i++)
    {
        // get pre time
        gettimeofday(&tvpre, NULL);
        // run rv_write
        rvStatus = rv_read(flashCtl.address + i * 8, &flashCtl.readBufferAddr[i]);
        if(FALSE == rvStatus) {
            break;
        }
        // get after time
        gettimeofday(&tvafter, NULL);
        // get elapsed time/us
        elapsed_time = tvafter.tv_usec-tvpre.tv_usec;
        // calc speed -> KB/s
        speed = ((float)8 * 1000 * 1000 / 1024) / elapsed_time;
        //fprintf(stderr, "Do Reading data from memory @ addr 0x%llx, rdata @ 0x%llx\n", flashCtl.address + i * 8,  rdata_buffer[i]);
        emit progress(i, speed, "Working...\n", (readtype | threadworking));
    }

    ft_close();

    if(FALSE == rvStatus) {
//        emit progress(i, speed, "Working...\n", readerror | threadworking);
        usleep(20 * 1000);
        return FALSE;
    }
    emit progress(i, speed, "Done Reading data from memory @ %s\n", readtype | threadend);
    return TRUE;
}

BOOL FlashThread::flashVerify()
{
    ULONGLONG dwLegth = filesize64;
    BOOL cmp_flag = TRUE;
    ULONGLONG i = 0;
    BOOL rvStatus;

    struct timeval tvpre, tvafter;
    long   elapsed_time = 0;
    float  speed   = 0;

    emit progress(0, speed, "Start Verifing data from memory @ %s\n", (verifytype | threadstart));

    rvStatus = flashRead();

    if(TRUE == rvStatus) {
        for (i = 0; i < flashCtl.fileSize; i++)
        {
            if (flashCtl.readBufferAddr[i] == flashCtl.writeBufferAddr[i])
            {
                cmp_flag = TRUE;
                //fprintf(stderr, "printf Verifing data from memory @ addr 0x%llx, rdata @ 0x%x, wdata @ 0x%x\n", flashCtl.address + i, flashCtl.mtpReadBuffer[i],flashCtl.mtpWriteBuffer[i]);
            }
            else
            {
                cmp_flag = FALSE;
                fprintf(stderr, "Error Occured Verifing data from memory @ addr 0x%llx, rdata @ 0x%x, wdata @ 0x%x\n", flashCtl.address + i, flashCtl.readBufferAddr[i],flashCtl.writeBufferAddr[i]);
                break;
            }
        }
    }

    if((FALSE == cmp_flag) || (FALSE == rvStatus)) {
        emit progress(dwLegth, speed, "Error Verifing data from memory @ %s\n", (verifyerror | threadend));
        return FALSE;
    } else {
        emit progress(dwLegth, speed, "Done Verifing data from memory @ %s\n", (verifytype | threadend));
        return TRUE;
    }

}

void FlashThread::flashAuto()
{
    BOOL rvStatus = FALSE;
    ULONGLONG dwLegth = filesize64;
    if(flashCtl.autoEraseFlag)
    {
        flashErase();
        usleep(20 * 1000);//delay for message handled
    }
    if(flashCtl.autoWriteFlag)
    {
        // if datasize <= 64KB, single write
        if(dwLegth <= 8192 || flashCtl.proj == PEP)
        {
            rvStatus = flashWrite();
        } else
        {
            rvStatus = flashWriteBurst();
        }
        if(FALSE == rvStatus) return;
        usleep(20 * 1000);//delay for message handled
    }
    if(flashCtl.autoVerifyFlag)
    {
        ft_freq   = (rvCfgInfo.speed < 2000) ? rvCfgInfo.speed : 2000;
        ft_wdelay = rvCfgInfo.wdelay;
        ft_rdelay = rvCfgInfo.rdelay;
        ft_close();
        ft_dev_init(ft_freq);
        ft_close();
        rvStatus = flashVerify();
        if(FALSE == rvStatus) return;
    }
}
