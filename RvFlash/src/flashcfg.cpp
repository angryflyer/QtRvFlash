#include "flashcfg.h"

flashCtrl flashCtl;

void flashctl_reset(void)
{
    flashCtl.autoFlag = false;
    flashCtl.eraseFlag = false;
    flashCtl.writeFlag = false;
    flashCtl.readFlag = false;
    flashCtl.verifyFlag = false;
}
