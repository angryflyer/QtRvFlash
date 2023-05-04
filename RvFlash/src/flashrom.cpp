// flashrom.cpp : Defines the entry point for the console application.
//

#include <math.h>
#include "stdint.h"
#include "mytime.h"
#include "flashrom.h"

#define mtp_base_addr      STATION_SDIO_BASE_ADDR              //5000000-5010000, 8K*8(bank)=64K
#define mtp_erase_sector   STATION_SDIO_MTP_SEC_ERASE_ADDR
#define mtp_erase_all      STATION_SDIO_MTP_CHIP_ERASE_ADDR    //write 03 to erase all
#define mtp_sector_config  STATION_SDIO_MTP_SEC_CONFIG_ADDR    //config sector

//////////////////////////////////////////////////////////////////
void process_reset(void);
//////////////////////////////////////////////////////////////////

BOOL rv_read(ULONGLONG addr, ULONGLONG *data)
{
    ULONGLONG rdata = 0;
    ULONGLONG rdataH= 0;
    BOOL rvStatus = TRUE;
    BYTE dwCount = 3;
    BYTE addrBitW = 40;
    BYTE dataBitW = 64;
    addrBitW = (flashCtl.proj == 0) ? 40 : 32;
    dataBitW = (flashCtl.proj == 0) ? 64 : 32;
    //struct timeval tvpre, tvafter;
    //gettimeofday(&tvpre, NULL);
    do
    {
        rdata = 0;
        rdataH= 0;
        rvStatus  = ReadDataAndCheckACK(addr, &rdata, addrBitW, dataBitW);
        rvStatus &= (flashCtl.proj == 0) ? rvStatus : ReadDataAndCheckACK(addr + 4, &rdataH, addrBitW, dataBitW);
        dwCount--;
        if (dwCount == 0)
        {
            break;
        }
    } while (rvStatus == FALSE);
    *data = (rdataH << 32 | rdata);
//    fprintf(stderr, "*data=0x%llx\n", *data);
    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    return rvStatus;
}

BOOL rv_do_read(ULONGLONG addr, ULONGLONG *data)
{
    ULONGLONG rdata = 0;
    BOOL rvStatus = TRUE;
    BYTE dwCount = 3;
    BYTE addrBitW = 40;
    BYTE dataBitW = 64;
    addrBitW = (flashCtl.proj == 0) ? 40 : 32;
    dataBitW = (flashCtl.proj == 0) ? 64 : 32;
    //struct timeval tvpre, tvafter;
    //gettimeofday(&tvpre, NULL);
    do
    {
        rdata = 0;
        rvStatus  = ReadDataAndCheckACK(addr, &rdata, addrBitW, dataBitW);
        dwCount--;
        if (dwCount == 0)
        {
            break;
        }
    } while (rvStatus == FALSE);
    *data = rdata;
//    fprintf(stderr, "*data=0x%llx\n", *data);
    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    return rvStatus;
}

BOOL rv_write(ULONGLONG addr, ULONGLONG data)
{
    BOOL rvStatus;
    BYTE addrBitW = 40;
    BYTE dataBitW = 64;
    addrBitW = (flashCtl.proj == 0) ? 40 : 32;
    dataBitW = (flashCtl.proj == 0) ? 64 : 32;
    //struct timeval tvpre,tvafter;
    //gettimeofday(&tvpre, NULL);
    rvStatus  = WriteDataAndCheckACK(addr, data, addrBitW, dataBitW);
    rvStatus &= (flashCtl.proj == 0) ? rvStatus : WriteDataAndCheckACK(addr + 4, data >> 32, addrBitW, dataBitW);

    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //	//getchar();
    return rvStatus;
}

BOOL rv_write_burst(ULONGLONG addr, ULONGLONG *data, DWORD dwBurstLen)
{
    BOOL rvStatus;
    BYTE addrBitW = 40;
    BYTE dataBitW = 64;
    addrBitW = (flashCtl.proj == 0) ? 40 : 32;
    dataBitW = (flashCtl.proj == 0) ? 64 : 32;
    //struct timeval tvpre,tvafter;
    //gettimeofday(&tvpre, NULL);
//    fprintf(stderr, "enter rv_write_burst!\n");
    rvStatus  = WriteDataBurst(addr, data, addrBitW, dataBitW, dwBurstLen);

    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //	//getchar();
    return rvStatus;
}

BOOL rv_do_write(ULONGLONG addr, ULONGLONG data)
{
    BOOL rvStatus;
    BYTE addrBitW = 40;
    BYTE dataBitW = 64;
    addrBitW = (flashCtl.proj == 0) ? 40 : 32;
    dataBitW = (flashCtl.proj == 0) ? 64 : 32;
    //struct timeval tvpre,tvafter;
    //gettimeofday(&tvpre, NULL);
    rvStatus  = WriteDataAndCheckACK(addr, data, addrBitW, dataBitW);
    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //	//getchar();
    return rvStatus;
}

void process_reset(void)//reset target such as pep chip
{
	set_reset(); // set reset
    fprintf(stderr, "Set RESET\n");
	usleep(50000);
	release_reset(); // release reset
    fprintf(stderr, "Release RESET\n");
}

BOOL chipConnect(void)
{
    uint64_t rdata = 0xffffffff;
    uint64_t value;
    uint64_t mtimecmp_v;
    uint64_t mtime_v;
    uint64_t mie_backup;
    BOOL rvStatus;

    static time_t ltime;
    ltime = time(NULL);
    fprintf(stderr, "Start connecting to soc @ %s\n", asctime(localtime(&ltime)));

    rvStatus = rv_read(STATION_SLOW_IO_SCU_FRST2S_BLOCKS_ADDR, &rdata);

    if(0x303 == rdata) {
        rvStatus = rv_write(STATION_ORV32_S2B_CFG_RST_PC_ADDR, mtp_base_addr); //set rst pc to mtp base addr
        //do_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR, 0x07000363);//enable_all_icg
        rvStatus |= rv_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR, 0x37ffffff);//enable_all_icg
        usleep(10 * 1000);
        //do_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR, 0x07000363);//hold_orv32_reset
        rvStatus |= rv_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR, 0x37fffffB);//hold_orv32_reset

        rvStatus |= rv_write(STATION_SLOW_IO_SCU_IC_DC_MODE_ADDR, 0x03); // 0K IC + 64K DC
        usleep(10 * 1000);
    } else {
        //stall or halt
        rvStatus = rv_write(STATION_ORV32_S2B_DEBUG_STALL_ADDR, 1);
        //do_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR, 0x07000363);//enable_all_icg
        rvStatus |= rv_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR, 0x37ffffff);//enable_all_icg
        usleep(10 * 1000);
        //do_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR, 0x07000363);//hold_orv32_reset
        rvStatus |= rv_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR, 0x37fffffB);//hold_orv32_reset

        rvStatus |= rv_write(STATION_SLOW_IO_SCU_IC_DC_MODE_ADDR, 0x03); // 0K IC + 64K DC
        usleep(10 * 1000);
    }

    rvStatus |= rv_read(STATION_DMA_SYSTEM_CFG_ADDR, &rdata);

    if (((rdata & 0xffffffff) != 0x8c00c801) || (rvStatus == FALSE))
    {
        ltime = time(NULL);
        fprintf(stderr, "Fail connecting to soc @ %s\n", asctime(localtime(&ltime)));
        return false;
    }

    ltime = time(NULL);
    fprintf(stderr, "Done connecting to soc @ %s\n", asctime(localtime(&ltime)));
    return true;
}

BOOL flashErase(ULONGLONG dwAddr, ULONGLONG dwLegth)
{
    uint64_t sector = 0;
    uint64_t rdata = 0xffffffff;
    static time_t ltime;

    sector = ceil((float)dwLegth / (8 * 1024));

    ltime = time(NULL);
    fprintf(stderr, "Start Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));
    do_read(mtp_base_addr);
    //usleep(100 * 1000);
    while (0x2 == (0x2 & do_read(STATION_SDIO_MTP_DEEP_STANDBY_ADDR)));
    //erase mtp sector
    if (sector <= 4) {
        for (DWORD i = 1; i <= sector; i++) {
            do_write(mtp_sector_config, i - 1);
            do_write(mtp_erase_sector, 0x03);
            while (do_read(mtp_erase_sector) == 0x03) {
                fprintf(stderr, "erasing mtp sector %lu...\r\n", i - 1);
            }
        }
    }
    else {
        //erase all mtp sector
        do_write(mtp_erase_all, 0x3);
        rdata = do_read(mtp_erase_all);
        fprintf(stderr, "do read reg mtp_erase_all=0x%llx\n", rdata);
        while (rdata != 0x1) {
            rdata = do_read(mtp_erase_all);
        }
        fprintf(stderr, "do read reg mtp_erase_all=0x%llx\n", rdata);
    }
    //  fprintf(stderr, "Do get erase register=0x%x\n",rdata);
    ltime = time(NULL);
    fprintf(stderr, "Done Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));
    fprintf(stderr, "Do Read 0x5000000 from MTP @ %llx\n", do_read(0x5000000));
    return true;
}

BOOL flashWrite(ULONGLONG dwAddr, BYTE* dwData, ULONGLONG dwLegth)
{
    uint64_t wdata_buffer[8 * 1024] = { 0xffffffff };
    uint64_t* wdata_temp = (uint64_t* )dwData;
    static time_t ltime;

    dwLegth = ceil((float)dwLegth / 8);

    for (DWORD i = 0; i < 8 * 8 * 1024 / 8; i++)
    {
        wdata_buffer[i] = 0xffffffff;
    }

    for (DWORD i = 0; i < dwLegth; i++)
    {
        wdata_buffer[i] = wdata_temp[i];
    }

    ltime = time(NULL);
    fprintf(stderr, "Start Writing file.bin to MTP @ %s\n", asctime(localtime(&ltime)));

    for (DWORD i = 0; i < dwLegth; i++)
    {
        do_write(mtp_base_addr + i * 8, wdata_buffer[i]);
    }
    ltime = time(NULL);
    fprintf(stderr, "Done Writing file.bin to MTP @ %s\n", asctime(localtime(&ltime)));

    fprintf(stderr, "Do Read 0x5000000 from MTP @ %llx\n", do_read(0x5000000));
    return true;
}

BOOL flashRead(ULONGLONG dwAddr, BYTE* dwGetData, ULONGLONG dwLegth)
{
    uint64_t rdata_buffer[8 * 1024] = { 0xffffffff };
    uint64_t* rdata_temp = (uint64_t*) dwGetData;

    static time_t ltime;

    dwLegth = ceil((float)dwLegth / 8);

    ltime = time(NULL);
    fprintf(stderr, "Start Reading data from MTP @ %s\n", asctime(localtime(&ltime)));
    for (DWORD i = 0; i < dwLegth; i++)
    {
        rdata_buffer[i] = do_read(mtp_base_addr + i * 8);
    }

    for (DWORD i = 0; i < dwLegth; i++)
    {
        rdata_temp[i] = rdata_buffer[i];
    }
    ltime = time(NULL);
    fprintf(stderr, "Done Reading data from MTP @ %s\n", asctime(localtime(&ltime)));
    fprintf(stderr, "Do Read rdata_temp[0] from MTP @ %llx\n", rdata_temp[0]);
    return true;
}

BOOL sysHoldReset()
{
    BOOL rvStatus = 0;
    ULONGLONG rdata = 0;
    BYTE dwCount  = 1;
    BYTE addrBitW = 32;
    BYTE dataBitW = 32;
    if(flashCtl.proj == 1)
    {
        do
        {
            rdata = 0;
            rvStatus  = ReadDataAndCheckACK(sysCtl.rstAddr, &rdata, addrBitW, dataBitW);
            dwCount--;
            if (dwCount == 0)
            {
                break;
            }
        } while (rvStatus == FALSE);
        rdata    = (rdata & 0x7fffffe0) | 0x8000001f;
        rvStatus &= WriteDataAndCheckACK(sysCtl.rstAddr, rdata, addrBitW, dataBitW);
    }

    return rvStatus;
}

BOOL sysReleaseReset()
{
    BOOL rvStatus = 0;
    BYTE addrBitW = 32;
    BYTE dataBitW = 32;
    if(flashCtl.proj == 1)
    {
        rvStatus = WriteDataAndCheckACK(sysCtl.rstAddr, 0x80000000, addrBitW, dataBitW);
    }

    return rvStatus;
}

BOOL sysSetPc()
{
    BOOL rvStatus = 0;
    BYTE addrBitW = 32;
    BYTE dataBitW = 32;
    if(flashCtl.proj == 1)
    {
        rvStatus = WriteDataAndCheckACK(sysCtl.pcAddr, sysCtl.pcValue, addrBitW, dataBitW);
    }

    return rvStatus;
}

