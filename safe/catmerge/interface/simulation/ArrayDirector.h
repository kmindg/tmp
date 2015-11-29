/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ArrayDirector.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declarations of classes ArrayDirector, SimDirector
 *                                  SPProcessControl and related data structures
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/27/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ARRAYDIRECTOR_H__
#define __ARRAYDIRECTOR_H__

# include "generic_types.h"
# include "shmem_class.h"
# include "AbstractProgramControl.h"
# include "AbstractArrayManager.h"
# include "mut_cpp.h"
# include "simulation/Runnable.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif


#define DIRECTOR_NAME "simulation_control"
#define SPA_NAME "SPA_control"
#define SPB_NAME "SPB_control"

typedef enum simulation_control_flags_e {
    /*
     * Simulation Startup control
     */
    REPORT_CONFIG = 0,
    EXIT_AFTER_REPORT_CONFIG = 1,

    /*
     * Local Configuration Options
     */
    CONTROL_CONFIG_TERMINATOR = 2,   //true = simulation control handles configuration
    LOAD_STANDARD_CONFIG = 3,        //true = Usersim programs Terminator using usersim_config.xml
                                     //false = Userim program terminator with specified config
    EXIT_AFTER_CONFIGURATION = 4,    // used for debug purposes

    /*
     * Debug Controls, note, ArrayDirector::mCurrentSPIndex indexes into these flags
     */
    DEBUG_SPA = 5,
    DEBUG_SPB = 6,

    /*
     * Process Management Controls, ArrayDirector::mCurrentSPIndex indexes into these flags
     */
    HALT_SPA = 7,  // immediate process exit
    HALT_SPB = 8,

    SHUTDOWN_SPA = 9, // Orderly system shutdown, followed by exit, ArrayDirector::mCurrentSPIndex indexes into these flags
    SHUTDOWN_SPB = 10,

    NO_FAILURE_ON_TIMEOUT = 11,  // allow a test to stop and not fail

    EXPECT_PEER_DEATH = 12,
    EXCEPTION_ENCOUNTERED = 13,

} simulation_control_flags_t;

typedef enum sp_control_flags_e {
    /*
     * Transports available within the Simulation
     */
    TRANSPORT_FCLI = 0,
    TRANSPORT_ADM = 1,
    TRANSPORT_PACKET_INTERCEPTION = 2,
    TRANSPORT_TAPI = 3,
    TRANSPORT_FAPI = 4,

    /*
     * Subsystems available(statically linked) within the Simulation
     */
    SYSTEM_FLARE =   32,
    SYSTEM_PHYSICAL_PACKAGE = 33,
    SYSTEM_TERMINATOR = 34,
    SYSTEM_L1CACHE  = 35,
    SYSTEM_EFDCACHE = 36,
    SYSTEM_CMID     = 37,

    /*
     * BVDSim SP Status Flags
     */
    SP_RUNNING = 40,    
    SP_PRESENT = 41,

    /*
     * Handshaking controls
     */
    QUERYCOMPLETE = 50,

    SP_READY_FOR_CONFIGURATION = 51,
    SP_CONFIGURATION_COMPLETE  = 52,
    SP_SHUTDOWN_COMPLETE       = 53,
    SP_HALT_COMPLETE           = 54,
} sp_control_flags_t;

/*
 * The test service uses the services flags to handshake control operations
 *
 * The Director updates the simulation_test_control_t data, then sets sim_test_trigger
 *    After which, the directory waits for the SP ack bits to become set.
 *    Once ack is received, the directory clears all flags
 *
 * The SP process test control waits for sim_test_trigger to be set.
 *   Once set, the Sp process retrieves the simulation_test_control_t data,
 *   then sets its ack flag, to acknowledge operation acceptance.
 *
 *
 */
typedef enum simulation_test_flags_e
{
    sim_test_trigger        = 0,
    sim_test_spa_ack        = 1,
    sim_test_spb_ack        = 2,
} simulation_test_flags_t;

typedef enum simulation_test_states_e
{
    sim_test_idle           = 0,
    sim_test_setup          = 1,
    sim_test_test           = 2,
    sim_test_teardown       = 4,
    sim_testing_complete    = 5
} simulation_test_states_t;

typedef struct simulation_test_control_s
{
    volatile UINT_32 state;
    volatile UINT_32 index;
    volatile UINT_8 buffer[128];
} simulation_test_control_t;

typedef struct simulation_control_data_s
{
    char configuration_path[256];
} simulation_control_data_t;

typedef struct sp_control_s
{
    int version;
    UINT_32 spID;
} sp_control_t;





# ifdef __cplusplus

/*
 * forward class declarations
 */
class SimulationDirector;
class ArrayDirector;


class SPSimulationControl: public shmem_service
{
public:
    SHMEMDLLPUBLIC SPSimulationControl(shmem_segment *Segment, shmem_service *Control, char *name, UINT_32 Index);
    SHMEMDLLPUBLIC ~SPSimulationControl();
    SHMEMDLLPUBLIC void setVersion(int Version);
    SHMEMDLLPUBLIC int getVersion();
    SHMEMDLLPUBLIC UINT_32 getSpControlProcessIndex();
    SHMEMDLLPUBLIC char *getName();

    SHMEMDLLPUBLIC void setSpID(UINT_32 spID);
    SHMEMDLLPUBLIC UINT_32 getSpID();

    SHMEMDLLPUBLIC void setProcessControl(AbstractProgramControl *processControl);
    SHMEMDLLPUBLIC AbstractProgramControl *getProcessControl();

    SHMEMDLLPUBLIC UINT_64             getControlFlags();
    SHMEMDLLPUBLIC bool                getControlFlag(UINT_32 index);
    SHMEMDLLPUBLIC void                setControlFlag(UINT_32 index, bool value);
    SHMEMDLLPUBLIC bool                getStatusFlag(UINT_32 index);
    SHMEMDLLPUBLIC void                setStatusFlag(UINT_32 index, bool value);
    

    SHMEMDLLPUBLIC void BootSP();
    SHMEMDLLPUBLIC void DumpSP(const char* cmd);
    SHMEMDLLPUBLIC void RestoreSP(const char* cmd);
    SHMEMDLLPUBLIC void ShutdownSP();
    SHMEMDLLPUBLIC void HaltSP();

    SHMEMDLLPUBLIC void waitForSPProcessExit();
    SHMEMDLLPUBLIC static void monitorForHaltThread(PVOID context);
    SHMEMDLLPUBLIC static void registerHaltListner(Runnable *listner);
    SHMEMDLLPUBLIC static void unregisterHaltListner(Runnable *listner);

private:
    void performSPCompletionHandshake(UINT_32 command_flag, UINT_32 completion_flag);
    void waitForSPProcessAcknowledge(UINT_32 flag);
    void monitorForHalt();

    shmem_service           *mControl;
    sp_control_t            *mSpData;
    AbstractProgramControl  *mProcessControl;
    AbstractProgramControl  *mAltProcessControl;
    UINT_32                 mIndex;
    char                    *mName;
    static Runnable         *sListner;
};

class sp_test_service
{
public:
    sp_test_service(shmem_segment *Segment);

};



class SimulationDirector: public shmem_service
{
public:
    SHMEMDLLPUBLIC ~SimulationDirector();

    SHMEMDLLPUBLIC void registerSpAProgram(char *command);
    SHMEMDLLPUBLIC void registerSpBProgram(char *command);

    SHMEMDLLPUBLIC void BootSystem();
    SHMEMDLLPUBLIC void ShutdownSystem();


    
private:
    friend class ArrayDirector;

    SimulationDirector(shmem_segment *Segment);

    simulation_control_data_t   *mControlData;
};


class AbstractSPTester
{
public:
    virtual void ControlTesting() = 0;
};

class SubordinateSPTestControl: public AbstractSPTester
{
public:
    SHMEMDLLPUBLIC SubordinateSPTestControl(SimulationDirector *Director, SPSimulationControl *SPControl, CAbstractTest *testControl);

    SHMEMDLLPUBLIC void ControlTesting();

private:
    SimulationDirector   *mDirector;
    SPSimulationControl *mSPControl;
    CAbstractTest       *mTestControl;
};

/*
 * The ArrayDirector is a singleton.
 * It provides static factory methods to construct
 * all necessary control instances.
 *
 * All simulation applications need to use this API
 * to instantiate thier require controllers
 */
class ArrayDirector: public AbstractArrayManager
{
public:
    SHMEMDLLPUBLIC ArrayDirector(shmem_segment *segment);
    SHMEMDLLPUBLIC ~ArrayDirector();
    
    SHMEMDLLPUBLIC void set_flags(UINT_64);
    SHMEMDLLPUBLIC bool get_flag(UINT_32 index);
    SHMEMDLLPUBLIC void set_flag(UINT_32 index, bool value);

    
    SHMEMDLLPUBLIC void BootSystem();
    SHMEMDLLPUBLIC void ShutdownSystem();
    SHMEMDLLPUBLIC void HaltSystem();

    /*
     * Return the SegmentID for the ArrayDirector Shared segment
     */
    SHMEMDLLPUBLIC const char *getSegmentID();

    /*
     * Return whether or not we are failing on timeout
     */
    SHMEMDLLPUBLIC static bool ShouldNotFailOnTimeout();

    /*
     * Use Static Class access methods
     */
    SHMEMDLLPUBLIC static void handle_simulation_query(UINT_32 sp);

    SHMEMDLLPUBLIC static ArrayDirector        *getSimulationDirector();
    SHMEMDLLPUBLIC static SPSimulationControl  *getSpAControl();
    SHMEMDLLPUBLIC static SPSimulationControl  *getSpBControl();
    SHMEMDLLPUBLIC static SPSimulationControl  *getCurrentSPControl();
    SHMEMDLLPUBLIC static SPSimulationControl  *getPeerSPControl();
    SHMEMDLLPUBLIC static bool                  isPeerPresent();
    SHMEMDLLPUBLIC static void                  setSPPresent();
    SHMEMDLLPUBLIC static void                  setPeerSPPresent();
    SHMEMDLLPUBLIC static void                  setSPAPresent();
    SHMEMDLLPUBLIC static void                  setSPBPresent();

    SHMEMDLLPUBLIC static void                  ProcessExceptions();

    SHMEMDLLPUBLIC static void                  ConfigureDebugger(int sp);
    SHMEMDLLPUBLIC static void                  MonitorSystem();
    
    SHMEMDLLPUBLIC static char    *DIRECTOR_SEGMENT_NAME; // see ArrayDirector.cpp for initialization
    SHMEMDLLPUBLIC static UINT_32 DIRECTOR_SEGMENT_SIZE;  // see ArrayDirector.cpp for initialization

private:
    bool                 wait();
    void                 notify();

    static void        CSX_GX_RT_DEFCC breakPointListener(csx_cstring_t file, csx_nuint_t line);

    static ArrayDirector *getInstance();

    UINT_8               mCurrentSPIndex;
    shmem_segment        *mSegment;
    
    SimulationDirector   *mSimulationControl;
    SPSimulationControl  *mSpAControl;
    SPSimulationControl  *mSpBControl;
    SPSimulationControl  *mCurrentSP;
    static PVOID          sThreadID;
};

# endif


#endif // __ARRAYDIRECTOR_H__
