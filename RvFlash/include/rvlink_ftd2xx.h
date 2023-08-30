#ifndef __RVLINK_FTD2XX_H
#define __RVLINK_FTD2XX_H

#include <stdio.h>
#include <stdbool.h> 
//#include <unistd.h>
//#include <sys/time.h>
#include "mytime.h"
#include "ftd2xx.h"

extern DWORD ft_freq;        //default freq KHz, same with rvlink speed for calculating usb waitime
extern DWORD ft_list_device; //default list device num and type
extern DWORD ft_wdelay;      //adjust wait time of usb bus for get stable data
extern DWORD ft_rdelay;      //adjust wait time of usb bus for get stable data
extern DWORD ft_dev_index;
extern char  ft_product[64];

typedef struct {
    BYTE start;
    BYTE ack;
    BYTE parity;
} rcvdat;

void setup(DWORD speed);

void set_reset(void);

void release_reset(void);

void set_gpiol3(BYTE gpiol3);

ULONGLONG do_read(ULONGLONG addr);

void do_write(ULONGLONG addr, ULONGLONG data);

void do_write_burst(ULONGLONG addr, ULONGLONG burst_data, DWORD burst_idx);

BOOL ft_dev_init(DWORD speed);
BYTE ft_list_dev(void);
BYTE gen_even_parity(BYTE *entry_data, WORD entry_len);
BYTE gen_odd_parity(BYTE *entry_data, WORD entry_len);
BOOL WriteDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG dwData, BYTE dwAddrBitW, BYTE dwDataBitW);
BOOL WriteDataBurst(ULONGLONG dwAddr, ULONGLONG *dwData, BYTE dwAddrBitW, BYTE dwDataBitW, DWORD dwBurstLen);
BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG *dwGetData, BYTE dwAddrBitW, BYTE dwDataBitW);
BYTE RotateLeft(BYTE *InputData , DWORD Lenth , DWORD ShiftLenth);
BYTE HandleData(ULONGLONG *dwDataHandled, rcvdat *dwAckData, BYTE *InputData, DWORD InputDataLenth, BYTE rwType);

BOOL ft_open(void);

BOOL ft_close(void);

/*
void setup(unsigned short serverPort, struct sockaddr_in * serverAddr_p, int * clientSock_p);

void set_reset(struct sockaddr_in * serverAddr_p, int * clientSock_p);
void release_reset(struct sockaddr_in * serverAddr_p, int * clientSock_p);

void do_status_check(struct sockaddr_in * serverAddr_p, int * clientSock_p);

uint64_t do_read(struct sockaddr_in * serverAddr_p, int * clientSock_p, uint64_t addr);

void do_write(struct sockaddr_in * serverAddr_p, int * clientSock_p, uint64_t addr, uint64_t data);
void do_write_burst(struct sockaddr_in * serverAddr_p, int * clientSock_p, uint64_t addr, uint64_t* data, int len);

uint64_t do_read_backdoor(struct sockaddr_in * serverAddr_p, int * clientSock_p, uint64_t addr);

void do_write_backdoor(struct sockaddr_in * serverAddr_p, int * clientSock_p, uint64_t addr, uint64_t data);
*/
#endif
