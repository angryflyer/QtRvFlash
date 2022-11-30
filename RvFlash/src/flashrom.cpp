// flashrom.cpp : Defines the entry point for the console application.
//

#include "flashrom.h"
#include "stdafx.h"
#include <windows.h>

//============================================================================
//  Use of FTDI D2XX library:
//----------------------------------------------------------------------------
//  Include the following 2 lines in your header-file
#pragma comment(lib, "FTD2XX.lib")
#include "ftd2xx.h"
//============================================================================
#include "string.h"
#include <stdlib.h>
#include <chrono>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <map>
#include <time.h>
#include "sw_header.h"

#define mtp_base_addr      STATION_SDIO_BASE_ADDR              //5000000-5010000, 8K*8(bank)=64K
#define mtp_erase_sector   STATION_SDIO_MTP_SEC_ERASE_ADDR
#define mtp_erase_all      STATION_SDIO_MTP_CHIP_ERASE_ADDR    //write 03 to erase all
#define mtp_sector_config  STATION_SDIO_MTP_SEC_CONFIG_ADDR    //config sector

void process_reset(void);

typedef struct {
	BYTE start;
	BYTE ack;
	BYTE parity;
	DWORD dwSeekCount;
	DWORD dwDataBitCount;
} rcvdat;

const BYTE AA_ECHO_CMD_1 = '\xAA';
const BYTE AB_ECHO_CMD_2 = '\xAB';
const BYTE BAD_COMMAND_RESPONSE = '\xFA';
const BYTE LOOP_BACK_ENA = '\x84';  //enable loop-back , internel connect TDI to TDO
const BYTE LOOP_BACK_DISA = '\x85'; //disable loop-back

const BYTE MSB_VEDGE_CLOCK_IN_BIT = '\x22';
const BYTE MSB_EDGE_CLOCK_OUT_BYTE = '\x11';
const BYTE MSB_EDGE_CLOCK_IN_BYTE = '\x24';

const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_IN = '\x24';
const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_OUT = '\x11';
const BYTE MSB_DOWN_EDGE_CLOCK_BIT_IN = '\x26';
const BYTE MSB_UP_EDGE_CLOCK_BYTE_IN = '\x20';
const BYTE MSB_UP_EDGE_CLOCK_BYTE_OUT = '\x10';
const BYTE MSB_RISING_EDGE_CLOCK_BIT_IN = '\x22';
const BYTE MSB_FALLING_EDGE_CLOCK_BIT_OUT = '\x13';

//  extern parameters
DWORD waitfreq = 2000;//KHz
DWORD waitlevel = 650;
DWORD ft_listdevices = false;

#define dwTesioWriteFlag           (UCHAR) 1
#define dwTesioReadFlag            (UCHAR) 0
#define dwTestioStart              (UCHAR) 0
#define dwTestioStop               (UCHAR) 1
#define dwTestioAck                (UCHAR) 0
#define waitByteTh                 (UCHAR) 12         //waitByte threshold
#define waitMtpEraseTime           (ULONG) 130*1000   //erase need at least 120ms,sector and chip erase:120ms, unit:us
#define waitMtpProgTime            (ULONG) 1*500     //normal at least 1ms mtp program byte:55us,unit:us
#define waitTime                   (ULONG) 50         //normal 50us for stable data,unit:us
#define rvlinkSpeed                (waitfreq)
#define AckWaitByte(waitime,freq)  ((waitime * freq / 1000) / 8) //convert wait time to number of bytes(clkout for receive only)
#define waitStableTime		       (((waitByteTh + 1) * 8) * 1.f / waitfreq)
//	#define waitStableTh               ceil((rvlinkSpeed <= 50) ? (waitStableTime * 4) : (rvlinkSpeed <= 100) ? (waitStableTime * 5) : (rvlinkSpeed < 1000) ? (waitStableTime * 20) : 4) //affect connect speed              
#define waitStableTh               (waitStableTime)
//  #define waitStableTh               (UCHAR) 0
//  #define waitDataStableTime         ((waitStableTime > waitStableTh) ? waitStableTime : waitStableTh)
#define waitDataStableTime         (waitStableTh >= 10 ? 5 * 1000 : waitStableTh * 1000)

#define waitMtpEraseByte           (AckWaitByte(waitMtpEraseTime,waitfreq) > waitByteTh ? AckWaitByte(waitMtpEraseTime,waitfreq) : waitByteTh)
#define waitMtpProgByte            (AckWaitByte(waitMtpProgTime,waitfreq) > waitByteTh ? AckWaitByte(waitMtpProgTime,waitfreq) : waitByteTh)
#define waitByte                   (AckWaitByte(waitTime,waitfreq) > waitByteTh ? AckWaitByte(waitTime,waitfreq) : waitByteTh)
#define rwcycle                    (DWORD) 1000
#define timeoutvalue               (DWORD) 300
//	#define Parity         1

/* 从data中获取第n bit的值 注：data只能为uint8*类型指针 */
#define GET_BIT_N_VAL(data, n)  \
            (0x1 & (( *((data) + (n)/8) & (0x1 << ((n)%8)) ) >> ((n)%8)))


FT_STATUS ftStatus;			//Status defined in D2XX to indicate operation result
FT_HANDLE ftHandle;			//Handle of FTD2XX device port 
BYTE OutputBuffer[1024] = {0};		//Buffer to hold MPSSE commands and data to be sent to FTD2XX
BYTE InputBuffer[65536] = {0};		//Buffer to hold Data bytes to be read from FTD2XX
BYTE dwDataTemp[65536] = {0};
BYTE dwDataRotate[65536] = { 0 };
DWORD dwClockDivisor = 0x001B;  //1000KHz
//DWORD dwClockDivisor = 0x0037;  //800KHz
//DWORD dwClockDivisor = 0x004A;  //Value of clock divisor, SCL Frequency = 60/((1+0x004A)*2) (MHz) = 400khz
//DWORD dwClockDivisor = 0x0095;  //200khz
DWORD dwNumBytesToSend = 0; 	//Index of output buffer
DWORD dwNumBytesSent = 0, dwNumBytesRead = 0, dwNumInputBuffer = 0;

BYTE dwAddr_Array[5] = { 0 };
BYTE dwData_Array[8] = { 0 };
BYTE STRB = 0xff;
DWORD timeoutcounter = 0;

BYTE ByteDataRead;//ByteAddress
BYTE ByteAddresshigh = 0x00, ByteAddresslow = 0x80;		//EEPROM address is '0x0080'
BYTE ByteDataToBeSend = 0x5A;							//data programmed and read
//////////////////////////////////////////////////////////////////
void usleep(unsigned long usec);
void gettimeofday(struct timeval* tv, void* tzp);
void setup(DWORD speed);
void set_reset(void);
void release_reset(void);
ULONGLONG do_read(ULONGLONG addr);
void do_write(ULONGLONG addr, ULONGLONG data);
void do_write_burst(ULONGLONG addr, ULONGLONG burst_data, DWORD burst_idx);
void set_gpiol3(BYTE gpiol3);
void process_reset(void);
////////////////////////////////////////////////////////////////// 
BOOL ft_device_init(DWORD speed);
BYTE soc_gen_even_parity_common(BYTE* entry_data, WORD entry_len);
BYTE soc_gen_odd_parity_common(BYTE* entry_data, WORD entry_len);
BOOL WriteDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG dwData);
BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG* dwGetData);
BYTE RotateLeft(BYTE* InputData, DWORD Lenth, DWORD shiftLenth);
BYTE InputDataHandle(ULONGLONG* dwDataHandled, rcvdat* dwAckData, BYTE* InputData, DWORD InputDataLenth, BYTE rwType);
BYTE ft_listdevice(void);
//////////////////////////////////////////////////////////////////

#if 0
int flashrom(int argc, const char * argv[])
{
	FILE* stream = NULL;
	static time_t ltime;
	uint64_t addr = 0;
	uint64_t start_addr = 0;
	uint64_t wdata = 0;
	uint64_t rdata = 0;
	uint64_t cmp_flag = 0;
	uint64_t lenth_stream = 0;
	uint64_t sector = 0;
	uint64_t reset = 0;
	uint64_t wdata_buffer[8 * 1024] = { 0xffffffff };
	uint64_t rdata_buffer[8 * 1024] = { 0xffffffff };
    char     filename[200] = "D:\\QtDemo\\build-Demo-Desktop_Qt_5_15_2_MSVC2019_32bit-Debug\\debug\\test_case\\freertos_mtp_i2c0_32MHz.bin";

	ltime = time(NULL);

	//setup ft_device

	int speed = 2000;//KHz
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "speed=", 6) == 0) {
			speed = atoi(argv[i] + 6);
		}
		else if (strncmp(argv[i], "reset", 5) == 0) { /* for script usage and dont need enter the internal handshake mode */
			reset = 1;
		}
		else if (strncmp(argv[i], "filename=", 9) == 0) { //specify file lacation and file name
			strcpy(filename, argv[i] + 9);
		}
		else if (strncmp(argv[i], "waitlevel=", 10) == 0) {
			waitlevel = atoi(argv[i] + 10);
		}
	}
	if (speed == 0) {
		speed = 2000; // Default
	}
	else if (speed > 4000)
	{
		speed = 4000;//speed <= sizeof(InputBuffer) * 8 / waitMtpEraseTime = 65536 * 8 / 130 = 4032KHz
	}
	waitfreq = speed;

    fprintf(stderr, "rvlink speed = %0d\n", speed);  //config rvlink speed
	setup(speed);

	if (reset == 1)
	{
		process_reset();    //reset target such as chip
	}

    fprintf(stderr, "filename=%s\n", filename);
    fprintf(stderr, "\n");
    fprintf(stderr, "############################################################\n");
    fprintf(stderr, "\n");

	stream = fopen(filename, "rb+");
	if (NULL == stream)
	{
        fprintf(stderr, "error!\n");
        //getchar();
    }
	fseek(stream, 0, SEEK_END);
	lenth_stream = ftell(stream);
	sector = ceil((float)lenth_stream / (8 * 1024));

	ltime = time(NULL);
    fprintf(stderr, "Do get flashrom size=%.1fK (%dBYTES) ", float(lenth_stream) / 1024, lenth_stream);
    fprintf(stderr, "@ %s\n", asctime(localtime(&ltime)));

	ltime = time(NULL);
    fprintf(stderr, "Do get erase sectors=%d ", sector);
    fprintf(stderr, "@ %s\n", asctime(localtime(&ltime)));

	ltime = time(NULL);
    fprintf(stderr, "Start connecting to soc @ %s\n", asctime(localtime(&ltime)));

	rdata = do_read(STATION_ORV32_S2B_CFG_RST_PC_ADDR);

	if ((rdata & 0xffffffff) == 0x0)
	{
		ltime = time(NULL);
        fprintf(stderr, "Fail connecting to soc @ %s\n", asctime(localtime(&ltime)));
        //getchar();
	}

	ltime = time(NULL);
    fprintf(stderr, "Done connecting to soc @ %s\n", asctime(localtime(&ltime)));

	ltime = time(NULL);
    fprintf(stderr, "Start Cleaning data buffer with 0xff @ %s\n", asctime(localtime(&ltime)));
	for (DWORD i = 0; i < 8 * 8 * 1024 / 8; i++)
	{
		wdata_buffer[i] = 0xffffffff;
		rdata_buffer[i] = 0xffffffff;
	}
	ltime = time(NULL);
    fprintf(stderr, "Done Cleaning data buffer with 0xff @ %s\n", asctime(localtime(&ltime)));

	fseek(stream, 0, SEEK_SET);
	fread(wdata_buffer, sizeof(uint64_t), ceil(lenth_stream / 8), stream);

	// if(reset == 1)
	// {
	//   process_reset();    //reset target such as chip
	// }

	do_write(STATION_ORV32_S2B_CFG_RST_PC_ADDR, mtp_base_addr); //set rst pc to mtp base addr
	do_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR, 0x07000363);//enable_all_icg
	usleep(10 * 1000);
	do_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR, 0x07000363);//hold_orv32_reset

	usleep(10 * 1000);

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
                fprintf(stderr, "erasing mtp sector %d...\r\n", i - 1);
			}
		}
	}
	else {
		//erase all mtp sector
		do_write(mtp_erase_all, 0x3);
		rdata = do_read(mtp_erase_all);
        fprintf(stderr, "do read reg mtp_erase_all=0x%x\n", rdata);
		while (rdata != 0x1) {
			rdata = do_read(mtp_erase_all);
		}
        fprintf(stderr, "do read reg mtp_erase_all=0x%x\n", rdata);
	}
    //  fprintf(stderr, "Do get erase register=0x%x\n",rdata);
	ltime = time(NULL);
    fprintf(stderr, "Done Erasing MTP All Sector @ %s\n", asctime(localtime(&ltime)));
	ltime = time(NULL);
    fprintf(stderr, "Start Writing file.bin to MTP @ %s\n", asctime(localtime(&ltime)));

	for (DWORD i = 0; i < ceil(lenth_stream / 8); i++)
	{
		do_write(mtp_base_addr + i * 8, wdata_buffer[i]);
	}
	ltime = time(NULL);
    fprintf(stderr, "Done Writing file.bin to MTP @ %s\n", asctime(localtime(&ltime)));

	usleep(100 * 1000);//delay for switch from program to read mode
	while (0x0 != do_read(STATION_SDIO_MTP_DEEP_STANDBY_ADDR));
	do_read(mtp_base_addr);//active mtp and clear bus data

	ltime = time(NULL);
    fprintf(stderr, "Start Verifing data from MTP @ %s\n", asctime(localtime(&ltime)));
	for (DWORD i = 0; i < ceil(lenth_stream / 8); i++)
	{
		rdata_buffer[i] = do_read(mtp_base_addr + i * 8);
	}


	for (DWORD i = 0; i < ceil(lenth_stream / 8); i++)
	{
		if (rdata_buffer[i] == wdata_buffer[i])
		{
			cmp_flag = TRUE;
		}
		else
		{
			cmp_flag = FALSE;
            fprintf(stderr, "Error Occured Verifing data from MTP @ addr 0x%x, rdata @ 0x%llx, wdata @ 0x%llx\n", mtp_base_addr + i * 8, rdata_buffer[i], wdata_buffer[i]);
			break;
		}
	}

	if (cmp_flag == FALSE)
	{
		ltime = time(NULL);
        fprintf(stderr, "Error Occured Verifing data from MTP @ %s\n", asctime(localtime(&ltime)));
	}
	else
	{
		ltime = time(NULL);
        fprintf(stderr, "Done Verifing data from MTP @ %s\n", asctime(localtime(&ltime)));
	}

	fclose(stream);
	// do_write(0x310,0x37ffffff);//en icg
	// usleep(10*1000);
	// do_write(STATION_SLOW_IO_SCU_S2ICG_BLOCKS_ADDR,0x37ffffff);//en icg
	// usleep(10*1000);
	// do_write(STATION_SLOW_IO_SCU_S2FRST_BLOCKS_ADDR,0x37ffffff);//release_orv32_reset 
	process_reset();

	ltime = time(NULL);
    fprintf(stderr, "Done Releasing ORV32 Reset @ %s\n", asctime(localtime(&ltime)));

	return 0;
}
#endif

void usleep(unsigned long usec)
{
	LARGE_INTEGER    dwStart;

	LARGE_INTEGER     dwCurrent;

	LARGE_INTEGER     dwFrequence;

	LONGLONG              counter;

	if (!QueryPerformanceFrequency(&dwFrequence))
	{
		return;
	}

	QueryPerformanceCounter(&dwStart);

	counter = dwFrequence.QuadPart * usec / 1000 / 1000;

	dwCurrent = dwStart;

	while ((dwCurrent.QuadPart - dwStart.QuadPart) < counter)
	{
		QueryPerformanceCounter(&dwCurrent);
	}
}

/*
void gettimeofday(struct timeval *tp, void *tzp)
{
  uint64_t intervals;
  FILETIME ft;

  GetSystemTimeAsFileTime(&ft);

 //  * A file time is a 64-bit value that represents the number
 //  * of 100-nanosecond intervals that have elapsed since
 //  * January 1, 1601 12:00 A.M. UTC.
 //  *
 //  * Between January 1, 1970 (Epoch) and January 1, 1601 there were
 //  * 134744 days,
 //  * 11644473600 seconds or
 //  * 11644473600,000,000,0 100-nanosecond intervals.
 //  *
 //  * See also MSKB Q167296.

intervals = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
intervals -= 116444736000000000;

tp->tv_sec = (long)(intervals / 10000000);
tp->tv_usec = (long)((intervals % 10000000) / 10);
}
*/

void gettimeofday(struct timeval* tv, void* tzp)
{

	auto time_now = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	auto duration_in_s = std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch()).count();
	auto duration_in_us = std::chrono::duration_cast<std::chrono::microseconds>(time_now.time_since_epoch()).count();

	tv->tv_sec = duration_in_s;
	tv->tv_usec = duration_in_us;
}

BYTE soc_gen_even_parity_common(BYTE* entry_data, WORD entry_len)
{
	DWORD i = 0;
	DWORD even_parity = 0;
	for (i = 0; i < entry_len; i++)
	{
		even_parity += GET_BIT_N_VAL((entry_data), i);
	}

	return (even_parity & 0x1);
}

BYTE soc_gen_odd_parity_common(BYTE* entry_data, WORD entry_len)
{
	DWORD i = 0;
	DWORD odd_parity = 0;

	for (i = 0; i < entry_len; i++)
	{
		odd_parity += odd_parity + GET_BIT_N_VAL((entry_data), i);
	}

	if (odd_parity % 2 == 0)
		return 1;
	else
		return 0;
}

BYTE RotateLeft(BYTE* InputData, DWORD Lenth, DWORD shiftLenth)
{
	BYTE valBit;
	DWORD LenthCount;
	DWORD shiftCount;
	DWORD dwcount;

	//InputData = (BYTE*)malloc(65536 * sizeof(BYTE));
	//BYTE* dataRotate = InputData;
	if (!Lenth) return false;
	shiftCount = shiftLenth;
    //fprintf(stderr, "shiftLenth=%d\n", shiftLenth);
	memcpy(dwDataRotate, InputData, Lenth);
	//for (dwcount = 0; dwcount < Lenth; dwcount++)
	//{
    //	fprintf(stderr, "dwDataRotate[%d]=0x%x\n", dwcount, dwDataRotate[dwcount]);
	//}
    //fprintf(stderr, "***************************************\n");
    ////getchar();
	while (shiftCount--)
	{
		//InputData = dataRotate;
        //fprintf(stderr, "shiftLenth=%d\n",shiftLenth);
        //fprintf(stderr, "Lenth=%d\n", Lenth);
		LenthCount = Lenth;
		LenthCount--;
		dwcount = 0;
		//	    valBit = *InputData >> 7;
		do
		{
			//*InputData = (*InputData << 1) | (*(InputData + 1) >> 7);
			//InputData++;
            //fprintf(stderr, "dwcount=%d\n", dwcount);
			dwDataRotate[dwcount] = (dwDataRotate[dwcount] << 1) | (dwDataRotate[dwcount + 1] >> 7);
            //fprintf(stderr, "dwDataRotate[%d]=0x%x\n", dwcount, dwDataRotate[dwcount]);
			dwcount++;
			//if (dwcount > 1000)
			//{
			//	//shiftCount = shiftLenth;
			//	//memcpy(dwDataRotate, InputData, Lenth);
			//	break;
			//}
		} while (LenthCount--);
		//	        *InputData = (*InputData << 1) | valBit;
		//*InputData = (*InputData << 1) | (*(InputData + 1) >> 7);
		//dwDataRotate[dwcount] = (dwDataRotate[dwcount] << 1) | (dwDataRotate[dwcount + 1] >> 7);
	}
	memcpy(InputData, dwDataRotate, Lenth);
	//for (dwcount = 0; dwcount < Lenth; dwcount++)
	//{
    //	fprintf(stderr, "dwDataRotate[%d]=0x%x\n", dwcount, dwDataRotate[dwcount]);
	//}
    ////getchar();
//	free(InputData);
	return true;
}

BYTE InputDataHandle(ULONGLONG* dwDataHandled, rcvdat* dwAckData, BYTE* InputData, DWORD InputDataLenth, BYTE rwType)
{
	DWORD dwSeekCount = 0, dwDataBitCount = 0;
	DWORD dwCount = 0;
	*dwDataHandled = 0;
	////
	memcpy(dwDataTemp, InputData, InputDataLenth);
	// for(dwCount=0; dwCount < InputDataLenth; dwCount ++)
	// {	
    // 	fprintf(stderr, "dwDataTemp[%d]=0x%llx\n",dwCount,dwDataTemp[dwCount]);
	// }	
	for (dwSeekCount = 0; dwSeekCount < InputDataLenth; dwSeekCount++)
	{
		for (dwDataBitCount = 0; dwDataBitCount < 8; dwDataBitCount++)
		{
            //			fprintf(stderr, "InputData[dwSeekCount%d] << dwDataBitCount%d=0x%x\n",dwSeekCount,dwDataBitCount,0xc0 & InputData[dwSeekCount] << dwDataBitCount);
			if (0x00 == (0xc0 & (InputData[dwSeekCount] << dwDataBitCount)))  //seek start and compare start and read addr ack,record the dwSeekCount and dwDataBitCount
			{
				dwAckData->start = 0;
				if ((dwDataBitCount == 7) && ((InputData[dwSeekCount + 1] >> dwDataBitCount) != 0x0)) //represent ack ok,check next array data
				{
					dwAckData->ack = 1;
				}
				else
				{
					dwAckData->ack = 0;
				}
                // fprintf(stderr, "dwSeekCount=%d\ndwDataBitCount=%d\n", dwSeekCount, dwDataBitCount);
                // fprintf(stderr, "InputData[dwSeekCount]=%x\nInputData[dwSeekCount+1]=%x\n", InputData[dwSeekCount], InputData[dwSeekCount+1]);
				break; //find start and break loop
			}
		}
		if ((dwSeekCount == (InputDataLenth - 1)) && (dwDataBitCount == 8))
		{
			*dwDataHandled = 0;
            //			fprintf(stderr, "error: do read incorrect ack\n");
			dwAckData->start = 1;
		}
		if (dwAckData->start == 0) break;
	}
	dwAckData->dwSeekCount = dwSeekCount;
	dwAckData->dwDataBitCount = dwDataBitCount;
    //	fprintf(stderr, "dwSeekCount=%d\ndwDataBitCount=%d\n", dwSeekCount, dwDataBitCount);
    //  fprintf(stderr, "dwSeekCount=%d\ndwDataBitCount=%d\ndwDataTemp[dwSeekCount]=0x%x\n \
	    InputDataLenth-dwSeekCount=%d\ndwDataBitCount + 2=%d\n", dwSeekCount,     \
		dwDataBitCount,dwDataTemp[dwSeekCount],InputDataLenth-dwSeekCount,dwDataBitCount + 2);

	//  RotateLeft(dwDataTemp,InputDataLenth,(dwSeekCount * 8 + dwDataBitCount + 2));//dwSeekCount bytes, remove bits start && ack bit
	//  RotateLeft(&dwDataTemp[dwSeekCount], InputDataLenth - dwSeekCount, dwDataBitCount + 2);//dwSeekCount bytes, remove bits start && ack bit
	//if (rwType == dwTesioReadFlag)
	//{
	//	for (dwCount = 0; dwCount < 8; dwCount++)
	//	{
    //		//		fprintf(stderr, "dwDataTemp[%d]=0x%llx\n",dwCount,dwDataTemp[dwCount]);
	//		*dwDataHandled = (*dwDataHandled << 8 | dwDataTemp[dwSeekCount + dwCount]);
	//	}
	//	dwAckData->parity = (dwDataTemp[dwSeekCount + dwCount] >> 7);
	//}
	//else
	//{
	//	dwAckData->parity = (dwDataTemp[dwSeekCount] >> 7);
	//}
        //fprintf(stderr, "dwDataHandled=0x%lx\n",*dwDataHandled);
	return true;
}

BOOL ft_device_init(DWORD speed)
{
	//////////////////////////////////////////////////////////
	/// INITIALIZE SECTION
	///////////////////////////////

	//////////////////////////////////////////////////////////////////
	// Define the variables used 
	//////////////////////////////////////////////////////////////////

	DWORD dwCount;
	DWORD devIndex = 0;
	char Buf[64];
	//	init linux envrionment
		// system("sudo lsmod | grep ftdi_sio");//avoid conflict
		// system("rmmod ftdi_sio");
		// system("rmmod usbserial");
		// system("ls /sys/bus/usb/drivers/ftdi_sio");

	dwClockDivisor = 30 * 1000 / speed - 1; //Value of clock divisor = (30 / freq -1) (30 & freq unit:MHZ)
//list device information!
	if (ft_listdevices == true)
	{
        ft_listdevice();
	}
	//return information of port marked devIndex,such as FT4232HL's devIndex(has 4 ports):0,1,2,3
	ftStatus = FT_ListDevices((PVOID)devIndex, &Buf, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
	ftStatus = FT_OpenEx((PVOID)Buf, FT_OPEN_BY_DESCRIPTION, &ftHandle);
    //    fprintf(stderr, "devIndex:%d\n",devIndex);

	if (ftStatus != FT_OK)
	{
        fprintf(stderr, "Can't open FTD2XX device! \n");
        //getchar();
		return false;
	}
	else
	{      // Port opened successfully
        fprintf(stderr, "Successfully get  channel A description:%s\n", Buf);
        fprintf(stderr, "Successfully open channel A of FTD2XX device! \n");

		ftStatus |= FT_ResetDevice(ftHandle); 	//Reset USB device
        //fprintf(stderr, "FT_ResetDevice!\n");
		//Purge USB receive buffer first by reading out all old data from FTD2XX receive buffer
		ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	 // Get the number of bytes in the FTD2XX receive buffer
        //fprintf(stderr, "FT_GetQueueStatus_dwNumInputBuffer!=%d\n",dwNumInputBuffer);
		if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
			FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  	//Read out the data from FTD2XX receive buffer
    //fprintf(stderr, "FT_Read_after!=%d\n",dwNumInputBuffer);
		ftStatus |= FT_SetUSBParameters(ftHandle, 65536, 65536);	//Set USB request transfer size
        //fprintf(stderr, "FT_SetUSBParameters!\n");
		ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);	 //Disable event and error characters
        //fprintf(stderr, "FT_SetChars!\n");
		ftStatus |= FT_SetTimeouts(ftHandle, 0, 5000);		//Sets the read and write timeouts in milliseconds for the FTD2XX
                //fprintf(stderr, "FT_SetTimeouts!\n");
		ftStatus |= FT_SetLatencyTimer(ftHandle, 16);		//Set the latency timer
        //fprintf(stderr, "FT_SetLatencyTimer!\n");
//		ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00); 		//Reset controller
        //fprintf(stderr, "FT_SetBitMode_Reset!\n");
		ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x02);	 	//Enable MPSSE mode
                //fprintf(stderr, "FT_SetBitMode!\n");
		if (ftStatus != FT_OK)
		{
            fprintf(stderr, "fail on initialize FTD2XX device! \n");
            //getchar();
			return false;
		}
		usleep(50 * 1000);	// Wait for all the USB stuff to complete and work

//                fprintf(stderr, "dwTestioStart Synchronize the MPSSE!\n");
		//////////////////////////////////////////////////////////////////
		// Synchronize the MPSSE interface by sending bad command ��0xAA��
		//////////////////////////////////////////////////////////////////
		OutputBuffer[dwNumBytesToSend++] = '\xAA';		//Add BAD command ��0xAA��
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
		dwNumBytesToSend = 0;			//Clear output buffer
		do {
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	 // Get the number of bytes in the device input buffer
		} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   	//or Timeout

            //fprintf(stderr, "dwNumInputBuffer=%d,ftStatus=%d\n",dwNumInputBuffer,ftStatus);

		BOOL bCommandEchod = false;

        //fprintf(stderr, "false=%d\n",false);
        //fprintf(stderr, "bCommandEchod=%d\n",bCommandEchod);

		ftStatus = FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  //Read out the data from input buffer

        //fprintf(stderr, "InputBuffer=0x%x,0x%x\ndwNumBytesRead=%d\n",InputBuffer[0],InputBuffer[1],dwNumBytesRead);

		for (dwCount = 0; dwCount < dwNumBytesRead - 1; dwCount++)	//Check if Bad command and echo command received
		{
			if ((InputBuffer[dwCount] == 0xfa) && (InputBuffer[dwCount + 1] == 0xaa))
			{
                //fprintf(stderr, "If_InputBuffer=0x%x,0x%x\n",InputBuffer[0],InputBuffer[1]);
				bCommandEchod = true;
				break;
			}
		}
		if (bCommandEchod == false)
		{
            fprintf(stderr, "fail to synchronize MPSSE with command '0xAA' \n");
            //getchar();
			return false; /*Error, can��t receive echo command , fail to synchronize MPSSE interface;*/
		}

		//////////////////////////////////////////////////////////////////
		// Synchronize the MPSSE interface by sending bad command ��0xAB��
		//////////////////////////////////////////////////////////////////
		//dwNumBytesToSend = 0;			//Clear output buffer
		OutputBuffer[dwNumBytesToSend++] = '\xAB';	//Send BAD command ��0xAB��
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
		dwNumBytesToSend = 0;			//Clear output buffer
		do {
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	//Get the number of bytes in the device input buffer
		} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   //or Timeout
		bCommandEchod = false;
		ftStatus = FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  //Read out the data from input buffer
		for (dwCount = 0;dwCount < dwNumBytesRead - 1; dwCount++)	//Check if Bad command and echo command received
		{
			if ((InputBuffer[dwCount] == 0xfa) && (InputBuffer[dwCount + 1] == 0xab))
			{
				bCommandEchod = true;
				break;
			}
		}
		if (bCommandEchod == false)
		{
            fprintf(stderr, "fail to synchronize MPSSE with command '0xAB' \n");
            //getchar();
			return false;
			/*Error, can��t receive echo command , fail to synchronize MPSSE interface;*/
		}

        //		fprintf(stderr, "MPSSE synchronized with BAD command \n");

				////////////////////////////////////////////////////////////////////
				//Configure the MPSSE for rvlink(test io) communication with pep(two signals for test io)
				//////////////////////////////////////////////////////////////////
		OutputBuffer[dwNumBytesToSend++] = '\x8A'; 	//Ensure disable clock divide by 5 for 60Mhz master clock
		OutputBuffer[dwNumBytesToSend++] = '\x97';	 //Ensure turn off adaptive clocking
		OutputBuffer[dwNumBytesToSend++] = '\x8D'; 	//Enable 3 phase data clock, used by I2C to allow data on both clock edges
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the commands
		dwNumBytesToSend = 0;			//Clear output buffer
		OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output 
		OutputBuffer[dwNumBytesToSend++] = '\x73'; 	//Set data, clk high, WP disabled by SK, DO at bit ��1��, GPIOL0 at bit ��0��
		OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��
		// The SK clock frequency can be worked out by below algorithm with divide by 5 set as off
		// SK frequency  = 60MHz /((1 +  [(1 +0xValueH*256) OR 0xValueL])*2)
		OutputBuffer[dwNumBytesToSend++] = '\x86'; 			//Command to set clock divisor
		OutputBuffer[dwNumBytesToSend++] = dwClockDivisor & '\xFF';	//Set 0xValueL of clock divisor
		OutputBuffer[dwNumBytesToSend++] = (dwClockDivisor >> 8) & '\xFF';	//Set 0xValueH of clock divisor
        fprintf(stderr, "Successfully set  connect speed:%0dKHz\n", speed);//printf speed config information

		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the commands
		dwNumBytesToSend = 0;			//Clear output buffer
		usleep(20 * 1000);		//Delay for a while

		//Turn off loop back in case
		OutputBuffer[dwNumBytesToSend++] = LOOP_BACK_DISA;		//LOOP_BACK_DISA Command to turn off loop back of TDI/TDO connection, LOOP_BACK_ENA to turn on 
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the commands
		dwNumBytesToSend = 0;			//Clear output buffer
		usleep(30 * 1000);		//Delay for a while
	}
	//	FT_Close(ftHandle);
	return true;
}

BOOL WriteDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG dwData)
{
	FT_STATUS ftStatus = FT_OK;
	ULONGLONG dwDataTemp;
	DWORD dwCount;
	BYTE dwGetAck = 0xff;
	rcvdat rcvdata = { 0xff, 0xff, 0xff };
	BYTE dwDataSend[16] = { 0 };
	BYTE Parity = 1;
	BYTE Array = 0;

	dwDataSend[0] = 0xfc | (dwTestioStart << 1) | dwTesioWriteFlag;
	//dwAddr
	dwDataSend[1] = 0xff & (dwAddr >> 32);
	dwDataSend[2] = 0xff & (dwAddr >> 24);
	dwDataSend[3] = 0xff & (dwAddr >> 16);
	dwDataSend[4] = 0xff & (dwAddr >> 8);
	dwDataSend[5] = 0xff & (dwAddr >> 0);
    //fprintf(stderr, "dwDataSend[5]=0x%x\n",dwDataSend[5]);
	dwDataSend[6] = STRB;
	//dwData
	dwDataSend[7] = 0xff & (dwData >> 56);
	dwDataSend[8] = 0xff & (dwData >> 48);
	dwDataSend[9] = 0xff & (dwData >> 40);
	dwDataSend[10] = 0xff & (dwData >> 32);
	dwDataSend[11] = 0xff & (dwData >> 24);
	dwDataSend[12] = 0xff & (dwData >> 16);
	dwDataSend[13] = 0xff & (dwData >> 8);
	dwDataSend[14] = 0xff & (dwData >> 0);
	//Calculate Parity
	Array = sizeof(dwDataSend);
	Parity = soc_gen_even_parity_common(&dwDataSend[1], (Array - 2) * 8);
	Parity = (Parity + dwTesioWriteFlag) % 2;
    //	fprintf(stderr, "Write Parity=0x%x\n",Parity);
		//Parity and dwTestioStop
	dwDataSend[15] = 0x3f | (Parity << 7) | (dwTestioStop << 6);
    //	fprintf(stderr, "dwDataSend[15]=0x%x\n",dwDataSend[15]);

	dwNumBytesToSend = 0;

	for (dwCount = 0; dwCount < 4; dwCount++)	// Repeat commands to ensure the minimum period of the dwTestioStop setup time ie 600ns is achieved
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
		OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set dataout high, clk low, set GPIOL0-2 to 1(high)
		OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��
	}

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 	//clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = '\x0e';  //14+1=15 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	//Data length of 0x0000 means 1 byte data to clock out

	for (dwCount = 0; dwCount < 15; dwCount++)	// Repeat commands to pass dwDataSend to OutputBufer
	{
		OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount];

	}
	//output Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = '\x01';  //2 bits
	OutputBuffer[dwNumBytesToSend++] = dwDataSend[15];	//Data length of 0x0000 means 1 byte data to clock out
	//Set dataout input?
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set data high,clk low, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf1';	//Set CLK,GPIOL0-3 pins as output with bit ��1��, data and other pins as input with bit ��0��

	//Get Ack data from chip,clock out with byte in
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_IN; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
	if ((dwAddr >= STATION_SDIO_MTP_TIME_CONFIG_ADDR__DEPTH_0) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
	{
		OutputBuffer[dwNumBytesToSend++] = waitMtpEraseByte & 0xff;  //n bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitMtpEraseByte >> 8) & 0xff;
        //		fprintf(stderr, "waitMtpEraseByte=0x%x,%d\n",waitMtpEraseByte,waitMtpEraseByte);
	}
	else if ((dwAddr >= STATION_SDIO_MTP_ADDR) && (dwAddr < STATION_SDIO_MTP_TIME_CONFIG_ADDR__DEPTH_0))
	{
		OutputBuffer[dwNumBytesToSend++] = waitMtpProgByte & 0xff;  //n bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitMtpProgByte >> 8) & 0xff;  //n bytes to clock out
//		fprintf(stderr, "waitMtpProgByte=0x%x,%d\n",waitMtpProgByte,waitMtpProgByte);
	}
	else
	{
		OutputBuffer[dwNumBytesToSend++] = waitByte & 0xff;  //n bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitByte >> 8) & 0xff;  //n bytes to clock out
//		fprintf(stderr, "waitByte=0x%x,%d\n",waitByte,waitByte);
	}
	//	OutputBuffer[dwNumBytesToSend++] = 0xff;	//Data length of 0x0000 means 1 byte data to clock out, no need data here for sample in
	//	OutputBuffer[dwNumBytesToSend++] = 0xff;

		//flush chip buffer to pc
	OutputBuffer[dwNumBytesToSend++] = '\x87';	//Send answer back immediate command

	//Set bus to idle, set clk and data high
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x73'; 	//Set data,clk high, set GPIOL0-3 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);		//Send off the commands

	dwNumBytesToSend = 0;			//Clear output buffer
//  if((dwAddr >= STATION_SDIO_MTP_TIME_CONFIG_ADDR__DEPTH_0) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
// 	{
// 		usleep(300 * 1000); //calc 13bytes * 8bit /  1 =  104ms, then 200*1000us >> 104ms for stable data. 
// 	}
// 	else if((dwAddr >= STATION_SDIO_BASE_ADDR) && (dwAddr < (STATION_SDIO_BASE_ADDR + 8*8*1024)))
// 	{
// 		usleep(waitDataStableTime * waitlevel * 1000);
// 	}
// 	else
// 	{
// 		usleep(waitDataStableTime * 1000);
// //		fprintf(stderr, "do set waitStableTime=%3.2f\n",waitStableTime);
// //		fprintf(stderr, "do set waitStableTh=%3.2f\n",waitStableTh);
// 	}	
	//usleep(100);
	//Check if ACK bit received, may need to read more times to get ACK bit or fail if timeout
	dwCount = 0;
	while (dwGetAck)
	{
		dwNumInputBuffer = 0;
		do
		{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
			timeoutcounter++;
			if (timeoutcounter == timeoutvalue)
			{
				timeoutcounter = 0;
				break;
			}
            //			fprintf(stderr, "timeoutcounter=%d\n",timeoutcounter);
		} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));

        // fprintf(stderr, "Write Read ACK dwNumInputBuffer=%d\n",dwNumInputBuffer);

		ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  	//Read dwNumInputBuffer bytes from device receive buffer

		// static time_t ltime;
		// ltime = time(NULL);
        // fprintf(stderr, "Start InputDataHandle @ %s\n", asctime(localtime(&ltime)));

		InputDataHandle(&dwDataTemp, &rcvdata, InputBuffer, dwNumBytesRead, dwTesioWriteFlag);                   //Handle received data
		dwGetAck = rcvdata.ack;

		// ltime = time(NULL);
        // fprintf(stderr, "Done InputDataHandle @ %s\n", asctime(localtime(&ltime)));

		if ((ftStatus != FT_OK) || (dwCount == rwcycle))
		{
            fprintf(stderr, "error code 1:fail to get ACK when write byte!---0x%x\n", dwGetAck);
            fprintf(stderr, "error addr  :0x%llx error data:0x%llx\n", dwAddr, dwData);
			return FALSE; //Error, can't get the ACK bit from debug port
		}
		else
		{
			// dwGetAck = (InputBuffer[0] << 8) | InputBuffer[1];
            // fprintf(stderr, "Do Get WACK:InputBuffer[0]=0x%x,InputBuffer[1]=0x%x\n",InputBuffer[0],InputBuffer[1]);
            // fprintf(stderr, "Do Get WACK:dwGetAck=0x%x\n",dwGetAck);
			if (dwGetAck != dwTestioAck)	//Check ACK bit 0 on data byte read out
			{
				//timeout counter
				dwCount++;
			}
			else
			{
				break;
			}
		}
	}

	return TRUE;
}

BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG* dwGetData)
{
	FT_STATUS ftStatus = FT_OK;
	ULONGLONG dwData = 0;
	DWORD dwCount;
	//DWORD dwDataBitCount;
	BYTE dwGetAck = 0xff;
	rcvdat rcvdata = { 0xff, 0xff, 0xff };
	BYTE dwDataSend[7] = { 0 };
	BYTE Parity = 1;
	BYTE Array = 0;
	BYTE dwDataTemp[8] = { 0 };

	dwDataSend[0] = 0xfc | (dwTestioStart << 1) | dwTesioReadFlag;
	//dwAddr
	dwDataSend[1] = 0xff & (dwAddr >> 32);
	dwDataSend[2] = 0xff & (dwAddr >> 24);
	dwDataSend[3] = 0xff & (dwAddr >> 16);
	dwDataSend[4] = 0xff & (dwAddr >> 8);
	dwDataSend[5] = 0xff & (dwAddr >> 0);
    //fprintf(stderr, "dwDataSend[5]=0x%x\n",dwDataSend[5]);

	//Calculate Parity
	Array = sizeof(dwDataSend);
	Parity = soc_gen_even_parity_common(&dwDataSend[1], (Array - 2) * 8);
	Parity = (Parity + dwTesioReadFlag) % 2;
    //	fprintf(stderr, "Read Addr Parity=0x%x\n",Parity);
		//Parity and dwTestioStop
	dwDataSend[6] = 0x3f | (Parity << 7) | (dwTestioStop << 6);

	/////////////////////////
	dwNumBytesToSend = 0;

	for (dwCount = 0; dwCount < 4; dwCount++)	// Repeat commands to ensure the minimum period of the dwTestioStop setup time ie 600ns is achieved
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
		OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set dataout high, clk low, set GPIOL0-2 to 1(high)
		OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��
	}

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 	//clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = '\x05';  //5+1=6 bytes to clk out except dwDataSend[6] and stop
	OutputBuffer[dwNumBytesToSend++] = '\x00';	//Data length of 0x0000 means 1 byte data to clock out

	for (dwCount = 0; dwCount < 6; dwCount++)	// Repeat commands to pass dwDataSend to OutputBufer [0]-[5]
	{
		OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount];

	}
	//output Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop [6]
	OutputBuffer[dwNumBytesToSend++] = '\x01';  //1+1 = 2 bits
	OutputBuffer[dwNumBytesToSend++] = dwDataSend[6];	//Data length of 0x0000 means 1 byte data to clock out
	//Set dataout input?
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set data,clk high, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf1';	//Set CLK,GPIOL0-3 pins as output with bit ��1��, data and other pins as input with bit ��0��

	//Get Ack data from chip,clock out with byte in
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_IN; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
	if ((dwAddr >= STATION_SDIO_MTP_ADDR) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
	{
		OutputBuffer[dwNumBytesToSend++] = waitMtpProgByte & 0xff;  //num+1 bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitMtpProgByte >> 8) & 0xff;
	}
	else
	{
		OutputBuffer[dwNumBytesToSend++] = waitByte & 0xff;  //num+1 bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitByte >> 8) & 0xff;
	}
    //	fprintf(stderr, "readAckWaitByte(waitTime,waitfreq)=0x%x,%d\n",AckWaitByte(waitTime,waitfreq),AckWaitByte(waitTime,waitfreq));

		//flush chip buffer to pc
	OutputBuffer[dwNumBytesToSend++] = '\x87';	//Send answer back immediate command

	//Set bus to idle, set clk and data high
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x73'; 	//Set data,clk high, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0-3 pins as output with bit ��1��, other pins as input with bit ��0��

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);		//Send off the commands

	//usleep(waitDataStableTime * waitlevel * 1000);  //calc 13bytes * 8bit / 50 = 2.08ms, then 20 *1000us >> 2.08ms for stable data. 
    if((dwAddr >= STATION_SDIO_MTP_ADDR) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
    {
        usleep(waitlevel);
    }
    usleep(waitDataStableTime);
    //fprintf(stderr, "waitlevel=%dus, waitDataStableTime=%.fus\n", waitlevel, waitDataStableTime);
	dwNumBytesToSend = 0;			//Clear output buffer
	//Check if ACK bit received, may need to read more times to get ACK bit or fail if timeout

	dwNumBytesToSend = 0;			//Clear output buffer
	do {
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	//Get the number of bytes in the device input buffer
	} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   //or Timeout

	dwCount = 0;
	while (dwGetAck)
	{
		dwNumInputBuffer = 0;
		do
		{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
			timeoutcounter++;
			if (timeoutcounter == timeoutvalue)
			{
				timeoutcounter = 0;
				break;
			}
            //			fprintf(stderr, "timeoutcounter=%d\n",timeoutcounter);
		} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));
        //fprintf(stderr, "timeoutcounter=%d\n", timeoutcounter);
        //fprintf(stderr, "dwNumInputBuffer=%d\n", dwNumInputBuffer);
        //fprintf(stderr, "ftStatus=%d\n", ftStatus);
        // fprintf(stderr, "Read dwNumInputBuffer=%d\n",dwNumInputBuffer);
		ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  	//Read dwNumInputBuffer bytes from device receive buffer
		////////////////////////////////////////////////////////////
		InputDataHandle(&dwData, &rcvdata, InputBuffer, dwNumBytesRead, dwTesioReadFlag);
		dwGetAck = rcvdata.ack;

        //  fprintf(stderr, "Read dwGetAck=%d\n",dwGetAck);
        //	fprintf(stderr, "Do Get dwNumBytesRead:dwData=%d\n",dwNumBytesRead);
		if ((ftStatus != FT_OK) || (dwCount == rwcycle))
		{
            fprintf(stderr, "ftStatus @ ftStatus 0x%x \n", ftStatus);
            fprintf(stderr, "rwcycle  @ rwcycle  %d \n", rwcycle);
            fprintf(stderr, "error code 1:fail to get ACK when read byte! @ address 0x%llx \n", dwAddr);
            fprintf(stderr, "\n");
			return FALSE; //Error, can't get the ACK bit from EEPROM
            //getchar();
		}
		else
		{
			//      dwGetAck = (InputBuffer[0] << 8) | InputBuffer[1];
			// 		for(dwCount=0; dwCount < 10 ; dwCount ++)
			// 		{
            // //		fprintf(stderr, "InputBuffer[%d]=0x%x\n",dwCount,InputBuffer[dwCount]);
			// 			if(dwCount < 8)
			// 			{
			//                 dwData = (dwData << 8 | InputBuffer[dwCount+1]);
            // //				fprintf(stderr, "dwData[%d]=0x%llx\n",dwCount,dwData);
			// 			}
			// 		}
            //		fprintf(stderr, "do read dwData=0x%llx\n",dwData);
            //		fprintf(stderr, "Do Get ReadWACK:InputBuffer[0]=0x%x,InputBuffer[1]=0x%x\n",InputBuffer[0],InputBuffer[1]);
            //		fprintf(stderr, "Do Get ReadWACK:dwGetAck=0x%x\n",dwGetAck);
            //		fprintf(stderr, "Do Get ReadData:dwData=0x%llx\n",dwData);
			if (dwGetAck != dwTestioAck)	//Check ACK bit 0 on data byte read out
			{
				//timeout counter
				dwCount++;
			}
			else
			{
				RotateLeft(&InputBuffer[rcvdata.dwSeekCount], dwNumBytesRead - rcvdata.dwSeekCount, rcvdata.dwDataBitCount + 2);//dwSeekCount bytes, remove bits start && ack bit
				dwData = 0;
				for (dwCount = 0; dwCount < 8; dwCount++)
				{
                    //		fprintf(stderr, "dwdatatemp[%d]=0x%llx\n",dwcount,dwdatatemp[dwcount]);
					dwData = (dwData << 8 | InputBuffer[rcvdata.dwSeekCount + dwCount]);
				}
				rcvdata.parity = (InputBuffer[rcvdata.dwSeekCount + dwCount] >> 7);//Calculate Read Data Parity
				//dwData = 0x128c6500000000;
				for (BYTE i = 0; i < 8; i++)
				{
					dwDataTemp[i] = 0xff & (dwData >> ((7 - i) * 8));
                    //			fprintf(stderr, "Do Get dwDataTemp:dwDataTemp[%d]=0x%llx\n",i,dwDataTemp[i]);
				}
				Array = sizeof(dwDataTemp);
				Parity = ((soc_gen_even_parity_common(dwDataTemp, Array * 8)) + rcvdata.ack) % 2;
                //fprintf(stderr, "Do Get ReadData:dwData=0x%llx\n",dwData);
                //fprintf(stderr, "calc parity = 0x%x, rcvdata.parity = 0x%x, rcvdata.ack = 0x%x, addr=0x%llx, dwData=0x%llx\n", Parity, rcvdata.parity, rcvdata.ack, dwAddr, dwData);
				if (Parity != rcvdata.parity)
				{
                    fprintf(stderr, "error code 2:wrong parity = 0x%x, should be parity = 0x%x, rcvdata.ack = 0x%x, addr=0x%llx, dwData=0x%llx\n", Parity, rcvdata.parity, rcvdata.ack, dwAddr, dwData);
                    //			//getchar();
				}
				break;
			}
		}
	}
	*dwGetData = dwData;
	//return dwData;
	if ((rcvdata.start || rcvdata.ack || (Parity != rcvdata.parity)))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BYTE ft_listdevice(void)
{
	DWORD numDevs;
	FT_DEVICE_LIST_INFO_NODE* devInfo;

	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus == FT_OK)
	{
        fprintf(stderr, "Number of devices is %d\n", numDevs);
	}
	else
	{
        fprintf(stderr, "FT_CreateDeviceInfoList failed \n");// FT_CreateDeviceInfoList failed
	}

	if (numDevs > 0)
	{
		// allocate storage for list based on numDevs 
		devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevs);
		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
		if (ftStatus == FT_OK)
		{
            for (DWORD i = 0; i < numDevs; i++)
			{
                fprintf(stderr, "Dev %d:\n", i);
                fprintf(stderr, " Flags=0x%x\n", devInfo[i].Flags);
                fprintf(stderr, " Type=0x%x\n", devInfo[i].Type);
                fprintf(stderr, " ID=0x%x\n", devInfo[i].ID);
                fprintf(stderr, " LocId=0x%x\n", devInfo[i].LocId);
                fprintf(stderr, " SerialNumber=%s\n", devInfo[i].SerialNumber);
                fprintf(stderr, " Description=%s\n", devInfo[i].Description);
                fprintf(stderr, " ftHandle=0x%x\n", devInfo[i].ftHandle);
			}
		}
	}
	return true;
}

void setup(DWORD speed)
{
    ft_device_init(speed);
}

void set_reset(void)
{
	dwNumBytesToSend = 0;

	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x03'; 	//Set gpio low
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��



	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
    //	fprintf(stderr, "dwNumBytesSent=%d\n",dwNumBytesSent);
	dwNumBytesToSend = 0;
}

void release_reset(void)
{
	dwNumBytesToSend = 0;

	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x73'; 	//Set GPIO high
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
	dwNumBytesToSend = 0;
}

ULONGLONG do_read(ULONGLONG addr)
{
	ULONGLONG data = 0;
	BOOL dwDataStatus = TRUE;
	BYTE dwCount = 3;
	//struct timeval tvpre, tvafter;
	//gettimeofday(&tvpre, NULL);
	do
	{
		dwDataStatus = ReadDataAndCheckACK(addr, &data);
		dwCount--;
		if (dwCount == 0)
		{
			break;
		}
	} while (dwDataStatus == FALSE);

	//gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
	return data;
}

BOOL rv_read(ULONGLONG addr, ULONGLONG *data)
{
    ULONGLONG rdata = 0;
    BOOL rvStatus = TRUE;
    BYTE dwCount = 3;
    //struct timeval tvpre, tvafter;
    //gettimeofday(&tvpre, NULL);
    do
    {
        rvStatus = ReadDataAndCheckACK(addr, &rdata);
        dwCount--;
        if (dwCount == 0)
        {
            break;
        }
    } while (rvStatus == FALSE);
    *data = rdata;

    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    return rvStatus;
}

void do_write(ULONGLONG addr, ULONGLONG data)
{
	//struct timeval tvpre,tvafter;
	//gettimeofday(&tvpre, NULL);

	WriteDataAndCheckACK(addr, data);

	//gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //	//getchar();
}

BOOL rv_write(ULONGLONG addr, ULONGLONG data)
{
    BOOL rvStatus;
    //struct timeval tvpre,tvafter;
    //gettimeofday(&tvpre, NULL);

    rvStatus = WriteDataAndCheckACK(addr, data);

    //gettimeofday(&tvafter, NULL);
    //fprintf(stderr, "Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //	//getchar();
    return rvStatus;
}

void do_write_burst(ULONGLONG addr, ULONGLONG burst_data, DWORD burst_idx)
{
	burst_idx = burst_idx;
	WriteDataAndCheckACK(addr, burst_data);
}

void set_gpiol3(BYTE gpiol3)
{
	dwNumBytesToSend = 0;

	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\xf3' & ((gpiol3 << 7) | 0x7f); 	//Set gpio high
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
    fprintf(stderr, "set gpiol3=%d\n", gpiol3);
	dwNumBytesToSend = 0;
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
//        rvStatus |= rv_read(STATION_DMA_MTIME_EN_ADDR, &value);
//        //fprintf(stderr, "mtimeen = 0x%llx \n", value);
//        rvStatus |= rv_write(STATION_DMA_MTIME_EN_ADDR, 0);
//        rvStatus |= rv_read(STATION_DMA_MTIMECMP_VP_0_ADDR, &mtimecmp_v);
//        //fprintf(stderr, "mtimecmp = 0x%llx \n", mtimecmp_v);
//        rvStatus |= rv_read(STATION_DMA_MTIME_ADDR, &mtime_v);
//        //fprintf(stderr, "mtime = 0x%llx \n", mtime_v);
//        //rvStatus = rv_write(STATION_DMA_MTIME_ADDR, mtimecmp_v - 1);
//        rvStatus |= rv_write(STATION_DMA_MTIMECMP_VP_0_ADDR, mtime_v+1);

//        rvStatus |= rv_read(STATION_ORV32_MIE_ADDR, &mie_backup);
//        //fprintf(stderr, "mie = 0x%llx \n", mie_backup);
//        value = mie_backup &(~0x80); //disable mtie
//        //fprintf(stderr, "value = 0x%llx \n", value);
//        rvStatus |= rv_write(STATION_ORV32_MIE_ADDR, value);
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

BOOL ft_open(void)
{
    FT_STATUS ftStatus = FT_OK;
    ftStatus = FT_Open(0,&ftHandle);
    if(FT_OK == ftStatus)
    {
        return true;
    }
    else
    {
        return false;
    }
}

BOOL ft_close(void)
{
    FT_STATUS ftStatus = FT_OK;
    ftStatus = FT_Close(ftHandle);
    if(FT_OK == ftStatus)
    {
        return true;
    }
    else
    {
        return false;
    }
}
