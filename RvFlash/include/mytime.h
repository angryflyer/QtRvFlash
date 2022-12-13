#ifndef MYTIME_H
#define MYTIME_H

#include "time.h"
#ifdef _WIN32
// Compiling on Windows
#include <windows.h>
#endif

void usleep(unsigned long usec);

void gettimeofday(struct timeval* tv, void* tzp);

#endif // MYTIME_H
