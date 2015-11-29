/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2006,2010-2015 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 ************************************************************************/

#ifndef __BasicVolumeDriver__
#define __BasicVolumeDriver__

//++
// File Name:
//      BasicVolumeDriver.h
//
// Contents:
//      Defines the BasicVolumeDriver class.
// C4201: nonstandard extension used : nameless struct/union
// C4100: 'identifier' : unreferenced formal parameter
// C4127: conditional expression is constant
// C4512: 'class' : assignment operator could not be generated
// C4291: 'void *operator new(size_t,void *)' : no matching operator delete found; memory will not be freed if initialization throws an exception
//           - BVD assumption is that exceptions are disabled.  Simulation build environment does not support this.
#pragma warning(disable : 4201 4100 4127 4512 4291)

extern "C" {
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "ktrace.h"
};


# include "BasicVolumeDriver/BasicVolume.h"
# include "BasicLib/ConfigDatabase.h"
# include "CmiUpperInterface.h"

# include "BasicLib/BasicObjectTracer.h"
# include "BasicLib/BasicTraceLevelToRingMapping.h"

# include "BasicVolumeDriver/AllocatedMemoryTracker.h"
# include "BasicVolumeDriver/RequiredMemoryCalculator.h"
# include "BasicLib/LockedListOfCancelableBasicIoRequest.h"
# include "flare_ioctls.h"
# include "CrashCoordinator.h"



// The Basic Volume Driver package exists for the purpose of making it easier to create
// drivers that support the K10 Layered Driver interface, which in turn assumes drivers
// based on the Windows Driver Model (WDM).
//
// To leverage this package, a specific driver must implement subclasses of both
// BasicVolumeDriver and BasicVolume.  The BasicVolumeDriver subclass would be 1:1 with
// the WDM DRIVER_OBJECT, and the BasicVolume subclass is actually stored in the WDM
// DEVICE_OBJECT::DeviceExtension.
// 
// The subclass of BasicVolume would typically have an instance of ConsumedVolume in order
// to open a DEVICE_OBJECT from the driver below, and to forward IRPs to that device
// object.
//
// A simple driver that just passes IRPs through to the driver below it can be implemented
// by just subclassing BasicVolume and BasicVolumeDriver.  The BasicVolumeDriver provides
// means of creating volume DEVICE_OBJECTS either via the Windows registry, or through the
// K10 Auto-Insert Driver interface.  These mechanisms create device objects on one SP
// only.
//
// The current incarnation of BasicVolumeDriver provides neither persistence mechanisms
// nor administrative transactions.   Note, however, that the KDBM package could be used
// for persistence, and the Invista SyncTransaction component could be used for
// transaction support.
//
// The IoIRPTracker class provides an implementation of an object that would be allocated
// to keep track of an IRP that has entered this driver, if more tracking than simply
// queuing the IRP on a ListOfBasicIoRequest is needed. IoIRPTracker has support for the
// K10 DCA Read/Write IRPs. It is assumed that IoIRPTracker would be subclassed.
//
// Object based tracing is supported via the BasicObjectTracer interface, which is
// implemented for both BasicVolume and BasicVolumeDriver.  This allows ktrace control on
// a per object level via a standardized interface.
//
// Windows Registry access is provided in a simple form via the classes
// RegistryBasedDatabaseKey and RegistryBasedDatabaseValue.  The same interface is also
// supported by a purely transient implementation of the classes MemoryBasedDatabaseKey
// and MemoryBasedDatabaseValue.
//
// Substantial support is provided for SP to SP messaging, including Volume to Volume
// messages.  A VolumeMultiplexor allows messages to be addressed to specific volumes in
// another SP's instance of the same driver.  The classes InboundMessage,
// OutboundMessagePayload, and OutboundVolumeMessagePayload are provided as part of this
// support.
// 
// The class BasicDeviceObject is provided to allow a driver to create its own WDM device
// object that is not a BasicVolume. BasicDeviceObject allows the driver's IRP dispatch
// functions to route incoming IRPs to such driver created device objects.
class SubclassDriver
{
    // Not a real class, intended to trigger comments above to be loaded into reverse
    // engineering tools.
};

class VolumeMultiplexor;


// We need to keep a list of these objects.
SLListDefineListType(CMI_SP_CONTACT_LOST_EVENT_DATA, ListOfCMI_SP_CONTACT_LOST_EVENT_DATA);
SLListDefineInlineCppListTypeFunctions(CMI_SP_CONTACT_LOST_EVENT_DATA, 
                                       ListOfCMI_SP_CONTACT_LOST_EVENT_DATA, FLink);


// BasicVolumeDriver is a singleton class that provides the driver framework for K10
// "Layered Drivers".
//
// The standard usage is for ::DriverEntry to instantiate a single instance of a sub-class
// of this class, and then to call BasicVolumeDriver::DriverEntry.
//
// BasicVolumeDriver inherits from BasicObjectTracer to allow debugging statements to be
// logged into the ktrace log.
//
// BasicVolumeDriver maintains a collection of BasicVolumes, where registry entries
// specify which Volumes to create at startup. The sub-class driver must supply the size
// and constructor for a sub-class of BasicVolume, so that BasicVolumeDriver can create
// instances of such BasicVolume sub-classes.
//
// The Registry entries that the BasicVolumeDriver reads are found in
//  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\<Driver Name>\Parameters
//
//    All sub-keys of "Parameters" are enumerated, and those sub-key names are
//    unimportant.  If a subkey has a string value named "DeviceName", then the
//    BasicVolumeDriver will try to create a volume whose WDM device name is the
//    associated string value.  Another expected value at the same level as "DeviceName"
//    is the 16 byte binary value called "WWN", which is a unique identifier for the
//    volume. If the volume is successfully created, and the string value named
//    "DeviceNameLink" is found, a WDM symbolic link will be created, linking the value of
//    "DeviceNameLink" to the value of "DeviceName". The sub-class driver, when creating a
//    volume, is passed in a handle to the sub-key of parameters that defines this
//    particular volume, allowing that driver to retrieve other registry parameters.
//
//    Note: "DeviceName" string values should not be created under the Factory key.
//
//    If "Parameters/Factory" has a string value named "FactoryName", then WDM
//    DEVICE_OBJECT with the device name specified by the string value will be created.
//    This device object will support the AIDVD (Auto-Insert Disk Volume Device), allowing
//    other drivers on this SP to create and destroy volumes. An optional string value 
//    of "FactoryNameLink" allows a symbolic link to be added for FactoryName.
//
//    If "Parameters/Factory" has a nonempty string value named "DeviceLinkNamePrefix",
//    a default symbolic link name will be created for each auto-inserted volume lacking
//    an explicit symbolic link name.  The default symbolic link name will be the
//    concatenation of the prefix string, the port WWN, an underscore, and the node WWN.
//    Each WWN is represented as 16 hex digits.
//
//    If "Parameters/ExternalAutoInsertFactory0" is defined and is not "", it is a string value
//    which defines the auto-insert factory to use to create device objects above
//    each volume which will have BasicVolumeParams::UpperDeviceName, and the volume 
//    expose an intermediate name to this auto-inserted volume.  
//    If "Parameters/ExternalAutoInsertFactory{1,2,3}" are also specified, Factory0 will still
//    create the top device object connecting to the Factory1 object which will connect to the 
//    factory2 object and eventually to the Volume itself. 
//
//    "CMMPoolSize" ULONGLONG value contains the number of bytes to create the CMM pool
//    for this driver with, if the sub-class driver calls
//    BasicVolumeDriver::CppInitializeCmm(). If this value is not found, then the specific
//    driver default is used. If the resulting size is 0, then no CMM pool is created.
//
//    A subkey of "Trace" allows the override of the mapping of specific levels of tracing
//    to ktrace buffers.  Mapping to TRC_K_NONE turns off tracing for that level. Values
//    under this sub-key that may be specified are:
//       - ERROR, START, WARN, INFO, DebugTerse, DebugVerbose
//       - the DWORD value is the ktrace buffer number, e.g., TRC_K_STD=0, TRC_K_START=1,
//         TRC_K_NONE=0xff
//
//    "Trace\TraceName", if set, contains the string to prefix Trace messages with.  If
//    not set ...\CurrentControlSet\services\<Driver Name>\DisplayName is used.
//
//    A subkey of "NumIrpStackLocationsForVolumes" allows the override of the number of
//    IRP stack locations calculated by summing layered driver registry entries.
// 
//    A subkey of "SuppressDcaSupport" contains a bit mask of what DCA support bits should
//    be forced off, assuming VOID
//    BasicVolume::IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_CAPABILITIES_QUERY(PBasicIoRequest
//    Irp) is not overridden.  Default is 0.  If non-zero, the sub-class's setting for
//    this bit is ignored.
//
//
// The BasicVolumeDriver class allows creation of a VolumeMultiplexor, which supports
// volume-to-volume messages across two SPs where the volumes have the same WWNs, and it
// also allows for driver-to-driver messages between SPs.
//
// Possible Enhancements:
//   - Support configuration persistence by using KDBM, add an inter-SP configuration
//     transaction mechanism, and enhance the factory concept by supporting user space
//     IOCTLs with additional parameters.
//   
class BasicVolumeDriver : public BasicDriver, public BasicObjectTracer {
public:
    // Constructs a driver.  This is always called as part of construction of the
    // sub-class driver.  It initializes the trace mapping, saves away the name of the
    // driver to use for tracing.
    //
    // @param name - a descriptive name for this driver
    // @param traceMapping - the initial values for mapping of trace levels to KTrace ring
    //                       buffers.  Defaults to standard mapping if not specified.
    BasicVolumeDriver(const char * name,
        const BasicTraceLevelToRingMapping & traceMapping = BasicTraceLevelToRingMapping());

    virtual ~BasicVolumeDriver();

    // Returns the number of stack locations that are needed for sending IRPs to volumes
    // "below" this driver.  It is calcuated in DriverEntry() from the registry, by
    // summing the contents of HKLM/System\CCS\Services\ScsiTargConfig\StackLocations, or
    // which may be explicitly set by <Driver>\Parameters\NumIrpStackLocationsForVolumes.
    virtual CCHAR NumIrpStackLocationsForVolumes() const;

    // Returns the value of the registery parameter of the same name as read at driver
    // startup.
    ULONG SuppressDcaSupport() const  { return mSuppressDcaSupport; }

    // ::CppInitializeCmm () is required of all cppsh drivers. Typically subclasses of BVD
    // implement that function by simply calling this one.  A pool allocation causes "new"
    // calls to allocate from CMM rather than from WDM memory. Note that we call the cppsh
    // driver to allocate the CMM pool. The cppsh driver does not support us allocating
    // our own pool. So instead, we call the cppsh driver and it creates the CMM pool for
    // us.
    //
    // If <registryPath>\Parameters\CMMSize exists, it contains the number of bytes to
    // allocate for the pool, otherwise MemoryNeeded is used.  If the resuling size is 0,
    // no pool is allocated.
    //
    // Arguments: 
    // @param RegistryPath - the RegistryPath DriverEntry was called with.  Used to look
    //                       for Parameters\CMMSize.
    // @param MemoryNeeded - the number of bytes of memory to allocate for the pool, if
    //                       registry entry does not exist.
    // @param pLongTermClient - client of long term pool's name 
    // @param pPoolClient - client of pool we create for this driver.
    //
    // Returns:
    //     NT_SUCCESS() - the pool was created, or MemoryNeeded == 0 and the
    //     Parameters\CMMSize
    //            did not exist or was also 0.
    //
    //     otherwise - an error because the pool could not be created.
    //
    static EMCPAL_STATUS CppInitializeCmm(
        const PCHAR RegistryPath,     
        ULONGLONG MemoryNeeded,
        char *pLongTermClient,  char *pPoolClient);


    //   Called from ::DriverEntry routine to start up the driver.
    //
    //   @param rootKey - Parameter to ::DriverEntry.
    //   @param spid - SPID of this SP.
    //
    // Return Value:
    //   STATUS_SUCCESS - Driver was started. 
    //   Error - Driver cannot be started.
    EMCPAL_STATUS DriverEntry(DatabaseKey * Parameters, CMI_SP_ID spid, char* driverName);

    EMCPAL_STATUS DriverUnload();

    // Creates a volume based on the parameters, using the Volume sub-class specified by
    // the driver.  This will call the sub-class driver's SizeOfVolume() method, create
    // the device object using this size for the size of the DeviceExtension, then it will
    // call the sub-class driver's DeviceConstructor() method, using the device extention
    // as the memory for the volume object.
    //
    // @param volumeParams - A structure derived from BasicVolumeParams, describing the
    //                       initial device attributes.
    // @param volumeParamsLen - The number of bytes is the subclass of BasicVolumeParams.
    // @param howCreated - what caused this volume to initially be created?
    // @param UniqueVolumeIndex - a unique small integer used to index bitmaps.  This value
    //                            must not change over the lifetime of the volume, so it
    //                            must persist across reboot.
    //
    // The volume created is not yet initialized, and it cannot receive IRPs until
    // initialized. The volume is in the hash table, however, and therefore other
    // operations might be performed on it.
    //
    // Return Value:
    //   STATUS_SUCCESS - The device was created.
    //   Error - The device cannot be created.
    EMCPAL_STATUS CreateVolume(BasicVolumeParams * volumeParams, ULONG volumeParamsLen,
        BasicVolumeCreationMethod howCreated = BvdVolumeCreate_Unknown, VolumeIndex UniqueVolumeIndex = 0
        );


    // Destroys a volume
    //
    // @param key - the WWN of the volume to destroy.
    EMCPAL_STATUS DestroyVolume(const VolumeIdentifier & key);

    // There is a driver global spin lock used to protect various global data structures.
    // This function acquires that spin lock.
    VOID AcquireDriverLock() { mDriverLock.Acquire(); }

    // Releases the driver's spin lock.
    VOID ReleaseDriverLock() { mDriverLock.Release(); }

    // Returns the KTRACE ring buffer to use to display a particular trace level.
    // TRC_K_NONE is used to indicate no tracing.
    // @param level - the trace level the query is about.
    //
    // The sub-class may completely override this.
    virtual KTRACE_ring_id_T DetermineTraceBuffer(BasicTraceLevel level) const
        { return mTraceLevelMapping.DetermineTraceBuffer(level); }

    // Create a volume multiplexor to allow volume messages to be sent and received from
    // the peer.  Volume messages go between volumes that have the same WWN/Key.
    //
    // @param conduit_id - The CMID conduit ID to use.  Must not already be in use on this
    //                     SP.
    // @param mux - the VolumeMultiplexor created.
    //
    // Returns true on success, false on errors.
    bool CreateVolumeMultiplexor(CMI_CONDUIT_ID conduit_id, VolumeMultiplexor * & mux);

    // Panics the peer SP. It will be synchronous operation.
    //
    // @param mux - The VolumeMultiplexor on which we want to send the panic event.
    //
    // Return:
    //  NT_SUCCESS() - Successfully panicked the SP.
    //  !NT_SUCCESS() - Unable to panic the SP. 
    EMCPAL_STATUS PanicPeerSpSynchronous(VolumeMultiplexor * mux);
    
    // Panics the peer SP. It will be asynchronous operation and therefore doesn't 
    // wait for notification that the IOCTL was received by the specified SP.
    //
    // @param mux - The VolumeMultiplexor on which we want to send the panic event.
    //
    // Return:
    //  NT_SUCCESS() - Successfully panicked the SP.
    //  !NT_SUCCESS() - Unable to panic the SP. 
    EMCPAL_STATUS PanicPeerSpAsynchronous(VolumeMultiplexor * mux);
    
    // Find a volume by WWN.  The driver lock must be held before--and will be held
    // after--the call.
    //
    // @param key - the volume WWN
    //
    // Returns NULL if the volume is not found, otherwise returns the volume found.
    BasicVolume * FindVolumeDriverLockHeld(const VolumeIdentifier & key)
    {
        return mVolumeHash.Find(key);
    }

    // Find a volume by WWN.  If found, the volume is returned with the volume lock held,
    // ensuring that the volume cannot be destroyed.
    //
    // @param key - the volume WWN
    //
    // Returns NULL (with no lock) if the volume is not found, otherwise returns the
    // volume found, with the volume locked.
    BasicVolume * FindAndLockVolume(const VolumeIdentifier & key);

    // Find a volume by device name.  If found, the volume is returned with the volume lock held,
    // ensuring that the volume cannot be destroyed.
    //
    // @param deviceName - the volume upper device name
    //
    // Returns NULL (with no lock) if the volume is not found, otherwise returns the
    // volume found, with the volume locked.
    BasicVolume * FindAndLockVolume(const CHAR * deviceName);
    
    // Iterator to walk though the list of all volumes, when combined with
    // GetAndLockNextVolume().
    //
    // The standard sequence is:
    //    - vol = GetAndLockFirstVolume();
    //    - while (vol) {
    //    -    VolumeIdentifier key = vol->Key();
    //    -    ...
    //    -    vol->ReleaseSpinLock();
    //    -    vol = GetAndLockNextVolume(key);
    //    - }
    //        
    //
    // Returns NULL for no more volumes, otherwise the first volume of the iteration, with
    // the spin lock for that volume held.
    //
    // NOTE: The spin lock ensures that vol remains in existence.  If the caller releases
    // the spin lock, then the caller must ensure object existence.
    BasicVolume * GetAndLockFirstVolume();

    // Iterator to walk though the list of all volumes.  See GetAndLockFirstVolume().
    //
    // @param key - The last volume's key.
    //
    // Returns NULL for no more volumes, otherwise the next volume.
    BasicVolume * GetAndLockNextVolume(const VolumeIdentifier & key);

    // Specialized iterator to walk though the list of all volumes when 
    // caller already holds driver lock.
    //
    // The standard sequence is:
    //    - vol = GetFirstVolumeDriverLockHeld();
    //    - while (vol) {
    //    -    VolumeIdentifier key = vol->Key();
    //    -    ...
    //    -    vol = GetNextVolumeDriverLockHeld(key);
    //    - }
    //        
    //
    // Returns NULL for no more volumes, otherwise the first volume of the iteration.
    //
    BasicVolume * GetFirstVolumeDriverLockHeld() {return mVolumeHash.GetFirst();}

    // Specialized iterator to walk though the list of all volumes when 
    // caller already holds driver lock.
    // See GetFirstVolumeDriverLockHeld().
    //
    // @param key - The last volume's key.
    //
    // Returns NULL for no more volumes, otherwise the next volume.
    BasicVolume * GetNextVolumeDriverLockHeld(const VolumeIdentifier & key) {return mVolumeHash.GetNext(key);}

    //Accessor function for private variable.
    ULONG GetNumVolumesLockNotHeld(){return mNumVolumes;}

    // Returns the SP ID of this SP.
    const CMI_SP_ID & MySpId() const;

    // Returns the SP ID of the peer SP.
    const CMI_SP_ID & PeerSpId() const;

    // Called back when a message is received that is addressed to the driver itself,
    // rather than to some volume.
    //
    // The subclass with typically override this function if it communicates with other
    // SPs.
    //
    // @param mux - the mux the message arrived on  
    // @param volId - a 16 byte WWN sent from the other SP, whose meaning is dependent on
    //                the kind of inbound message.
    //
    // @param message - the message that just arrived
    //
    // Returns false if message service is complete and the message can be released, true
    // if the message will be released later.
    virtual bool MessageReceived(VolumeMultiplexor * mux, const VolumeIdentifier & volId, 
        InboundMessage * message) { return false; };

    // Called back when a malformed message is received from the peer, indicating a bug.
    // It is unclear whether the bug exists on this SP or the peer, so the default
    // implementation PANIC's.  The subclass might choose to override this behavior.
    //
    // @param mux - the mux the message arrived on
    // @param message - the message that just arrived
    //
    // Returns false if message service is complete and the message can be released, true
    // if the message will be released later.
    virtual bool CMIProtocolViolation_MessageReceived(VolumeMultiplexor * mux, InboundMessage * message)
    {
        FF_ASSERT(false);
        return false;
    }

    // A CMI message is arriving that has a fixed data portion, and CMID is asking for the
    // physical location into which to place the data, but the message is malformed or
    // destined for a non-existent volume, indicating a bug.  Since this is a virtual
    // function, it allows the sub-class to determine the policy.
    //
    // @param pDMAAddrEventData - see CmiUpperInterface.h
    virtual VOID CMIProtocolViolation_DMAAddrNeeded(
        PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA pDMAAddrEventData) const
    {
        // It would be possible for the sub-class to DMA the data into a bit-bucket, but
        // there is no mechanism to indicate to the normal message processing that that
        // the DMA was not done, so recovery from such a hypothetical bug would be
        // difficult.
        FF_ASSERT(false);
    }


    // Called when a message is received for a volume, but the volume was not found. The
    // subclass will typically override this function if it communicates with other SPs.
    //
    // The volume exists on the sender, but not on this SP.  In some drivers, this can
    // only be a bug.
    //
    // @param mux - the mux the message arrived on
    // @param volId - a 16 byte WWN sent from the other SP, whose meaning is dependent on
    //                the kind of inbound message.
    // @param message - the message that just arrived
    //
    // Returns false if message service is complete and the message can be released, true
    // if the message will be released later.
    virtual bool  MessageReceivedForNonExistantVolume(VolumeMultiplexor * mux, const VolumeIdentifier & vol,
        InboundMessage * message)
    {
        return false;
    }


    // The driver is notified of all contact lost events, as are all volumes. This
    // indicates that some SP other than the peer has lost contact. This function can be
    // overridden by the subclass driver.
    // @param mux - the mux that the contact lost is being reported on (one call per mux)
    // @param    delayedReleaseHandle - used later if "false" is returned.
    // @param    spId - The SP that failed or became unreachable. 
    //
    // Returns:
    //    false - delayed Release; don't allow reconnection until
    //            DelayedContactLostComplete() is called.
    //    true - ContactLost processing is complete.
    virtual bool  ContactLost(VolumeMultiplexor * mux, PCMI_SP_CONTACT_LOST_EVENT_DATA delayedReleaseHandle, CMI_SP_ID spId);




    // Create a Persistence factory device object.
    // @param FactoryName - the device object  to create.
    // @param MaxVolumes - the total number of volumes allowed in this driver.
    // @param MaxEdges - the total number of consumed volumes allowed in this driver.
    // @param priority - The auto-insert priority of this driver (realtive to other AI drivers)
    // @param FactoryLinkName - this parameter may be a NULL string.
    // Returns a success status if the factory is created, otherwise an error status is
    // returned.
    EMCPAL_STATUS CreatePersistenceFactory (
        const char * FactoryName, ULONG MaxVolumes, ULONG MaxEdges, AIDVD_PRIORITY priority,
        const char * FactoryDeviceLinkName, const char * DeviceLinkNamePrefix,
        bool notifyWhenInitializationCompletes,
        const char * DatabaseName = NULL, bool LoadDatabase = FALSE, ULONG KDBMBufferSize = 0,
        ULONG RecordSize = 0, ULONG DriverRecordSize = 0, ULONG CleanupDB = 0);

    // Serialize transaction Intent/Commit on this SP with single SP transactions.
    // This is acquired during the transaction validate phase, and released on commit.
    // SP local operations can also acquire/release this, but never acquire then start transaction.
    void TransactionSerializationSemaphoreAcquire() { mPerSpTransactionSemaphore.Acquire(); }

    void TransactionSerializationSemaphoreRelease() { mPerSpTransactionSemaphore.Release(); }
    
    // Returns NULL terminated ACSII string containing the name of the driver.
    const char * DriverName() const { return mDisplayName; }

    // Returns NULL terminated ACSII string containing the name of the driver to use for
    // trace statements.
    const char * TraceName() const { return mTraceName; }

    
    // If this is an auto-insert driver (specified by the registry), defines the order its
    // objects be inserted into the device stack.
    // Returns:  the priority to report as AIDVD_PRIORITY_NONE, unless overridden with the
    // registry value "AutoInsertPriority".
    virtual AIDVD_PRIORITY GetAutoInsertPriority() { return AIDVD_PRIORITY_NONE; }

    // If this is an auto-insert driver (specified by the registry), indicates
    // whether or not this driver receives a AidvdInitializationComplete()
    // call during boot.
    virtual bool RequiresInitializationNotification() { return false; }

    // If this is an auto-insert driver (specified by the registry), this callback
    // indicates that all volumes are created during the boot time sequence. 
    virtual void AidvdInitializationComplete() { return; }

    // Returns the WDM driver object for this driver.
    PEMCPAL_NATIVE_DRIVER_OBJECT  GetDriverObject() { return BasicDeviceObject::GetDriverObject(); }

    // Returns a reference to the K10SpinLock object for this driver.
    K10SpinLock & GetDriverLock() { return mDriverLock; }


      // All volumes are notified of all contact lost events, only then is the driver
      // itself notified. The volumes call AddContactLostHoldEvent() to indicate that they
      // wish to delay the release of the contact lost event until they are completely
      // cleaned up. This is for the purpose of preventing the failed SP from reconnecting
      // until the cleanup is done.
    //
    // This function can be overridden by the subclass driver, but the subclass driver
    // should call it.
    //
    // @param mux - the mux that the contact lost is being reported on (one call per mux)
    // @param delayedReleaseHandle - used later if "false" is returned.
    //
    // Return Value:
    //    false - delayed Release; don't allow reconnection until
    //            DelayedContactLostComplete() is called.
    //    true - ContactLost processing is complete.
    virtual bool PeerContactLost(VolumeMultiplexor * mux, PCMI_SP_CONTACT_LOST_EVENT_DATA delayedReleaseHandle);

    // Prevent any CONTACT LOST events from being acknowledged until a corresponding
    // number of calls to RemoveContactLostHoldEvent().  May be called at IRQL <=
    // DISPATCH_LEVEL with arbitrary locks held.
    // @param Count - the number to increment by
    virtual VOID AddContactLostHoldEvent(ULONG Count = 1);

    // Undoes a corresponding number of calls to AddContactLostHoldEvent(). Must not be
    // called with any locks held.
    // @param Count - the number to decrement by
    virtual VOID RemoveContactLostHoldEvent(ULONG Count = 1);

    //Get the current transaction number of persistence factory
    ULONG GetCurrentTransactionNumber();
    
    // Get the current committed revision number from the persistent factory.
    ULONG GetCurrentCommittedRevisionNumber();
    
    // Get the maximum supported revision number. This method can be overridden by a sub-class driver
    // that wants to report a different maximum supported revision number.
    virtual UINT_32 GetMaximumSupportedRevisionNumber() const;
    
    // Called during startup, passing in the persisted driver parameters, or zeros if the database was
    // just created.
    // @param bvdParam - the driver parameters read from the database.
    // @param paramLen - the number of bytes in the database record.  Bytes never written should be 0.
    // Returns: success if the parameters are acceptable, or an error if the parameters are sufficiently
    //          corrupt to reject the database.
    EMCPAL_STATUS InitializeDriverParam(BasicVolumeDriverParams * bvdParam, ULONG paramLen);

    EMCPAL_STATUS SetDriverParam(BasicVolumeDriverParams * bvdParam, ULONG paramLen);

    EMCPAL_STATUS GetDriverParam(BasicVolumeDriverParams * bvdParam, ULONG paramLen);
       

    // Asynchronously services the IRP.
    // @param Irp - a Query DSriverStatus IRP
    VOID     QueryDriverStatus(PBasicIoRequest  Irp);


    EMCPAL_STATUS QueryVolumeStatus(VolumeIdentifier * Key,
                               BasicVolumeStatus * ReturnedVolumeStatus,
                               ULONG MaxVolumeParamsLen, ULONG & OutputLen);

    EMCPAL_STATUS GetDriverStatistics(BasicVolumeDriverStatistics * statistics, ULONG MaxOutputLength,
                         ULONG & OutputLength);

    EMCPAL_STATUS GetVolumeStatistics(VolumeIdentifier * Key,
                                 BasicVolumeStatistics * ReturnedVolumeStatistics,
                                 ULONG MaxVolumeStatisticsLen, ULONG & OutputLen);
	// Clear statistics for the particular volume.
    //
    // @param key - Desired volume key.
    // @param MaxVolumeStatisticsLen - length of volume statistics struct.
    //
    // Returns: success if operation completed successfully otherwise error.
    EMCPAL_STATUS ClearVolumeStatistics(VolumeIdentifier * Key,                                 
                                 ULONG MaxVolumeStatisticsLen);

    // Clear statistics for all the volumes.
    //
    // Returns: success if operation completed successfully otherwise error.
    EMCPAL_STATUS ClearAllVolumeStatistics();


    // Returns WWN of all volumes which where changed after LastVolumeChangeCount in the output buffer.
    //
    // @param LastVolumeChangeCount - limits the WWNs returned to those > this value.
    // @param resp - where the list of WWNs is placed.
    // @param MaxOutputLength - the number of bytes resp is.
    // @param OutputLength - the number of bytes actually returned
    EMCPAL_STATUS EnumVolumes(ULONGLONG LastVolumeChangeCount, BasicVolumeEnumVolumesResp * resp, 
                                 ULONG MaxOutputLength, ULONG & OutputLength);

    EMCPAL_STATUS QueryVolumeParams(VolumeIdentifier * Key,
                               BasicVolumeGetVolumeParamsResp * resp,
                               ULONG MaxVolumeParamsLen, ULONG & OutputLen);

    // Verify that the driver action params are acceptable to BVD. Calls
    // EventValidateVolumeParamsForCreate() which the sub-class can override.
    // @param IsInitialingSP - is this the SP on which the transaction arrived
    // @param DriverAction - a sub-class of BasicDriverAction, defined for the specific
    //                       driver.
    // @param newLen - the number of bytes in the subclass of BasicDriverAction
    // Returns: true if the parameters are acceptable, false otherwise.
    bool DriverActionValidate(BOOLEAN IsInitialingSP, const BasicDriverAction * DriverAction, ULONG Len);

    // Routine to handle PERFORM_DRIVER_ACTION IOCTL Takes the action specified on the
    // driver. Calls EventPerformDriverAction() which the sub-class can
    // override.
    //
    // WARNING: This call occurs prior to the persistence of any parameters, allowing this function
    // to indicate that persistence is required.  Care should be taken to ensure externally 
    // visible actions occur in UpdateDriverParamsReleaseLock rather than here, because if there is a crash,
    // the transaction may abort, and the driver params will revert to that prior to the transaction.
    //
    // @param IsInitialingSP - is this the SP on which the transaction arrived
    // @param ActionBuf - a sub-class of BasicDriverAction, defined for the specific
    //                    driver.
    // @param Len - The number of bytes passing in for the subclass of ActionBuf
    // @params newDriverParams - A buffer of the size to hold current state of Driver params.
    //                           The buffer must be updated with the desired parameters.
    //
    // Returns: The number of bytes to write to the database.
    ULONG PerformDriverAction( BOOLEAN IsInitialingSP, BasicDriverAction * ActionBuf, ULONG Len, BasicVolumeDriverParams * newDriverParams);

    // Allow the BVD to perform any cleanup needed after the Driver Action transaction is done.
    // Calls EventDriverActionCleanup() which the sub-class can override.
    VOID PerformDriverActionCleanup();

    // Renames a volume's WWN key.
    // @param params - defines the OldWwn and NewWwns
    // @param newLen - the number of bytes in the subclass of basic volume params.
    VOID ChangeLunWwn(BasicVolumeWwnChange * params, ULONG ParamLen);

    // Verify that the volume params are acceptable to BVD for volume creation. Calls
    // EventValidateVolumeParamsForCreate() which the sub-class can override.
    // @param newVolumeParams - a sub-class of BasicVolumeParams, defined for the specific
    //                          driver.
    // @param newLen - the number of bytes in the subclass of basic volume params. 
    bool ValidateVolumeParamsForCreate(const BasicVolumeParams * newVolumeParams, ULONG newLen);

    // Verify that the commit level is acceptable to BVD. Calls
    // EventValidateDriverCommitLevel() which the sub-class can override.
    // @param DriverCommit - a sub-class of BasicDriverCommitParams, defined for the specific
    //                       driver.
    // @param newLen - the number of bytes in the subclass of BasicDriverCommitParams
    // Returns: true if the parameters are acceptable, false otherwise.
    bool PerformDriverRevisionValidate(const BasicVolumeDriverCommitParams * DriverCommit, ULONG Len);
    
    // Allow the BVD to perform any activity needed before the commit of the current driver revision.
    // Calls EventPerformDriverRevisionPreCommit() which the sub-class can override.
    // @param DriverCommit - a sub-class of BasicDriverCommitParams, defined for the specific
    //                       driver.
    // @param newLen - the number of bytes in the subclass of BasicDriverCommitParams
    //
    void PerformDriverRevisionPreCommit(const BasicVolumeDriverCommitParams * DriverCommit, ULONG Len);
        
    // Allow the BVD to perform any activity needed after the commit of the current driver revision.
    // Calls EventPerformDriverRevisionPostCommit() which the sub-class can override.
    // @param DriverCommit - a sub-class of BasicDriverCommitParams, defined for the specific
    //                       driver.
    // @param newLen - the number of bytes in the subclass of BasicDriverCommitParams
    //
    void PerformDriverRevisionPostCommit(const BasicVolumeDriverCommitParams * DriverCommit, ULONG Len);

    // Allow the BVD to perform any cleanup needed after the commit of the current driver revision.
    // Calls EventPerformDriverCommitCleanup() which the sub-class can override.
    void PerformDriverCommitCleanup();
        
    virtual CrashDisposition CrashCoordinatorCallback(PNotificationInformation pNotificationInfo);

    static CrashDisposition StaticCrashCoordinatorCallback(PNotificationInformation pNotificationInfo);

    // Basic volume registers for pending crash notification, and may recommend a change
    // in behavior.
    CrashDisposition NotifyOfImpendingCrash(PNotificationInformation pNotificationInfo);

    // Returns the count which is incremented every time any volume params, driver params,
    // volume status, or driver status data changes.
    ULONGLONG  GetDriverChangedCount() { return mDriverChangeCount; };
    

    // Indicate that params or status in the driver has changed.
    ULONGLONG  IncrementDriverChangedCountReleaseLock()
    {

        ULONGLONG result = ++mDriverChangeCount;

        ListOfBasicIoRequest    irps;
        
        // Move all the IRPs off the list first, to avoid cases where the
        // list is popluated as its being drained. 
        for(;;) {
            BasicIoRequest * irp = mWaitingForDriverChangeCount.RemoveHead();

            if (!irp) break;

            irps.AddTail(irp);
        }
        
        mDriverLock.Release();

        for(;;) {
            BasicIoRequest * irp = irps.RemoveHead();

            if (!irp) break;

            QueryDriverStatus(irp);
        }
       
        return result;
    }
    // Returns true if this SP is SP-A, false if SP B.
    bool  ThisSPisA() const { 
        // If the peer is SP B (odd) then we are A.
        return (mPeerSpId.engine & 1) != 0; 
    }

    // Request that an external Auto-insert factory create a device.
    // @param factoryName - the name of the AI factory device object
    // @param wwn - the LunWnn to ask that factgory to create.  This is a unique key.
    // @param DeviceName - the name to expose
    // @param LinkName - the alternative device name to expose
    // @param LowerDeviceName - The device name to consume
    EMCPAL_STATUS ExternalFactoryAutoInsertCreate(
        const CHAR * factoryName, const K10_WWID & wwn,
        const CHAR * DeviceName, const CHAR * LinkName, const CHAR * LowerDeviceName);


    // Undo a create operation.
    // @param factoryName - the name of the AI factory device object
    // @param wwn - the LunWnn to ask that factgory to create.  This is a unique key.
    EMCPAL_STATUS ExternalFactoryAutoInsertDestroy(
        const CHAR * factoryName, const K10_WWID & wwn);

    // Called when an IOCTL_BVD_SHUTDOWN_WARNING is received
    // from user space (normally admin).  This is the first step
    // in Admin's controlled shutdown/restart processing.  From this function 
    // a derived driver is called via its EventValidateShutdown() method
    // if it chooses to have one (not necessary if subclass has no real needs at
    // this point).  The shutdown warning can be rejected with bad status by
    // the driver at this point.
    EMCPAL_STATUS AdminShutdownWarning();

    // Called when an IOCTL_BVD_SHUTDOWN is received from user space (normally admin).
    // This routine will set the SP local mbShutdownComing to true which will prevent 
    // configuration changes from occurring.  This function will call a derived driver's
    // EventShutdown() method if it has one (otherwise the BVD default does nothing.
    // At this time the normal shutdown processing is going to do everything it can to get 
    // shutdown so attempting to reject this with bad status is fruitless. 
    EMCPAL_STATUS AdminShutdown();

    // Causes most IOCTL's to be rejected when it returns true. Should be redefined only
    // if some clean-up needs to be done at shutdown time before that happens.
    virtual bool IsShuttingDown() const {return mbShutdownComing;}

protected:

    // Notifies the sub-class driver that the BasicVolumeDriver is about to construct
    // volumes based on registry parameters.
    //
    // @param parameters - an open registry key starting at
    //                     ...\CurrentControlSet\services\<DriverName>\Parameters
    //
    // The sub-class driver must override if it is interested in this notification.
    virtual EMCPAL_STATUS DriverEntryBeforeConstructingDevices(DatabaseKey * parameters) {
        return EMCPAL_STATUS_SUCCESS;
    }

    // Notifies the sub-class driver that the BasicVolumeDriver has constructed and
    // initialized volumes.
    //
    // @param parameters - an open registry key starting at
    //                     ...\CurrentControlSet\services\<DriverName>\Parameters
    //
    // The sub-class driver must override if it is interested in this notification.
    virtual EMCPAL_STATUS DriverEntryAfterConstructingDevices(DatabaseKey * parameters) {
        return EMCPAL_STATUS_SUCCESS;
    }

    // Allow sub-class to insert code during Driver Unload, before volumes are
    // automatically destroyed. The sub-class driver must override if it is interested.
    virtual void AboutToUnload() {}

    // Deletes all device objects.  Should only be called during Unload() or
    // DriverEntry().
    VOID DeleteDevices();

    // Returns the amount of memory needed to hold an instance of the volume class.
    virtual ULONG SizeOfVolume() const = 0;





    // Every driver must implement  DeviceConstructor()
    //
    // @param volumeParams - the parameter of the volume, to be passed into the
    //                       BasicVolume constructor. ValidateVolumeParams() has already
    //                       been called and returned true.
    // @param volumeParamsLen - the number of bytes that were specified for volumeParams.
    // @param memoryForVolume - this contains SizeOfVolume(), and is where a
    //                          PassThruVolume should be constructed.
    virtual BasicVolume *  DeviceConstructor(BasicVolumeParams * volumeParams, 
        ULONG volumeParamsLen, VOID * memory) = 0;


    // Given a registry key, allow the subclass driver to fill in the values in the volume
    // params structure.  The default function will handle standard filter driver registry
    // entries if SizeOfVolumeParams() is overridden with a size large enough to
    // accommidate FilterDriverVolumeParams.
    virtual bool FillInVolumeParametersFromRegistry(DatabaseKey * deviceParams, 
        BasicVolumeParams * params);


public:

    // If we have external factories, should intermediate AI volumes have link names?
    virtual bool SpecifyDeviceLinkNamesForIntermediateAutoInsertVolumes() const { return false; }

    // Derived driver will overload this when it needs to know when its
    // configuraiton has been loaded.  Useful for hybrid auto-insert drivers.
    virtual bool EventDatabaseLoadNotification() { return true; }

    // Derived driver will overload this when it needs to know when its
    // configuration load has failed.  No-op otherwise.
    virtual void EventDatabaseLoadFailed() {  }

    // Called for each legal volume index during database creation.  Gives the driver a
    // chance to create a record for volumes it knows about by other means.
    // @param volIndex     - a number from 0-maxVolumes, indicating the volume 
    //                       index that this record would be created for.
    // @param volumeParams - the base class of the parameters to set if desired.
    // @param volumeParamsLen - the number of bytes of data provided in volumeParams
    // Returns: true - volumeParams were modified to describe the record to create.
    // false - no volume should be created for this volume index.
    virtual bool EventDatabaseCreation_DescribeLegacyVolume(VolumeIndex volIndex, 
        BasicVolumeParams *  volumeParams, UINT_32 volumeParamsLen) { return false;}

    // Determine if the volume parameters passed in are valid for the subclass of
    // BasicVolumeParams. The base class is assumed to be validated.
    // @param newVolumeParams - the base class of the parameters to validate.
    // @param newLen - the number of bytes of data provided that need to be validated.
    // Returns: true if the sub-class is valid, false otherwise.
    virtual bool EventValidateVolumeParamsForCreate(const BasicVolumeParams * newVolumeParams, ULONG newLen) const;

    // Determine if the volume driver parameters passed in are valid for the subclass of
    // BasicVolumeDriverParams. The base class is assumed to be validated.
    // @param newVolumeDriverParams - the base class of the parameters to validate.
    // @param newLen - the number of bytes of data provided that need to be validated.
    // Returns: true if the sub-class is valid, false otherwise.
    // FIX: should be const, but FCT won't compile, see AR425284
    virtual bool EventValidateDriverParamsForUpdate(const BasicVolumeDriverParams * newVolumeDriverParams, ULONG newLen);
    
    // Determine if the commit level is acceptable for the subclass of 
    // BasicVolumeDriverCommitParams. The base class is assumed to be validated.
    // @param newCommitParams - the base class of the commit parameters.
    // @param newLen - the number of buytes of the data provided that need to be validated.
    // Returns: true if the sub-class is valid, false otherwise.
    virtual bool EventValidateDriverCommitLevel(const BasicVolumeDriverCommitParams * newCommitParams, ULONG newLen)
    {
        return true;
    }
        
    // Perform any actions necessary before the new revision level is committed. The
    // sub-class should override this method if necessary.
    // @param newCommitParams - the base class of the commit parameters.
    // @param newLen - the number of buytes of the data provided that need to be validated.
    virtual void EventPerformDriverRevisionPreCommit(const BasicVolumeDriverCommitParams * newCommitParams, ULONG newLen)
    {
        return;
    }
    
    // Perform any actions necessary after the new revision level has been committed.
    // @param newCommitParams - the base class of the commit parameters.
    // @param newLen - the number of buytes of the data provided that need to be validated.
    // Returns: true if the sub-class is valid, false otherwise.
    virtual void EventPerformDriverRevisionPostCommit(const BasicVolumeDriverCommitParams * newCommitParams, ULONG newLen)
    {
        return;
    }

    // Perform any necessary cleanup after the new revision level has been committed.
    virtual void EventPerformDriverCommitCleanup()
    { }

    // Determine if the volume driver actions passed in are valid for the subclass of
    // BasicVolumeDriverAction. The base class is assumed to be validated.
    // @param DriverAction - the base class of the actions to validate.
    // @param newLen - the number of bytes of data provided that need to be validated.
    // Returns: true if the sub-class is valid, false otherwise.   Default if not overridden:
    // attempts to set actions are rejected.
    virtual bool EventDriverActionValidate(const BasicDriverAction * DriverAction, ULONG newLen)
    {
         return false;
    }

    // Routine to handle specific driver parts of the PERFORM_DRIVER_ACTION IOCTL.
    // Takes the action specified on the driver.
    //
    // WARNING: This call occurs prior to the persistence of any parameters, allowing this function
    // to indicate that persistence is required.  Care should be taken to ensure externally 
    // visible actions occur in EventDriverParamsUpdatedReleaseLock rather than here, because if there is a crash,
    // the transaction may abort, and the driver params will revert to that prior to the transaction.
    //
    // @param DriverAction - a sub-class of BasicDriverAction, defined for the specific
    //                    driver.
    // @param Len - The number of bytes passing in for the subclass of DriverAction
    // @params newDriverParams - A copy of the existing driver parameters on the original
    //                           call.  May be modified if the DriverAction specifies all SPs,
    //                           and all SPs must do the exact same modification.
    //                           If modified, the persistent parameters will be persisted,                   
    //                           EventDriverParamsUpdatedReleaseLock will be called if the 
    //                           parameters were updated.
    //
    virtual void EventPerformDriverAction(BasicDriverAction * DriverAction, ULONG Len, BasicVolumeDriverParams * newVolumeDriverParams)
    {
         //Derived class shall override this function and perform Driver specific
         //operation.
    }

    // Perform any necessary cleanup after the Driver Action transaction is done.
    virtual void EventDriverActionCleanup()
    {
         return;
    }

    // The derived driver will fill in addtional driver status through this function.
    // @param status - a pointer to the base class which can be static_cast to the specific driver's
    //                 status structure.   The caller will fill in any base class status, the specific
    //                 driver must fill in any driver specific status.
    // @param MaxOutputLength - the number of bytes allocated for status.  No more than this many bytes should be written.
    // @param OutputLength    - the size of the driver specific structure.  Must be <= MaxOutputLength.
    // Returns: Success if the structure was of an adequate size, false otherwise.
    virtual EMCPAL_STATUS EventQueryDriverStatusReleaseLock(BasicVolumeDriverStatus * status, ULONG MaxOutputLength,
                                                       ULONG & OutputLength) {
        ReleaseDriverLock(); 
        return EMCPAL_STATUS_SUCCESS; 
    };

    // The derived driver will fill in addtional driver statistics through this function.
    // @param status - a pointer to the base class which can be static_cast to the specific driver's
    //                 stats structure.   The caller will fill in any base class stats, the specific
    //                 driver must fill in any driver specific status.
    // @param MaxOutputLength - the number of bytes allocated for stats.  No more than this many bytes should be written.
    // @param OutputLength    - the size of the driver specific structure.  Must be <= MaxOutputLength.
    // Returns: Success if the structure was of an adequate size, false otherwise.
   virtual EMCPAL_STATUS EventGetDriverStatisticsReleaseLock(BasicVolumeDriverStatistics * stats, ULONG MaxOutputLength,
                                                         ULONG & OutputLength) {
        ReleaseDriverLock(); 
        return EMCPAL_STATUS_SUCCESS; 
    };

    // Called during startup, passing in the persisted driver parameters, or zeros if the database was
    // just created.
    // @param bvdParam - the driver parameters read from the database.  The callee may modify these,
    //                   causing them to reported out on a GET_DRIVER_PARAMS, but this should only
    //                   be done if the information appeared never to have been set w/ a 
    //                   SET_VOLUME_PARAMS (the initial parameters would be all zeros).
    // @param paramLen - the number of bytes in the database record.  Bytes never written should be 0.
    virtual void EventDriverParamsInitializeReleaseLock( BasicVolumeDriverParams * bvdParam, ULONG paramLen) {
        ReleaseDriverLock();
        return;
    }
                                                     
    // Called during startup to indicate initialize transaction is done.  May be overidden by derived
    // driver if it has some other thing(s) to do while still holding the inter-SP lock.
    virtual void EventDatabaseLoadedTransactionLockHeld( ) {
        return;
    }
                                                     
    // Called after the new driver parameter have been persisted.
    // @param bvdParam - the driver parameters just written to the database. The callee may modify these,
    //                   causing them to reported out on a GET_DRIVER_PARAMS, but this should only
    //                   be done if the information appeared never to have been set w/ a 
    //                   SET_VOLUME_PARAMS (the initial parameters would be all zeros).
    // @param paramLen - the number of bytes in the database record. 
    virtual void EventDriverParamsUpdatedReleaseLock(BasicVolumeDriverParams * bvdParam, ULONG paramLen) {
        ReleaseDriverLock();
        return;
    }


    // Convert the parameters to the same format as the last committed revision.
    virtual void EventDownConvertDriverParams(BasicVolumeDriverParams * bvdParam, ULONG newParamLen, ULONG ondiskParamLen)
    {
        //default behavior is do nothing. Do not even truncate it, since BVD will only
        //write ondiskParamLen into disk
    }

    // Convert the parameters to the same format as the currently running revision.
    virtual void EventUpConvertDriverParams(BasicVolumeDriverParams * bvdParam, ULONG newParamLen, ULONG ondiskParamLen)
    {
        //default behavior is do nothing
    }

    // Convert the parameters to the same format as the last committed revision.
    virtual void EventDownConvertVolumeParams(BasicVolumeParams * bvdParam, ULONG newParamLen, ULONG ondiskParamLen)
    {
        //default behavior is do nothing. Do not even truncate it, since BVD will only write ondiskParamLen into disk
    }

    // Convert the parameters to the same format as the currently running revision.
    virtual void EventUpConvertVolumeParams(BasicVolumeParams * bvdParam, ULONG newParamLen, ULONG ondiskParamLen)
    {
        //default behavior is do nothing
    }

    // Derived driver can overload this if it wants to do anything special 
    // at shutdown warning time.
    virtual bool EventValidateShutdown()
    {
        return true;  //BVD is always happy to shutdown :-)
    }

    // Derived driver should overload this function if it has specific internal
    // things it want to do at shutdown time
    virtual void EventShutdown()
    {
        // nothing by default
    }

    virtual ULONG GetDriverStatusSize() const
    {
         return sizeof(BasicVolumeDriverStatus);
    }

protected:
    virtual void CopyDriverStatus(BasicVolumeDriverStatus& status) const;
    
private:
    EMCPAL_STATUS ConsiderCreatePersistenceFactory(DatabaseKey * Parameters);
    EMCPAL_STATUS ConsiderVolumeCreationFromRegistry(DatabaseKey * Parameters);

    // Fills in a DriverStatus structure.
    EMCPAL_STATUS QueryDriverStatusReleaseLock(BasicVolumeDriverStatus * status, ULONG MaxOutputLength,
                               ULONG & OutputLength);

public:

    // Default Handler for IOCTLs
    virtual void IRP_MJ_DEVICE_CONTROL_Other(PBasicIoRequest pIrp) 
    {
        pIrp->CompleteRequest(EMCPAL_STATUS_NOT_IMPLEMENTED, 0);
        return;
    };

    // Default handler for IOCTLs when Database is NOT loaded
    virtual void IRP_MJ_DEVICE_CONTROL_OtherNoDB(PBasicIoRequest pIrp) 
    {
        pIrp->CompleteRequest(EMCPAL_STATUS_NOT_IMPLEMENTED, 0);
        return;
    };

    // A BasicDeviceObject has the capability to queue arriving IRP to a thread, so the IRP is
    // serviced in the context of that thread.  See BasicDeviceObjectIrpDispatcherThread.
    // If the device queues the IRP to a BasicDeviceObjectIrpDispatcherThread in the
    // DispatchMethods above, the thread will call this function to do the real IRP
    // dispatch.
    //  Therefore subclasses that queue IRPs to threads must implement this method.
    //     
    // @param Irp - the IRP to dispatch
    virtual VOID DispatchQueuedIRP(PBasicIoRequest Irp) { FF_ASSERT(false); }

    // Call into the sub-class to determine how large the storage record for
    // BasicVolumeParams is.  Defaults to no sub-class.  Sub-classes that are smaller than
    // this will have persistent records padded out with zeros.
    virtual ULONG SizeOfVolumeParams() const { return sizeof(BasicVolumeParams); }

    virtual ULONG SizeOfVolumeDriverParams() const { return sizeof(BasicVolumeDriverParams); }


    // Get readable access to parameters.
    const  BasicVolumeDriverParams *  GetDriverParams() const { return mBVDDriverParam; }

    BasicVolume * GetFirstDeletedVolumeDriverLockHeld(){return mRecentlyDeletedVolumes.head;}
    BasicVolume * GetNextDeletedVolumeDriverLockHeld(BasicVolume * pVol) {return pVol->mHashNext;}

    ULONG     GetDeletedVolumeCount() {return mNumDeletedVolumes;}

    // Derived driver will override this function if it needs to keep some
    // number of recently deleted volumes.
    virtual  ULONG GetMaxDeletedVolumeCount() {return 0;}

    // In some cases it is advantageous to clear out the deleted volumes list.
    // This routine moves all the recently deleted volumes to the volume free list.
    void PurgeDeletedVolumes();

    ULONGLONG GetDeleteChangeCountLimit() {return mChangeCountBelowWhichEnumVolumesWouldBeUncertain;}
    void SetChangeCountLimit(ULONGLONG newLimit) {mChangeCountBelowWhichEnumVolumesWouldBeUncertain = newLimit;}
    
    // This value leaves room for non-configured volumes.  It also is the bias
    // for the UniqueVolumeIndex, so changing this value will change the
    // result of BasicVolume::GetVolumeIndex(), which can have NDU implications.
    // MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS (core_config.h) is at least
    // (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST + 1)
    virtual ULONG MaxInternalVolumes() const { return MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS; }

    // The logical # of volumes this driver supports, excluding MaxInternalVolumes().
    ULONG  MaxConfiguredVolumes() const { return mMaxConfiguredVolumes; }

    ULONG MaxTotalVolumes() const { return MaxConfiguredVolumes() + MaxInternalVolumes(); }
    ULONG MaxVolumeIndex() const { return MaxConfiguredVolumes() + MaxInternalVolumes(); }

    enum Consts { MaxExternalFactories = 3 };

    // Returns the name of the auto-insert factory at the specified index.
    // Indices start at 0, and are tightly packed.  A NULL return indicates 
    // that there is no factory at that index or above.
    virtual PCHAR ExternalFactoryName(ULONG index) { 
        if(index >= MaxExternalFactories) return NULL;
        return mExternalFactories[index];
    }

    // Internal logging function, could be overridden by sub-class driver.
    //
    // The default implementation prefixes the standard printf args with the driver name.
    //
    // @param ring - the ktrace ring buffer to log to.
    // @param format - the printf format string.
    // @param args - the format arguments.
    virtual VOID TraceRing(KTRACE_ring_id_T ring, const char * format, va_list args) const __attribute__ ((format(__printf_func__, 3, 0)));

private:

    enum                                        Constants { MaxDisplayName = 40 };

    // The driver name in ASCII for debug displays.
    char                                        mDisplayName[MaxDisplayName];

    // The driver name in ASCII for tracing displays.
    char                                        mTraceName[MaxDisplayName];


    // The current collections of volumes that exist on this SP.
    HashListOfBasicVolume                       mVolumeHash;

    // The number of Volumes currently in the hash table.
    ULONG                                       mNumVolumes;


     // Volumes that were deleted, which have been preserved for delta-poll purposes.
    FreeListOfBasicVolume                       mRecentlyDeletedVolumes;

    // The number of Volumes deleted and currently preserved for delta-poll purpose.
    ULONG                                       mNumDeletedVolumes;

    // Deleted volumes look-aside list.  This list contains Volumes for re-use.
    // Once a volume is allocated, it remains a volume, until the driver exits.
    // This allows some fields to remain valid, so that code with pointers
    // to the volume may continue to run.
    FreeListOfBasicVolume                       mFreeVolumeLookaside;

    // The SP ID of this SP.
    CMI_SP_ID                                   mMySpId;

    // The SP ID of the peer SP.
    CMI_SP_ID                                   mPeerSpId;

    // Protects mContactLostCleanupCount and mContactLostEventsPending.
    K10SpinLock                                 mContactLostSpinLock;

    // The number of things that need to be cleaned up before any contact lost events can
    // be completed.
    ULONG                                       mContactLostCleanupCount;

    // The contact lost events that need to be completed when the CLeanupCOunt reaches 0.
    ListOfCMI_SP_CONTACT_LOST_EVENT_DATA        mContactLostEventsPending;

        
    // The IRP stack size required by IRPs allocated by this driver for sending down the
    // device stack.
    ULONG                                       mNumIrpStackLocationsForVolumes;

    // Contents of registry entry.
    ULONG                                       mSuppressDcaSupport;


    // The maximum number of volumes that are supported for this driver.
    ULONG                                       mMaxConfiguredVolumes;

    ULONG                                       mMaxEdges;
    
    // Protects the drivers internal data structures.
    K10SpinLock                                 mDriverLock;

    // The current auto-insert/persistence factory, if any.
    class                                       PersistenceVolumeFactory * mPersistFactory; 

    //Driver record size in KDBM
    ULONG                                       mBVDDriverRecordSize;
    
    //Volume record size in KDBM
    ULONG                                       mBVDVolumeRecordSize;
    
    struct                                      BasicVolumeDriverParams * mBVDDriverParam;

    // Acquired to serialize Intent/Commit phase of transaction with other operations.
    BasicBinarySemaphore                 mPerSpTransactionSemaphore;

    //SP local change counter of driver to keep track of the changes in drivervolume.
    // Signed s'o InterlockedIncrement64 can be called.
    LONGLONG                                  mDriverChangeCount;  

    // Holds requests that are waiting for a change of mDriverChangeCount. 
    LockedListOfCancelableBasicIoRequest       mWaitingForDriverChangeCount;

    // This is the newest change count at which enum volumes might not report deleted volumes.
    ULONGLONG                                mChangeCountBelowWhichEnumVolumesWouldBeUncertain;  

    // Contains strings, which need to be freed on destruction, contain the 
    // device object names of the auso-insert factories.
    CHAR *                                    mExternalFactories[MaxExternalFactories];

    // Set to indicate we've received a shutdown command from Admin
    // Used to prevent config changes from other processes that haven't 
    // gotten the message yet
    bool                                        mbShutdownComing;

public:

    // Controls which trace levels are routed to which KTRACE ring buffers.
    BasicTraceLevelToRingMapping                mTraceLevelMapping;

};


#endif  // __BasicVolumeDriver__

