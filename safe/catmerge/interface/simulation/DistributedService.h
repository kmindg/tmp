/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DistributedService.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of class DistributedService
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/21/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _DISTRIBUTEDSERVICE_
#define _DISTRIBUTEDSERVICE_

# include "simulation/shmem_class.h"
# include "generic_types.h"
# include "simulation/Runnable.h"
# include "simulation/thread.h"


//
// NOTE: This class should not be used as a basis for attempting to create 3 legged stool tests
//   All new test development, whether or not it is a multi-sp test should  be done with
//  the classes found in BvdSim3LMutBaseClass.h
//
class DistributedService: public Runnable {
public:
    DistributedService(const char *uniqueServiceName, const char *processName = NULL, UINT_32 size = 0);
    ~DistributedService();

    /*
     * Methods to set/get Service state
     * Every DistributedService has 64 flag bits (0-63) available
     * to control and/or communicate with processing threads.
     * 
     * Implementing subclasses are responsible for managing the flags
     */
    void                set_thisSP_flags(UINT_32 flags, UINT_32 mask = 0xffffffff);
    void                set_peerSP_flags(UINT_32 flags, UINT_32 mask = 0xffffffff);
    UINT_32             get_thisSP_flags();
    UINT_32             get_peerSP_flags();
    void                set_flags(UINT_64, UINT_64);
    void                set_flags(UINT_64);
    UINT_64             get_flags();
    UINT_64             get_set_flags(UINT_64 flags);
    bool                get_flag(ULONG index);
    void                set_flag(ULONG index, bool flagValue);

    /*
     * The get/set_SP_flag methods partition the 64 flag bits 
     *  into two groups of 32 flags.
     * 
     * These methods index into the two groups of flags
     * using BvdSim_Registry::getSpId(), which controls the SP ID used
     * to confure a simulation process as either SPA or SPB.
     *
     * These methods are intended as convenience methods,
     * in the thread control run method, removing the need for subclasses
     * to have explicit logic to translate between the SP ID and a particular
     * flag, in a particular group..
     * 
     */
    bool                get_SP_flag(ULONG index);
    void                set_SP_flag(ULONG index, bool flagValue);

    void                wait_for_SP_status(ULONG index, bool flagValue=TRUE, ULONG pollFrequencyInMilliseconds = 1000, ULONG timeoutInMilliseconds = DefaultTimeout);

    bool                get_peerSP_flag(ULONG index);
    void                set_peerSP_flag(ULONG index, bool flagValue);

    void                wait_for_peerSP_status(ULONG index, bool flagValue=TRUE, ULONG pollFrequencyInMilliseconds = 1000, ULONG timeoutInMilliseconds = DefaultTimeout);

    /*
     * The wait_for/get/set_SP[A,B,Both]_flag/_status methods also partition the 64B flag bits
     * into two groups of 32 flags
     *
     * These methods provide direct/explicit access to the flags, for control purposes.
     *
     * These methods are intended as convenience methods, for use by test control logic, 
     * removing the need to translate between SP ID and a particular flag in a particular
     * group.
     */
    bool                get_SPA_flag(ULONG index);
    void                set_SPA_flag(ULONG index, bool flagValue);
    bool                get_SPB_flag(ULONG index);
    void                set_SPB_flag(ULONG index, bool flagValue);

    void                wait_for_SPA_status(ULONG index, bool flagValue=TRUE, ULONG pollFrequencyInMilliseconds = 1000, ULONG timeoutInMilliseconds = DefaultTimeout);
    void                wait_for_SPB_status(ULONG index, bool flagValue=TRUE, ULONG pollFrequencyInMilliseconds = 1000, ULONG timeoutInMilliseconds = DefaultTimeout);

    /*
     * convenience method, allowing a test to set both the A & B flags concurrently (Non atomic).
     */
    void                set_SPBoth_flag(ULONG index, bool flagValue);

    /*
     * convenience method, allowing a test to wait for both the A & B flags to become set.
     */
    void                wait_for_SPBoth_status(ULONG index, bool flagValue=TRUE, ULONG pollFrequencyInMilliseconds = 1000, ULONG timeoutInMilliseconds = DefaultTimeout);

    /*
     * convenience methods, allowing test to reset flags using a bitmask
     */
    void resetSPAFlagsFromMask(UINT_32 mask);
    void resetSPBFlagsFromMask(UINT_32 mask);
    void resetFlagsFromMask(UINT_32 mask);

    /*
     * Returns a pointer to the memory allocated during construction
     */
    void *getMemory();

    void start();
    virtual void stop();    // shutdown processing thread
    virtual void run() = 0; // processing thread logic, implemented in subclass
    virtual const char *getName();// returns DistributedService name

private:

    static const UINT_32 DefaultTimeout = 5*60*1000;
    static const UINT_32 SPDivisor = 32;

    const char *mProcessName;
    const char *mName;
    const UINT_32 mSize;
    shmem_segment *mSegment;
    shmem_service *mService;
    Thread *mThread;
};

#endif // _DISTRIBUTEDSERVICE_
