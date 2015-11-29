/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimSP.h
 ***************************************************************************
 *
 * DESCRIPTION: Simulated SP device interface definitions
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/05/2010 Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMSP
#define __BVDSIMSP

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "simulation/DistributedService.h"

// Enumerate the supported SPS types
enum SpsType
{
    NoSps = 0,
    Sps1_2KW,
    Sps2_2KW
};

class BvdSimSpsBase;
class BvdSimSP;


class SPDistributedServiceInterface: public DistributedService {
public:
    SPDistributedServiceInterface(const char *processName)
    : DistributedService("SPDevice", processName, sizeof(SP_ENVIRONMENT_STATE)*2) {};

    virtual void run() = 0;

    static const UINT_32 SP_INITIALIZED     = 0;  // Indicates that the Service Data is properly initialized
    static const UINT_32 SP_STOP            = 2;  // Control flag indicating that SP SM should stop
    static const UINT_32 SP_PERFORM_NOTIFY  = 3;  // Control flag indicating to perform notify
    static const UINT_32 SP_RUNNING         = 7;  // Status flag indicating that SP is executing
    static const UINT_32 SP_DONE            = 8;  // Status flag indicating that SP has exited
    static const UINT_32 SP_POWERISON       = 16; // Status flag indicating that SP Power is available
    static const UINT_32 SP_BGIOQUIESCED    = 24; // Status flag indicating that SP Background Serives IO is quiesced
};


class SPStateMachine: public SPDistributedServiceInterface {
public:
    SPStateMachine(UINT_32 SPIndex);

    void standardSetup();

    CACHE_HDW_STATUS getHardwareStatus();
    void setHardwareStatus(CACHE_HDW_STATUS status);

    UINT_32 getSpsCharge();
    void setSpsCharge(UINT_32 charge);
   
    /*
     * Used by BvdSimSpsBase to register event notification
     */
    void registerListener(Runnable *listener); 

    void stop();    
    void run();
    
private:
    // activity listener
    Runnable    *mListener;
    UINT_32 mSPIndex;
    PSP_ENVIRONMENT_STATE mEnvInfo;
};

class SPControl: public SPDistributedServiceInterface {
public:
    SPControl();

    void run();

    // Reset the SP device, i.e. clear its shared data.
    // CAUTION: THIS SHOULD ONLY BE USED BY A CALLER THAT HAS CONTROL OF THE
    // SP DEVICE'S CONTEXT.
    void Reset(UINT_32 index = 0xffffffff);

    void SetPowerOk(UINT_32 index = 0xffffffff);
    void SetPowerFail(UINT_32 index = 0xffffffff);
    void SetBatteryExhausted(UINT_32 index = 0xffffffff);

    CACHE_HDW_STATUS GetHardwareStatus(UINT_32 index = 0xffffffff);

    // Returns - true if the BG Services are quiesced
    bool IsBackgroundServicesIoQuiesced(UINT_32 index = 0xffffffff);

    // Query the state of the SPS power.
    // Returns - true if the SPS is providing power.
    bool IsPoweredOn(UINT_32 index = 0xffffffff);


private:
    void PerformNotify(UINT_32 index);

    SPStateMachine *mSMs[2];
};

typedef struct SPSDeviceData_s
{
    SpsType type;
    UINT_32 index;
    char deviceName[64];
    PIOTaskId taskId;
} SPSDeviceData_t;

// Enumerate the types of SPS events
enum SpsEventType
{
    PowerOkEvent = 1,
    PowerFailEvent,
    BatteryExhaustedEvent,
    BatteryChargeEvent,
    NoMoreEvents            // no more events - used as a terminal entry in event array 
};

// Define an SPS event
struct SpsEvent
{
    SpsEventType    eventType;  // type of event
    ULONG           seconds;    // number of seconds to wait before setting the event
    ULONG           pctCharge;  // amount of charge to set on BatteryChargeEvent
};


extern PEMCPAL_DEVICE_OBJECT BvdSim_create_CLARiiON_SP(int SP = -1, SpsType SPS = Sps1_2KW);
extern AbstractDriverReference *get_BvdSimSP_DriverReference();


//*****************************************************************************
// BvdSimSpsBase class is an abstract base class that defines the base
// SPS device. All variations of SPS devices are derived from this class.
// NOTE: the default behavior implements BvdSim1_2KwSps logic
//*****************************************************************************
class BvdSimSpsBase: public Runnable {

public:

    // Constructor
    BvdSimSpsBase(BvdSimSP *driver, SPStateMachine *spsSM);

    // Destructor
    virtual ~BvdSimSpsBase();

    /*
     * The BvdSimSpsBase registers with the SPStateMachine
     * When ever the SPStateMachine sees flag SP_PERFORM_NOTIFY
     * it invokes the listeners run method, to 
     * trigger the Drivers CompleteStateChangeNotification
     */
    void run();

    // required by the Runnable API
    const char *getName() { return "BvdSimSpsBase runner"; };

    // Get the status of the SPS hardware
    CACHE_HDW_STATUS getHdwStatus();

    // Get the SPS percentage charged
    ULONG getPercentCharged();

    virtual void setPowerOkEvent();

    virtual void setPowerFailEvent();
    
    virtual void setBatteryExhaustedEvent();

    virtual void setBatteryChargePercent(ULONG percentage);


protected:

    // Set the status of the SPS hardware
    void setHdwStatus(CACHE_HDW_STATUS status);

    // Set the SPS percentage charged
    void setPercentCharged(ULONG percent);


private:

    // Default constructor
    BvdSimSpsBase() {};


    // The environmental data for this SPS
    SPStateMachine *mSpsSM;

    // The SP Driver
    BvdSimSP *mDriver;
};  // class BvdSimSpsBase





//*****************************************************************************
// BvdSimSP class - the simulated SP device
//*****************************************************************************
class BvdSimSP {

public:

    // Every Class that implements VirtualFlareDevice must implement a static 
    // factory method that constructs instances for this class, using the supplied 
    // memory contained in DeviceObject->DeviceExtension.
    static BvdSimSP *factory(PEMCPAL_DEVICE_OBJECT DeviceObject, SPSDeviceData_t * deviceData);

    // The SPS state has changed. Complete the notification IRP
    void CompleteStateChangeNotification();

    // Cancel the state change notification. Complete the IRP with cancel status.
    void CancelStateChangeNotification();

    // Cancel a waiting IRP.
    static void WaitingIrpCancelRoutine(PVOID Unused, PBasicIoRequest Irp);

    // Dispatch DEVICE_CONTROL IRPs.
    EMCPAL_STATUS DispatchIRP_MJ_DEVICE_CREATE(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
    EMCPAL_STATUS DispatchIRP_MJ_DEVICE_CLOSE(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
    EMCPAL_STATUS DispatchIRP_MJ_DEVICE_CONTROL(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);

    ~BvdSimSP();

private:
    BvdSimSP(SPSDeviceData_s * deviceData, PEMCPAL_DEVICE_OBJECT DeviceObject, SPStateMachine *spSM);

    // Populate the IRP with the environmental state from the SPS
    void GetEnvironmentalState(PEMCPAL_IRP Irp);


    SPSDeviceData_s mDeviceData;
    PEMCPAL_DEVICE_OBJECT mDeviceObject;

    SPStateMachine *mSpSM;

    // True when a state change is pending
    bool mPendingStateChange;

    // Spin lock, used to manage access to mNotificationIrps
    K10SpinLock mLock;

    // Pointer to the associated SPS object
    BvdSimSpsBase* mSps;
    
    // State change notification IRPs are queued here
    ListOfCancelableBasicIoRequests mNotificationIrps;

};  // class BvdSimSP

#endif // __BVDSIMSP


