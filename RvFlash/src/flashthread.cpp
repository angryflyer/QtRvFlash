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
    emit progress(0, "", (flashtimer | threadstart));
    if(flashCtl.proj == 1)
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
        flashWrite();
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

    if(flashCtl.proj == 1)
    {
        ft_open();
        sysSetPc();
        sysReleaseReset();
        ft_close();
    }

    ft_open();
    process_reset();
    ft_close();

    emit progress(filesize64, "", (flashtimer | threadend));
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
    BOOL rvStatus = FALSE;
    static time_t ltime;

    dwLegth = filesize64;

    sector = ceil((float)flashCtl.fileSize / (8 * 1024));

    sectorlocat = (float)(flashCtl.address - STATION_SDIO_MTP_ADDR) / mtpsecsize;

    sectorlocat = sectorlocat + 1;

    eraseprogress = ceil((float)flashCtl.fileSize / (8 * sector));

    ltime = time(NULL);
    fprintf(stderr,  "Start Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));

    //emit progress(0, "Start Erasing MTP All Sector @ %s\n", (erasetype | threadstart));

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
                emit progress((i * eraseprogress), "Done Erasing MTP All Sector @ %s\n", (erasetype | threadworking));
                rvStatus = rv_read(STATION_SDIO_MTP_SEC_ERASE_ADDR, &rdata);
            }while (rdata == 0x03);
        }
    }
    //  fprintf(stderr,  "Do get erase register=0x%x\n",rdata);
    ltime = time(NULL);
    fprintf(stderr,  "Done Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));
    emit progress(dwLegth, "Done Erasing MTP All Sector @ %s\n", (erasetype | threadend));
    ft_close();
}

BOOL FlashThread::flashWrite()
{
    ULONGLONG dwLegth = filesize64;
    ULONGLONG i = 0;
    BOOL rvStatus = FALSE;

    ft_open();

    emit progress(i, "Start Writing file.bin to memory @ %s\n", (writetype | threadstart));
    for (i = 0; i < dwLegth; i++)
    {
        rvStatus = rv_write(flashCtl.address + i * 8, flashCtl.writeBufferAddr[i]);
        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Writing data to memory @ addr 0x%llx, wdata @ 0x%llx\n", flashCtl.address + i * 8,  wdata_buffer[i]);
        emit progress(i, "Working...\n", (writetype | threadworking));
    }

    ft_close();

    if(FALSE == rvStatus) {
        emit progress(i, "Working...\n", (writeerror | threadworking));
        return FALSE;
    } else {
        emit progress(i, "Done Writing file.bin to memory @ %s\n", (writetype | threadend));
        return TRUE;
    }
}

BOOL FlashThread::flashReadOut()
{
    ULONGLONG dwLegth = filesize64;

    ULONGLONG i = 0;

    BOOL rvStatus = FALSE;

    ULONGLONG rdata;

    ft_open();

    emit progress(i, "Start Reading data out from memory @ %s\n", (readouttype | threadstart));

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
        rvStatus = rv_read(flashCtl.address + i * 8,&flashCtl.readBufferAddr[i]);

        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Reading data from memory @ addr 0x%llx, rdata @ 0x%llx\n", flashCtl.address + i * 8,  rdata_buffer[i]);
        emit progress(i, "Working...\n", readtype | threadworking);
    }

    ft_close();

    if(FALSE == rvStatus) {
        emit progress(i, "Working...\n", readouterror | threadworking);
        Sleep(20);
        return FALSE;
    }
    else
    {
        emit progress(dwLegth, "Done Reading data out from memory @ %s\n", (readouttype | threadend));
    }

    return TRUE;
}

BOOL FlashThread::flashRead()
{
    ULONGLONG dwLegth = filesize64;;

    ULONGLONG i = 0;

    BOOL rvStatus = FALSE;

    ULONGLONG rdata;

    ft_open();

    //emit progress(i, "Start Reading data from memory @ %s\n", readtype | threadstart);
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
        rvStatus = rv_read(flashCtl.address + i * 8, &flashCtl.readBufferAddr[i]);
        if(FALSE == rvStatus) {
            break;
        }
        //fprintf(stderr, "Do Reading data from memory @ addr 0x%llx, rdata @ 0x%llx\n", flashCtl.address + i * 8,  rdata_buffer[i]);
        emit progress(i, "Working...\n", (readtype | threadworking));
    }

    ft_close();

    if(FALSE == rvStatus) {
//        emit progress(i, "Working...\n", readerror | threadworking);
        Sleep(20);
        return FALSE;
    }
    emit progress(i, "Done Reading data from memory @ %s\n", readtype | threadend);
    return TRUE;
}

BOOL FlashThread::flashVerify()
{
    ULONGLONG dwLegth = filesize64;
    BOOL cmp_flag = TRUE;
    ULONGLONG i = 0;
    BOOL rvStatus;

    emit progress(0, "Start Verifing data from memory @ %s\n", (verifytype | threadstart));

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
                //fprintf(stderr, "Error Occured Verifing data from memory @ addr 0x%llx, rdata @ 0x%x, wdata @ 0x%x\n", flashCtl.address + i, flashCtl.mtpReadBuffer[i],flashCtl.mtpWriteBuffer[i]);
                break;
            }
        }
    }

    if((FALSE == cmp_flag) || (FALSE == rvStatus)) {
        emit progress(dwLegth, "Error Verifing data from memory @ %s\n", (verifyerror | threadend));
        return FALSE;
    } else {
        emit progress(dwLegth, "Done Verifing data from memory @ %s\n", (verifytype | threadend));
        return TRUE;
    }

}

void FlashThread::flashAuto()
{
    BOOL rvStatus;
    if(flashCtl.autoEraseFlag)
    {
        flashErase();
        Sleep(20);//delay for message handled
    }
    if(flashCtl.autoWriteFlag)
    {
        rvStatus = flashWrite();
        if(FALSE == rvStatus) return;
        Sleep(20);//delay for message handled
    }
    if(flashCtl.autoVerifyFlag)
    {
        rvStatus = flashVerify();
        if(FALSE == rvStatus) return;
    }
}
