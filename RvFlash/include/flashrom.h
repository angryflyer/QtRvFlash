#ifndef FLASHROM_H
#define FLASHROM_H

#include <windows.h>
#include "sw_header.h"
#include "flashcfg.h"

extern DWORD waitfreq;       //default freq KHz, same with rvlink speed for calculating usb waitime
extern DWORD ft_listdevices; //default list device num and type
extern DWORD waitlevel;      //adjust wait time of usb bus for get stable data

void usleep(unsigned long usec);

void gettimeofday(struct timeval* tv, void* tzp);
void setup(DWORD speed);
void set_reset(void);
void release_reset(void);
ULONGLONG do_read(ULONGLONG addr);
void do_write(ULONGLONG addr, ULONGLONG data);
void do_write_burst(ULONGLONG addr, ULONGLONG burst_data, DWORD burst_idx);
void set_gpiol3(BYTE gpiol3);

int flashrom(int argc, const char * argv[]);

BOOL ft_device_init(DWORD speed);

void process_reset(void);

BOOL WriteDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG dwData);

BOOL ReadDataAndCheckACK(ULONGLONG dwAddr, ULONGLONG* dwGetData);

BOOL chipConnect(void);

//BOOL flashErase(ULONGLONG dwAddr, ULONGLONG dwLegth);

//BOOL flashWrite(ULONGLONG dwAddr, BYTE* dwData, ULONGLONG dwLegth);

//BOOL flashRead(ULONGLONG dwAddr, BYTE* dwGetData, ULONGLONG dwLegth);

BOOL rv_write(ULONGLONG addr, ULONGLONG data);
BOOL rv_read(ULONGLONG addr, ULONGLONG *data);

BOOL ft_open(void);

BOOL ft_close(void);

#endif // FLASHROM_H
