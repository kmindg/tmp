/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractStateMachine.h
 ***************************************************************************
 *
 * DESCRIPTION: SimPciHalSharedMemroyDevice class declarations.
 *       This class provides an implementation of production CMIDpci driver
 *       that performs communcation between SPs, via the shmem shared memory
 *       library.  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/28/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTSTATEMACHINE__
#define __ABSTRACTSTATEMACHINE__
# include "generic_types.h"
# include "shmem.h"
# include "EmcPAL.h"
# include "simulation/shmem_class.h"


typedef enum asm_state_e {
    ASM_DISABLED            = 0,
    ASM_EXECUTING           = 2,
    ASM_RETIRED             = 3,
    ASM_INIT_REQUEST        = 5,
    ASM_SHUTDOWN_REQUEST    = 6,
} asm_state_t;

class AbstractStateMachine
{
public:
    virtual ~AbstractStateMachine() {}
    virtual void        StateMachine() = 0;

    virtual char            *getName() = 0;
    virtual void setState(ULONG state) = 0;
    virtual ULONG           getState() = 0;
    virtual bool                wait() = 0;
    virtual bool                wait(ULONG msTimeout) = 0;
    virtual void              notify() = 0;
    virtual void                lock() = 0;
    virtual void              unlock() = 0;
};

class StateVariable
{
public:
    StateVariable(volatile ULONG *State);
    virtual ~StateVariable() {}

    virtual void setState(ULONG state);
    virtual ULONG           getState();

private:
    volatile ULONG *mState;
};

class SharedMemoryService_StateMachine: public AbstractStateMachine, public shmem_service
{
public:
    SharedMemoryService_StateMachine(shmem_segment *Segment,shmem_service *Service,ULONG offsetToStateVariable);

    SharedMemoryService_StateMachine(shmem_segment *Segment,
                                        char *Name,
                                        ULONG ServiceDataSize,
                                        ULONG offsetToStateVariable);

    virtual ~SharedMemoryService_StateMachine();

    /*
     * implementing classes must provide
     * method StateMachine()
     */
    virtual void        StateMachine() = 0;

    char            *getName();
    void setState(ULONG state);
    ULONG           getState();
    bool                wait();
    bool                wait(ULONG msTimeout);
    void              notify();
    void                lock();
    void              unlock();

protected:
    shmem_segment   *mSegment;
    StateVariable *mStateVariable;
};

class StateMachineControl
{
public:
    StateMachineControl(AbstractStateMachine *StateMachine);
    virtual ~StateMachineControl() {}
    virtual void     start();
    virtual void      stop();
    virtual bool  AdvanceToState(ULONG startState, ULONG triggerState, ULONG endState);

protected:
    AbstractStateMachine     *mStateMachine;
};


class ThreadStateMachineControl: public StateMachineControl
{
public:
    ~ThreadStateMachineControl();
    ThreadStateMachineControl(AbstractStateMachine *StateMachine);
    virtual void     start();
    virtual void      stop();
    

    static void ThreadLauncher(IN PVOID StartContext);
protected:
    EMCPAL_THREAD           mEmcPALThreadControl;
};

class AbstractDispatcher
{
public:
    virtual void Dispatch(ULONG index) = 0;
};

#endif // __ABSTRACTSTATEMACHINE__
