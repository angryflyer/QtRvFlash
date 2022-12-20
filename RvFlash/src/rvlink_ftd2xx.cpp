#include "rvlink_ftd2xx.h"
#include "flashcfg.h"
#include "string.h"
#include <stdlib.h>
#include <math.h>
//============================================================================
//  Use of FTDI D2XX library:
//----------------------------------------------------------------------------
//  Include the following 2 lines in your header-file
//#pragma comment(lib, "FTD2XX.lib")
#include "ftd2xx.h"
//============================================================================
	const BYTE AA_ECHO_CMD_1 = '\xAA';
	const BYTE AB_ECHO_CMD_2 = '\xAB';
	const BYTE BAD_COMMAND_RESPONSE = '\xFA';
	const BYTE LOOP_BACK_ENA = '\x84';  //enable loop-back , internel connect TDI to TDO
    const BYTE LOOP_BACK_DISA = '\x85'; //disable loop-back

	const BYTE MSB_VEDGE_CLOCK_IN_BIT = '\x22';
	const BYTE MSB_EDGE_CLOCK_OUT_BYTE = '\x11';
	const BYTE MSB_EDGE_CLOCK_IN_BYTE = '\x24';

	const BYTE MSB_RISING_EDGE_CLOCK_BYTE_IN = '\x20';
	const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_IN = '\x24';
	const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_OUT = '\x11';
	const BYTE MSB_DOWN_EDGE_CLOCK_BIT_IN = '\x26';
	const BYTE MSB_UP_EDGE_CLOCK_BYTE_IN = '\x20';
	const BYTE MSB_UP_EDGE_CLOCK_BYTE_OUT = '\x10';
	const BYTE MSB_RISING_EDGE_CLOCK_BIT_IN = '\x22';
    const BYTE MSB_RISING_EDGE_CLOCK_BIT_OUT = '\x12';
    const BYTE MSB_FALLING_EDGE_CLOCK_BIT_OUT = '\x13';

//  extern parameters
	DWORD waitfreq = 2000;//KHz
	DWORD waitlevel = 1;
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
//  #define waitStableTh               (waitStableTime)
    #define waitStableTh               (UCHAR) 0
//	#define waitDataStableTime         ((waitStableTime > waitStableTh) ? waitStableTime : waitStableTh)
    #define waitDataStableTime         (waitStableTh)

	#define waitMtpEraseByte           (AckWaitByte(waitMtpEraseTime,waitfreq) > waitByteTh ? AckWaitByte(waitMtpEraseTime,waitfreq) : waitByteTh)
	#define waitMtpProgByte            (AckWaitByte(waitMtpProgTime,waitfreq) > waitByteTh ? AckWaitByte(waitMtpProgTime,waitfreq) : waitByteTh)
	#define waitByte                   (AckWaitByte(waitTime,waitfreq) > waitByteTh ? AckWaitByte(waitTime,waitfreq) : waitByteTh)
//    #define rwcycle                    (DWORD) 10
//    #define timeoutvalue               (DWORD) 100
    #define rwcycle                    (DWORD) 1000
    #define timeoutvalue               (DWORD) 300
//	#define Parity         1

/* 从data中获取第n bit的值 注：data只能为uint8*类型指针 */
    #define GET_BIT_N_VAL(data, n)  \
            (0x1 & (( *((data) + (n)/8) & (0x1 << ((n)%8)) ) >> ((n)%8)))


	FT_STATUS ftStatus;			//Status defined in D2XX to indicate operation result
	FT_HANDLE ftHandle;			//Handle of FTD2XX device port 
	BYTE OutputBuffer[1024];		//Buffer to hold MPSSE commands and data to be sent to FTD2XX
	BYTE InputBuffer[65536];		//Buffer to hold Data bytes to be read from FTD2XX
    BYTE dwDataTemp[65536] = {0};
    BYTE dwDataRotate[65536] = {0};
	DWORD dwClockDivisor = 0x001B;  //1000KHz
	//DWORD dwClockDivisor = 0x0037;  //800KHz
	//DWORD dwClockDivisor = 0x004A;  //Value of clock divisor, SCL Frequency = 60/((1+0x004A)*2) (MHz) = 400khz
	//DWORD dwClockDivisor = 0x0095;  //200khz
	DWORD dwNumBytesToSend = 0; 	//Index of output buffer
	DWORD dwNumBytesSent = 0, 	dwNumBytesRead = 0, dwNumInputBuffer = 0;

    BYTE dwAddr_Array[5] = {0};
	BYTE dwData_Array[8] = {0};
	BYTE STRB = 0xff;
    DWORD timeoutcounter = 0;

	BYTE ByteDataRead;//ByteAddress
	BYTE ByteAddresshigh = 0x00, ByteAddresslow = 0x80;		//EEPROM address is '0x0080'
	BYTE ByteDataToBeSend = 0x5A;							//data programmed and read
//////////////////////////////////////////////////////////////////    
	BOOL ft_dev_init(DWORD speed);
	BYTE soc_gen_even_parity_common(BYTE *entry_data, WORD entry_len);
    BYTE soc_gen_odd_parity_common(BYTE *entry_data, WORD entry_len);
    BOOL WriteDataAndCheckACK(ULONGLONG dwAddr,ULONGLONG dwData, BYTE dwAddrBitW, BYTE dwDataBitW);
//    BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG *dwGetData, BYTE dwAddrBitW, BYTE dwDataBitW, BYTE *ptBuffer, ULONGLONG *ptNum);
    BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG *dwGetData, BYTE dwAddrBitW, BYTE dwDataBitW);
    BYTE RotateLeft(BYTE *InputData , DWORD Lenth , DWORD ShiftLenth);
    BYTE InputDataHandle(ULONGLONG *dwDataHandled, rcvdat *dwAckData, BYTE *InputData, DWORD InputDataLenth, BYTE rwType);
    BYTE ft_listdevice(void);
//////////////////////////////////////////////////////////////////
BYTE soc_gen_even_parity_common(BYTE *entry_data, WORD entry_len)
{
    DWORD i = 0;
    DWORD even_parity = 0;
    for(i = 0; i < entry_len; i++)                  
    {                                                                              
        even_parity += GET_BIT_N_VAL((entry_data), i);                 
    }

    return (even_parity & 0x1);
}

BYTE soc_gen_odd_parity_common(BYTE *entry_data, WORD entry_len)
{
    DWORD i = 0;
    DWORD odd_parity = 0;

    for(i = 0; i < entry_len; i++)                  
    {                                                                              
        odd_parity += odd_parity+ GET_BIT_N_VAL((entry_data), i);                 
    }

    if (odd_parity % 2 == 0)
        return 1;
    else
        return 0;
}

BYTE RotateLeft(BYTE *InputData , DWORD Lenth , DWORD ShiftLenth)
{
    BYTE valBit;
    DWORD LenthCount;
    BYTE *dwDataTemp = InputData;

    if(!Lenth) return false;

    while(ShiftLenth--)
    {
        InputData = dwDataTemp;
//		printf("ShiftLenth=%d\n",ShiftLenth);
        LenthCount = Lenth;
        LenthCount --;
//	    valBit = *InputData >> 7;
        do
        {
            *InputData = (*InputData << 1) | (*(InputData + 1) >> 7);
            InputData ++;
        }
        while(LenthCount--);
//	        *InputData = (*InputData << 1) | valBit;
        *InputData = (*InputData << 1) | (*(InputData + 1) >> 7);
    }
    return true;
}

BYTE InputDataHandle(ULONGLONG *dwDataHandled, rcvdat *dwAckData, BYTE *InputData, DWORD InputDataLenth, BYTE rwType)
{
	DWORD dwSeekCount, dwDataBitCount;
	DWORD dwCount;
//	BYTE dwDataTemp[InputDataLenth];
	BYTE dwDataShiftStart;
	BYTE dwDataShiftAck0;
	BYTE dwDataShiftAck1;
	*dwDataHandled = 0;
	////
	memcpy(dwDataTemp,InputData,InputDataLenth);
//     for(dwCount=0; dwCount < InputDataLenth; dwCount ++)
//     {
//        printf("dwDataTemp[%d]=0x%llx\n",dwCount,dwDataTemp[dwCount]);
//     }
	for(dwSeekCount = 0; dwSeekCount < InputDataLenth; dwSeekCount ++)
	{
		for(dwDataBitCount = 0; dwDataBitCount < 8; dwDataBitCount++)
		{
//			printf("InputData[dwSeekCount%d] << dwDataBitCount%d=0x%x\n",dwSeekCount,dwDataBitCount,0xc0 & InputData[dwSeekCount] << dwDataBitCount);
			dwDataShiftStart = (0x80 & InputData[dwSeekCount] << dwDataBitCount);
			dwDataShiftAck0  = (0x80 & InputData[dwSeekCount] << (dwDataBitCount + 1));
			dwDataShiftAck1  = InputData[dwSeekCount + 1] >> dwDataBitCount;
			if(0x00 == dwDataShiftStart)  //seek start and compare start and read addr ack,record the dwSeekCount and dwDataBitCount
			{	
				dwAckData->start = 0;
				if(dwDataBitCount == 7) //represent ack ok,check next array data
				{
					dwAckData->ack = (dwDataShiftAck1 == 0x00) ? 0 : 1;		
				}
				else
				{
					dwAckData->ack = (dwDataShiftAck0 == 0x00) ? 0 : 1;
				}
				// printf("dwSeekCount=%d\ndwDataBitCount=%d\n", dwSeekCount, dwDataBitCount);
				// printf("InputData[dwSeekCount]=%x\nInputData[dwSeekCount+1]=%x\n", InputData[dwSeekCount], InputData[dwSeekCount+1]);
				// printf("dwAckData->ack=%x\n", dwAckData->ack);
				break; //find start and break loop
			}
		}
		if ((dwSeekCount == (InputDataLenth - 1)) && (dwDataBitCount == 8))
		{
			*dwDataHandled = 0;
//			printf("error: do read incorrect ack\n");
			dwAckData->start = 1;
		}
		if(dwAckData->start == 0) break;
	}

//	printf("dwSeekCount=%d\ndwDataBitCount=%d\n", dwSeekCount, dwDataBitCount);
//  printf("dwSeekCount=%d\ndwDataBitCount=%d\ndwDataTemp[dwSeekCount]=0x%x\n \
    InputDataLenth-dwSeekCount=%d\ndwDataBitCount + 2=%d\n", dwSeekCount,     \
	dwDataBitCount,dwDataTemp[dwSeekCount],InputDataLenth-dwSeekCount,dwDataBitCount + 2);

//	RotateLeft(dwDataTemp,InputDataLenth,(dwSeekCount * 8 + dwDataBitCount + 2));//dwSeekCount bytes, remove bits start && ack bit
	RotateLeft(&dwDataTemp[dwSeekCount],InputDataLenth-dwSeekCount,dwDataBitCount + 2);//dwSeekCount bytes, remove bits start && ack bit
    if((rwType & 0x01) == dwTesioReadFlag)
	{
		for(dwCount=0; dwCount < 8; dwCount ++)
		{	
//                printf("dwDataTemp[%d]=0x%llx\n",dwSeekCount+dwCount,dwDataTemp[dwSeekCount+dwCount]);
                *dwDataHandled = (*dwDataHandled << 8 | dwDataTemp[dwSeekCount+dwCount]);
		}
        dwAckData->parity = (dwDataTemp[dwSeekCount + (rwType >> 1)] >> 7);
	}
	else
	{
		dwAckData->parity = (dwDataTemp[dwSeekCount] >> 7);
	}
//     printf("dwAckData->parity=%x\n", dwAckData->parity);
//     printf("dwDataHandled=0x%llx\n",*dwDataHandled);
	return true;
}

BOOL ft_dev_init(DWORD speed)
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

	dwClockDivisor = 30*1000 / speed - 1; //Value of clock divisor = (30 / freq -1) (30 & freq unit:MHZ)
//list device information!
    if(ft_listdevices == true)
	{
        ft_listdevice();
	}
//return information of port marked devIndex,such as FT4232HL's devIndex(has 4 ports):0,1,2,3
	ftStatus = FT_ListDevices((PVOID)devIndex, &Buf, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
	ftStatus = FT_OpenEx((PVOID)Buf, FT_OPEN_BY_DESCRIPTION, &ftHandle);
//    printf("devIndex:%d\n",devIndex);

	if (ftStatus != FT_OK)
    {
		printf("Can't open FTD2XX device! \n");
		getchar();
		return false;
	}
	else
    {      // Port opened successfully
        printf("Successfully get  channel A description:%s\n",Buf);
		printf("Successfully open channel A of FTD2XX device! \n");

		ftStatus |= FT_ResetDevice(ftHandle); 	//Reset USB device
		//printf("FT_ResetDevice!\n");
		//Purge USB receive buffer first by reading out all old data from FTD2XX receive buffer
		ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	 // Get the number of bytes in the FTD2XX receive buffer
		//printf("FT_GetQueueStatus_dwNumInputBuffer!=%d\n",dwNumInputBuffer);
		if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
				FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  	//Read out the data from FTD2XX receive buffer
		//printf("FT_Read_after!=%d\n",dwNumInputBuffer);
		ftStatus |= FT_SetUSBParameters(ftHandle, 65536, 65535);	//Set USB request transfer size
		//printf("FT_SetUSBParameters!\n");
		ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);	 //Disable event and error characters
		//printf("FT_SetChars!\n");
		ftStatus |= FT_SetTimeouts(ftHandle, 0, 5000);		//Sets the read and write timeouts in milliseconds for the FTD2XX
                //printf("FT_SetTimeouts!\n");
		ftStatus |= FT_SetLatencyTimer(ftHandle, 16);		//Set the latency timer
		//printf("FT_SetLatencyTimer!\n");
//		ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00); 		//Reset controller
		//printf("FT_SetBitMode_Reset!\n");
		ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x02);	 	//Enable MPSSE mode
                //printf("FT_SetBitMode!\n");
		if (ftStatus != FT_OK)
		{
			printf("fail on initialize FTD2XX device! \n");
			getchar();
			return false;
		}
		usleep(50 * 1000);	// Wait for all the USB stuff to complete and work

//                printf("dwTestioStart Synchronize the MPSSE!\n");		
		//////////////////////////////////////////////////////////////////
		// Synchronize the MPSSE interface by sending bad command ��0xAA��
		//////////////////////////////////////////////////////////////////
		OutputBuffer[dwNumBytesToSend++] = '\xAA';		//Add BAD command ��0xAA��
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
		dwNumBytesToSend = 0;			//Clear output buffer
		do{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	 // Get the number of bytes in the device input buffer
		}while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   	//or Timeout

	        //printf("dwNumInputBuffer=%d,ftStatus=%d\n",dwNumInputBuffer,ftStatus);	

		BOOL bCommandEchod = false;

		//printf("false=%d\n",false);
		//printf("bCommandEchod=%d\n",bCommandEchod);
		
		ftStatus = FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  //Read out the data from input buffer
		
		//printf("InputBuffer=0x%x,0x%x\ndwNumBytesRead=%d\n",InputBuffer[0],InputBuffer[1],dwNumBytesRead);
		
		for (dwCount = 0; dwCount < dwNumBytesRead - 1; dwCount++)	//Check if Bad command and echo command received
		{
			if((InputBuffer[dwCount] == 0xfa) && (InputBuffer[dwCount+1] == 0xaa))
			{
				//printf("If_InputBuffer=0x%x,0x%x\n",InputBuffer[0],InputBuffer[1]);
				bCommandEchod = true;
				break;
			}
		}
		if (bCommandEchod == false) 
		{	
			printf("fail to synchronize MPSSE with command '0xAA' \n");
			getchar();
			return false; /*Error, can��t receive echo command , fail to synchronize MPSSE interface;*/ 
		}
		
		//////////////////////////////////////////////////////////////////
		// Synchronize the MPSSE interface by sending bad command ��0xAB��
		//////////////////////////////////////////////////////////////////
		//dwNumBytesToSend = 0;			//Clear output buffer
		OutputBuffer[dwNumBytesToSend++] = '\xAB';	//Send BAD command ��0xAB��
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
		dwNumBytesToSend = 0;			//Clear output buffer
		do{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	//Get the number of bytes in the device input buffer
		}while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   //or Timeout
		bCommandEchod = false;
		ftStatus = FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  //Read out the data from input buffer
		for (dwCount = 0;dwCount < dwNumBytesRead - 1; dwCount++)	//Check if Bad command and echo command received
		{
			if ((InputBuffer[dwCount] == 0xfa) && (InputBuffer[dwCount+1] == 0xab))
			{
				bCommandEchod = true;
				break;
			}
		}
		if (bCommandEchod == false)  
		{	
			printf("fail to synchronize MPSSE with command '0xAB' \n");
			getchar();
			return false; 
			/*Error, can��t receive echo command , fail to synchronize MPSSE interface;*/ 
		}

//		printf("MPSSE synchronized with BAD command \n");

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
        printf ("Successfully set  connect speed:%0dKHz\n", speed);//printf speed config information

		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the commands
		dwNumBytesToSend = 0;			//Clear output buffer
		usleep(20 * 1000);		//Delay for a while

		//Turn off loop back in case
		OutputBuffer[dwNumBytesToSend++] = LOOP_BACK_DISA;		//LOOP_BACK_DISA Command to turn off loop back of TDI/TDO connection, LOOP_BACK_ENA to turn on 
		ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the commands
		dwNumBytesToSend = 0;			//Clear output buffer
		usleep(30 * 1000);		//Delay for a while
	}
	return true;
}

BOOL WriteDataAndCheckACK(ULONGLONG dwAddr,ULONGLONG dwData,BYTE dwAddrBitW,BYTE dwDataBitW)
{
	FT_STATUS ftStatus = FT_OK;
	ULONGLONG dwDataTemp;
	DWORD dwCount;
	BYTE dwGetAck = 0xff;
	rcvdat rcvdata = {0xff, 0xff, 0xff};
	BYTE dwDataSend[16] = {0};
	BYTE Parity = 1;
	BYTE Array = 0;
	BYTE dwAddrW = dwAddrBitW / 8;
	BYTE dwDataW = dwDataBitW / 8;
	BYTE dwMaskW = (dwDataBitW < 64) ? 1 : dwDataW / 8; 
	BYTE dWSumW  = dwAddrW + dwDataW + dwMaskW;

	dwDataSend[0] = 0xfc | (dwTestioStart << 1) | dwTesioWriteFlag; 
    //dwAddr
	for(dwCount = 1; dwCount <= dwAddrW; dwCount ++)
	{
		dwDataSend[dwCount] = 0xff & (dwAddr >> (dwAddrW - dwCount) * 8);
	}

	for(; dwCount <= dwAddrW + dwMaskW; dwCount ++)
	{
		dwDataSend[dwCount] = STRB;
	}
    //dwData
	for(; dwCount <= dWSumW; dwCount ++)
	{
		dwDataSend[dwCount] = 0xff & (dwData >> (dWSumW - dwCount) * 8);
	}
	//Calculate Parity
    Array = dWSumW;
	Parity = soc_gen_even_parity_common(&dwDataSend[1],Array*8);
	Parity = (Parity + dwTesioWriteFlag)%2;
//	printf("Write Parity=0x%x\n",Parity);
    //Parity and dwTestioStop
	dwDataSend[dwCount] = 0x3f | (Parity << 7) | (dwTestioStop << 6);
//	printf("dwDataSend[15]=0x%x\n",dwDataSend[15]);

	// for debug
	// for(BYTE i = 0; i < 16; i++)
	// {
	// 	printf("dwDataSend[%d]=0x%x\n", i, dwDataSend[i]);
	// }
	dwNumBytesToSend = 0;

	for(dwCount=0; dwCount<1; dwCount++)	// Repeat commands to ensure the minimum period of the dwTestioStop setup time ie 600ns is achieved
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
		OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set dataout high, clk low, set GPIOL0-2 to 1(high)
		OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��
	}
    
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 	//clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = '\x00';  //0+1=1 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	//Data length of 0x0000 means 1 byte data to clock out

	OutputBuffer[dwNumBytesToSend++] = dwDataSend[0];

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; //clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = dwAddrW - 1;                     //x+1 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	                        //Data length of 0x0000 means 1 byte data to clock out

	for(dwCount=1; dwCount<=dwAddrW; dwCount++)	                        // Repeat commands to pass dwDataSend to OutputBufer
	{
		OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount]; 	
		
	}

    //output mask/strb
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = dwDataW - 1;                     //dwDataW bits
	OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount];	            //Data length of 0x0000 means 1 byte data to clock out
	dwCount++;                                                          //++ needed here

	//output data
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; //clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = dwDataW - 1;                     //x+1 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	                        //Data length of 0x0000 means 1 byte data to clock out

	for(; dwCount<=dWSumW; dwCount++)	// Repeat commands to pass dwDataSend to OutputBufer
	{
		OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount]; 			
	}

    //output Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = '\x01';  //2 bits
	OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount];	//Data length of 0x0000 means 1 byte data to clock out
    //Set dataout input?
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set data high,clk low, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf1';	//Set CLK,GPIOL0-3 pins as output with bit ��1��, data and other pins as input with bit ��0��

	//Get Ack data from chip,clock out with byte in
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_IN; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
    if((dwAddr >= STATION_SDIO_MTP_TIME_CONFIG_ADDR__DEPTH_0) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
    {
        OutputBuffer[dwNumBytesToSend++] = waitMtpEraseByte & 0xff;  //n bytes to clock out
        OutputBuffer[dwNumBytesToSend++] = (waitMtpEraseByte >> 8) & 0xff;
 //		printf("waitMtpEraseByte=0x%x,%d\n",waitMtpEraseByte,waitMtpEraseByte);
    }
    else if((dwAddr >= STATION_SDIO_MTP_ADDR) && (dwAddr < STATION_SDIO_MTP_TIME_CONFIG_ADDR__DEPTH_0))
    {
        OutputBuffer[dwNumBytesToSend++] = waitMtpProgByte & 0xff;  //n bytes to clock out
        OutputBuffer[dwNumBytesToSend++] = (waitMtpProgByte >> 8) & 0xff;  //n bytes to clock out
 //		printf("waitMtpProgByte=0x%x,%d\n",waitMtpProgByte,waitMtpProgByte);
    }
    else
	{
		OutputBuffer[dwNumBytesToSend++] = waitByte & 0xff;  //n bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitByte >> 8) & 0xff;  //n bytes to clock out
//		printf("waitByte=0x%x,%d\n",waitByte,waitByte);
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
// //		printf("do set waitStableTime=%3.2f\n",waitStableTime);
// //		printf("do set waitStableTh=%3.2f\n",waitStableTh);
// 	}	
    dwNumBytesToSend = 0;			//Clear output buffer
    do {
        ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	//Get the number of bytes in the device input buffer
    } while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   //or Timeout

	//Check if ACK bit received, may need to read more times to get ACK bit or fail if timeout
	dwCount = 0;
	while(dwGetAck) 
	{
		dwNumInputBuffer = 0;
		do
		{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
			timeoutcounter ++;
			if(timeoutcounter == timeoutvalue)
            {
				timeoutcounter = 0;
				break;
			}
//			printf("timeoutcounter=%d\n",timeoutcounter);
		} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));

		// printf("Write Read ACK dwNumInputBuffer=%d\n",dwNumInputBuffer);

		ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer , &dwNumBytesRead);  	//Read dwNumInputBuffer bytes from device receive buffer

		// static time_t ltime;
		// ltime = time(NULL);
		// fprintf(stderr, "Start InputDataHandle @ %s\n", asctime(localtime(&ltime)));

        InputDataHandle(&dwDataTemp, &rcvdata, InputBuffer, dwNumBytesRead, dwTesioWriteFlag | (dwDataW << 1));                   //Handle received data
        dwGetAck = rcvdata.ack;

		// ltime = time(NULL);
		// fprintf(stderr, "Done InputDataHandle @ %s\n", asctime(localtime(&ltime))); 

		if ((ftStatus != FT_OK) || (dwCount == rwcycle))
		{
			printf("error code 1:fail to get ACK when write byte!---0x%x\n",dwGetAck);
			printf("error addr  :0x%llx error data:0x%llx\n",dwAddr,dwData);
			return FALSE; //Error, can't get the ACK bit from debug port
		}
		else 
		{
		    // dwGetAck = (InputBuffer[0] << 8) | InputBuffer[1];
		    // printf("Do Get WACK:InputBuffer[0]=0x%x,InputBuffer[1]=0x%x\n",InputBuffer[0],InputBuffer[1]);
		    // printf("Do Get WACK:dwGetAck=0x%x\n",dwGetAck);
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

//BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG *dwGetData, BYTE dwAddrBitW, BYTE dwDataBitW, BYTE *ptBuffer, ULONGLONG *ptNum)
BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG *dwGetData, BYTE dwAddrBitW, BYTE dwDataBitW)
{
	FT_STATUS ftStatus = FT_OK;
	ULONGLONG dwData  = 0;
	DWORD dwCount;
	BYTE dwGetAck = 0xff;
	rcvdat rcvdata = {0xff, 0xff, 0xff};
	BYTE dwDataSend[7] = {0};
	BYTE Parity = 1;
	BYTE Array = 0;
	BYTE dwDataTemp[8] = {0};
	BYTE dwAddrW = dwAddrBitW / 8;
	BYTE dwDataW = dwDataBitW / 8;
	BYTE dWSumW  = dwAddrW;

	dwDataSend[0] = 0xfc | (dwTestioStart << 1) | dwTesioReadFlag; 
    //dwAddr
	for(dwCount = 1; dwCount <= dwAddrW; dwCount ++)
	{
		dwDataSend[dwCount] = 0xff & (dwAddr >> (dwAddrW - dwCount) * 8);
	}
	//printf("dwDataSend[5]=0x%x\n",dwDataSend[5]);

	//Calculate Parity
    Array = dWSumW;
	Parity = soc_gen_even_parity_common(&dwDataSend[1],Array * 8);
	Parity = (Parity + dwTesioReadFlag) % 2;
//	printf("Read Addr Parity=0x%x\n",Parity);
    //Parity and dwTestioStop
	dwDataSend[dwCount] = 0x3f | (Parity << 7) | (dwTestioStop << 6);

	// // for debug
	// for(BYTE i = 0; i < 16; i++)
	// {
	// 	printf("dwDataSend[%d]=0x%x\n", i, dwDataSend[i]);
	// }    

	/////////////////////////
	dwNumBytesToSend = 0;

	for(dwCount=0; dwCount<4; dwCount++)	// Repeat commands to ensure the minimum period of the dwTestioStop setup time ie 600ns is achieved
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
		OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set dataout high, clk low, set GPIOL0-2 to 1(high)
		OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��
	}
    
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 	//clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = '\x00';  //0+1=1 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	//Data length of 0x0000 means 1 byte data to clock out

	OutputBuffer[dwNumBytesToSend++] = dwDataSend[0];

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_OUT; 	//clock data byte out on �Cve Clock Edge MSB first
	OutputBuffer[dwNumBytesToSend++] = dwAddrW - 1;  //x+1 bytes to clk out
	OutputBuffer[dwNumBytesToSend++] = '\x00';	     //Data length of 0x0000 means 1 byte data to clock out

	for(dwCount=1; dwCount<=dwAddrW; dwCount++)	// Repeat commands to pass dwDataSend to OutputBufer
	{
		OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount]; 	
		
	}

    //output Parity and dwTestioStop
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop [6]
	OutputBuffer[dwNumBytesToSend++] = '\x01';  //1+1 = 2 bits
	OutputBuffer[dwNumBytesToSend++] = dwDataSend[dwCount];	//Data length of 0x0000 means 1 byte data to clock out
    //Set dataout input?
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x72'; 	//Set data,clk high, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf1';	//Set CLK,GPIOL0-3 pins as output with bit ��1��, data and other pins as input with bit ��0��

	//Get Ack data from chip,clock out with byte in
	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BYTE_IN; 	//clock data bit out on �Cve Clock Edge MSB first, transfer Parity and dwTestioStop
     if((dwAddr >= STATION_SDIO_MTP_ADDR) && (dwAddr <= (STATION_SDIO_MTP_IDLE_ADDR + 8)))
     {
        OutputBuffer[dwNumBytesToSend++] = waitMtpProgByte & 0xff;  //num+1 bytes to clock out
        OutputBuffer[dwNumBytesToSend++] = (waitMtpProgByte >> 8) & 0xff;
     }
     else
	{
   		OutputBuffer[dwNumBytesToSend++] = waitByte & 0xff;  //num+1 bytes to clock out
		OutputBuffer[dwNumBytesToSend++] = (waitByte >> 8) & 0xff;
	}	
//	printf("readAckWaitByte(waitTime,waitfreq)=0x%x,%d\n",AckWaitByte(waitTime,waitfreq),AckWaitByte(waitTime,waitfreq));

    //flush chip buffer to pc
	OutputBuffer[dwNumBytesToSend++] = '\x87';	//Send answer back immediate command

	//Set bus to idle, set clk and data high
	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x73'; 	//Set data,clk high, set GPIOL0-2 to 1(high)
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0-3 pins as output with bit ��1��, other pins as input with bit ��0��

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);		//Send off the commands

//    usleep(waitDataStableTime * waitlevel * 1000);  //calc 13bytes * 8bit / 50 = 2.08ms, then 20 *1000us >> 2.08ms for stable data. 
    usleep(waitlevel);            //for real os only; visual machine: remove it
	dwNumBytesToSend = 0;			//Clear output buffer
	//Check if ACK bit received, may need to read more times to get ACK bit or fail if timeout

    dwNumBytesToSend = 0;			//Clear output buffer
    do {
        ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);	//Get the number of bytes in the device input buffer
    } while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));   //or Timeout

    dwCount = 0;
    while(dwGetAck)
    {
        dwNumInputBuffer = 0;
        do
        {
            ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
            timeoutcounter ++;
            if(timeoutcounter == timeoutvalue)
            {
                timeoutcounter = 0;
                break;
            }
//			printf("timeoutcounter=%d\n",timeoutcounter);
        } while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));
        // printf("Read dwNumInputBuffer=%d\n",dwNumInputBuffer);
        ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);  	//Read dwNumInputBuffer bytes from device receive buffer
//        *ptNum = dwNumBytesRead;
//        memcpy(ptBuffer,InputBuffer,dwNumBytesRead);
        ////////////////////////////////////////////////////////////
        InputDataHandle(&dwData, &rcvdata, InputBuffer, dwNumBytesRead, dwTesioReadFlag | (dwDataW << 1));
        dwGetAck = rcvdata.ack;

    //  printf("Read dwGetAck=%d\n",dwGetAck);
    //	printf("Do Get dwNumBytesRead:dwData=%d\n",dwNumBytesRead);
        if ((ftStatus != FT_OK) || (dwCount == rwcycle))
        {
            printf("error code 1:fail to get ACK when read byte! @ address 0x%llx \n", dwAddr);
            printf("\n");
            return FALSE; //Error, can't get the ACK bit from EEPROM
        }
        else
        {
    //      dwGetAck = (InputBuffer[0] << 8) | InputBuffer[1];
    // 		for(dwCount=0; dwCount < 10 ; dwCount ++)
    // 		{
    // //		printf("InputBuffer[%d]=0x%x\n",dwCount,InputBuffer[dwCount]);
    // 			if(dwCount < 8)
    // 			{
    //                 dwData = (dwData << 8 | InputBuffer[dwCount+1]);
    // //				printf("dwData[%d]=0x%llx\n",dwCount,dwData);
    // 			}
    // 		}
    //		printf("do read dwData=0x%llx\n",dwData);
    //		printf("Do Get ReadWACK:InputBuffer[0]=0x%x,InputBuffer[1]=0x%x\n",InputBuffer[0],InputBuffer[1]);
    //		printf("Do Get ReadWACK:dwGetAck=0x%x\n",dwGetAck);
    //		printf("Do Get ReadData:dwData=0x%llx\n",dwData);
            if (dwGetAck != dwTestioAck)	//Check ACK bit 0 on data byte read out
            {
                //timeout counter
                dwCount++;
            }
            else
            {
                //Calculate Read Data Parity
                //dwData = 0x128c6500000000;
                for(BYTE i=0; i < dwDataW; i++)
                {
                    dwDataTemp[i] = 0xff & (dwData >> ((sizeof(ULONGLONG) -1 -i) * 8));
//                    printf("Do Get dwDataTemp:dwDataTemp[%d]=0x%x\n",i,dwDataTemp[i]);
                }
                Array = dwDataW;
                Parity = ((soc_gen_even_parity_common(dwDataTemp, Array * 8)) + rcvdata.ack) % 2;
//                printf("Do Get ReadData:dwData=0x%llx\n",dwData);
        //		printf("calc parity = 0x%x, rcvdata.parity = 0x%x, rcvdata.ack = 0x%x, addr=0x%llx\n", Parity, rcvdata.parity, rcvdata.ack, dwAddr);
                if(Parity != rcvdata.parity)
                {
                    printf("error code 2:wrong parity = 0x%x, should be parity = 0x%x, rcvdata.ack = 0x%x, addr=0x%llx\n", Parity, rcvdata.parity, rcvdata.ack, dwAddr);
        //			getchar();
                }
                break;
            }
        }
    }
    *dwGetData = dwData >> (sizeof(ULONGLONG) * 8 - dwDataBitW);
//    printf("Do Get ReadData:*dwGetData=0x%llx\n",*dwGetData);
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
	ft_dev_init(speed);
}

void set_reset(void)
{
	dwNumBytesToSend = 0;

	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\x03'; 	//Set gpio low
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��

	

    ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	
//	printf("dwNumBytesSent=%d\n",dwNumBytesSent);
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
	BYTE dwCount = 1;
	//struct timeval tvpre, tvafter;
	//gettimeofday(&tvpre, NULL);
	do
	{
        dwDataStatus = ReadDataAndCheckACK(addr, &data, 32, 32);
		dwCount --;
		if (dwCount == 0)
		{
			break;
		}
	} while (dwDataStatus == FALSE);
	
	//gettimeofday(&tvafter, NULL);
	//printf("Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
    //fprintf(stderr, "do_read: addr = 0x%lx, data = 0x%lx\n", addr, data);
	return data;
}

void do_write(ULONGLONG addr, ULONGLONG data)
{
//	struct timeval tvpre,tvafter;
//    gettimeofday(&tvpre, NULL);
	WriteDataAndCheckACK(addr, 0xffffffff & data, 32, 32);
//	gettimeofday(&tvafter, NULL);
//	printf("Timediff=%dus\n", (tvafter.tv_usec-tvpre.tv_usec));
//	getchar();
//  fprintf(stderr, "do_write: addr = 0x%lx, data = 0x%lx\n", addr, data);
}

void do_write_burst(ULONGLONG addr, ULONGLONG burst_data, DWORD burst_idx)
{
	burst_idx = burst_idx;
	WriteDataAndCheckACK(addr, burst_data, 32, 32);
}

void set_gpiol3(BYTE gpiol3)
{
	dwNumBytesToSend = 0;

	OutputBuffer[dwNumBytesToSend++] = '\x80'; 	//Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] = '\xf3' & ((gpiol3 << 7) | 0x7f); 	//Set gpio high
	OutputBuffer[dwNumBytesToSend++] = '\xf3';	//Set SK,DO,GPIOL0 pins as output with bit ��1��, other pins as input with bit ��0��

    ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	
	printf("set gpiol3=%d\n",gpiol3);
	dwNumBytesToSend = 0;
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
