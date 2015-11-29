#ifndef __BARRIER_H__
#define __BARRIER_H__

#include "generic_types.h"

#ifdef ALAMOSA_WINDOWS_ENV

#define BARRIER_BUFSIZE 128
class BvdSimBarrier {
public:
    BvdSimBarrier(void) {};
    ~BvdSimBarrier(void) {};

    BOOL InitServer(char *name, ULONG initialTokenValue=0);
    BOOL InitClient(char *name, ULONG initialTokenValue=0);
    BOOL WakeupThenWait(void);
    BOOL WaitForTwoRunBoth(void);
    BOOL Wakeup(void);
private:
    // HANDLE mhPipe;
    void *mhPipe;
    char mBuffer[BARRIER_BUFSIZE];
    ULONG mToken;
};

#endif /* ALAMOSA_WINDOWS_ENV - DEADCODE */

#endif
