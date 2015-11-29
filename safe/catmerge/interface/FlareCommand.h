// FlareCommand.h: interface for the FlareCommand class
//
// Copyright (C) 2001-2011   EMC Corporation
//
//
// The FlareCommand class encapsulates the CamaX Flare IOCTLs used directly by admin
// The use of implementation class (FlareCommandImpl) helps avoid some header and
// class hierarcy issues.
//
//
//  Revision History
//  ----------------
//  27 Jul 01   H. Weiner   Initial version.
//  12 Oct 01   H. Weiner   Fix name of MemoryConfig, add init method for ADM_SYSTEM_CONFIG_CMD
//  17 Oct 01   H. Weiner   Define AdmMemoryConfig, so caller can refer to SP via WWN
//  24 Oct 01   H. Weiner   IOCTLs include a sunburst error code in the data they return
//   8 Nov 01   H. Weiner   Add BackDoorDiskPower(), ZeroDisk, RG ref by WWN in bind
//  26 Nov 01   H. Weiner   Add ClearCacheDirtyLun()
//  20 Dec 01   H. Weiner   Revised Send/Receive Diagnostic
//   4 Jan 02   H. Weiner   ZeroDisk() now uses disk key.
//  14 Jan 02   CJ Hughes   replace ADM_SEND_RCV_DIAG_FLARE_PARAM with
//                          ADM_RCV_DIAG_FLARE_PARAM
//  23 Jan 02   H. Weiner   Add DiskScsiPassThruWrite/Read
//   1 Feb 02   H. Weiner   Add GetSpId()
//  10 Apr 02   H. Weiner   DIMS 70381: Support stopping Zeroing of a disk.
//  25 Jul 02   MSZ         Add nondestructive bind functionality
//  11 Mar 03   CHH         Added change bind params only support
//  15 Dec 04   PEC         Added GroupVerify and SystemVerify methods to FlareCommand & FlareCommandImpl
//  27 Jun 05   KcM         DIMS 122008 - Port raid type support from Piranha to Release 22 Hammer.
//	12 Jul 05	REH			DIMS 126137: Change BackDoorDiskPower() power_on parameter 
//							from BOOL to ADM_FRU_POWER
//	03 Oct 05	M. Brundage	DIMS 132479: Added Reboot interface.
//	16 Nov 05	R. Hicks	Added elements_per_parity_stripe to AdmLunBindData
//	16 Jan 06	P. Caldwell	Added Proactive Spare support.
//	30 Mar 06	K. Boland	Added support for FixLUN command
//	03 Apr 06	P. Calewell	Added code for FastBind
//	05 Apr 06	P. Caldwell	Added support for LCC Stats.
//	18 Apr 06	P. Caldwell	Added FRUMON Fault Insertion support.
//	26 Oct 06	P. Caldwell	Added support for VBus Stats.
//	29 Jan 07	H. Weiner	Added support for setting the Flare DB drive queue depth to a specific value.
//	14 Aug 07	D. Hesi		Added support for Flexport Stats
//	30 Sep 07	H. Rajput	Modifications to incorporate Management Port Speeds Settings
//	29 Oct 07	R. Hicks	Add support for internal LUNs and Raid Groups
//	01 Nov 07	D. Hesi		DIMS 182342: Added support for I/O Module Port Marking
//  06 Dec 07   R. Proulx   DIMS 183595: Changed rebuild_time to rebuild_priority in AdmLunBindData structure
//  25 Jan 08   R. Proulx   DIMS 179755: Changed verify_time to verify_priority in AdmLunBindData structure
//	24 Oct 08	D. Hesi		DIMS 197128: Added LU Shrink Support
//	14 Nov 08	D. Hesi		DIMS 213061: Added FLU IOCTL support for Lun Shrink implementation
//  10 Nov 08   R. Saravanan Disk Spin Down Feature support
//  15 Dec 08   R. Saravanan Disk Spin Down phase 2 Feature support: Addition of idletimestandby entry in admraidgroupconfig 
//	11 Dec 08	D. Hesi		Added 10G Poseidon SLIC support
//	06 Jan 09	D. Hesi		Added Online Disk Firmware Upgrade (ODFU) support
//	22 Apr 09	R. Saravanan	DIMS 225276: Mapped system error codes to Sunburst error
//	14 Sep 09	R. Singh	DIMS 231648: Added support for NT/Mirror rebuild status.
//	20 Oct 09	B. Myers	DIMS 213810: Added support for SetNDUInProgress.
//	28 Oct 09	E. Carter	Coverity fixes.
//	04 Oct 10	B. Myers	Added support for SetNDUState.
//	24 Jan 11	D. Hesi		ARS 400263: Fix for capacity issues in Flare for rebinding  in holes
//	04 May 11	E. Carter	AR 414210 - Update MAX_ENCLOSURE_SLOTS for Voyager.
//	11 May 11	E. Carter	ARS 415421 - Add support for EMV
//  25 Jul 11	M. Iyer		Add large element size support for RG
//  24 Jul 12   D. Hesi     ARS 499781: Added support for MCR ODFU
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------Defines

#ifndef FLARE_ADMIN_CMD_H
#define FLARE_ADMIN_CMD_H

#define CALLBACK_FCN void

#include <windows.h>

#include "user_generics.h"
#include "adm_api.h"
#include "flare_adm_ioctls.h"
#include "SafeHandle.h"
#include "CaptivePointer.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include "ntmirror_user_interface.h"
#include "k10diskdriveradmin.h"

// This is where we define data structures that need to differ
// from the ones in the interface provided by Flare.  A major
// reason that admin indentifies most objects by key (usually 
// in the style of a WWN) rather than by an index.  The downside
// is that the locally defined structures will need to track 
// changes in their Flare equivalents.  Such changes may revlock
// callers.

typedef enum _AdmFlashState
{
    FLARE_FLASH_OFF = 0,
    FLARE_FLASH_ON,
    FLARE_FLASH_BLINK_10,   // 1st bit on, 2nd off
    FLARE_FLASH_BLINK_01,   // 1st bit off, 2nd on
} AdmFlashState;

#define MAX_ENCLOSURE_SLOTS         60		// Right now Voyager contains max # of slots.

typedef struct _AdmFlashControl
{
    K10_WWID        EnclosureWWN;
    BOOL            flash_enable;
    AdmFlashState   EnclosureFlash;
    AdmFlashState   DriveFlash[MAX_ENCLOSURE_SLOTS];
} AdmFlashControl;


typedef struct _AdmLunBindData
{
    DWORD unit_type;          // Mimics same field in LOGICAL_UNIT 
    DWORD phys_unit;          // host selects unit number 
    DWORD rebuild_priority;   // varies the amount of cycles given for rebuild
    LBA_T num_stripes;        // use "0" for controller to determine size 
    DWORD element_size;       // use "0" for controller to use default 
    DWORD elements_per_parity_stripe;	// use "0" for controller to use default 
    DWORD lu_attributes;      // mimics same field in LOGICAL_UNIT 
    DWORD cache_config_flags; // mimics same field in LOGICAL_UNIT
    DWORD fru_count;          // numbers of FRUs in this unit 
    DWORD flags;              // Miscellaneous Flags 
    DWORD verify_priority;    // determines background verify rate 
    DWORD zero_throttle_rate; // zero throttle rate value
    K10_WWID groupWwn;         // Raid group WWN.
    LBA_T address_offset;
    WORD align_lba;          // Sector to align with stripe boundary.

    // Non-Destructive Bind msz 25jul02
    BOOL NDBFlag;
    DWORD NDBPassword;

    BOOL FixLUN;

    /* List of disks on the unit 
     */
    K10_WWID DiskWwn[MAX_DISK_ARRAY_WIDTH];  
    
    /* The LUN's World Wide Name
     */
    K10_WWID        world_wide_name;
    
    /* The LUN's user defined name.
     */
    K10_LOGICAL_ID  user_defined_name;

    TRI_STATE is_private;          // unit is private/internal 
} AdmLunBindData;

// Input data for RAID Group Configuration IOCTL
typedef struct _AdmRaidGroupConfigCmd
{
    ADM_RG_CONFIG_OPCODE    opcode;
    BOOL                    extend_length;
    TRI_STATE               explicit_remove;
    UINT_32                 raid_group_id;  // INVALID_RAID_GROUP to let Flare pick
                                            // Only used for Create!
    K10_WWID                raid_group_wwn;  
    UINT_32                 disk_count;
    K10_WWID                drive_wwn[MAX_DISK_ARRAY_WIDTH];
    ADM_RG_EXP_RATE         expansion_rate;
    DWORD                   raid_type;      // Raid Type
    TRI_STATE               is_private;       // Raid Group is private/internal
	TRI_STATE               power_saving_enabled;
    UINT_64                 latency_time_for_becoming_active;
    UINT_64                 idle_time_for_standby;
	UINT_32                 raid_group_element_size;
}
AdmRaidGroupConfigCmd;

typedef struct _AdmMemoryConfig
{
    K10_WWID    SpWWN[2];
    DWORD       dwNumberSPsSet;
    DWORD       write_cache_size;
    DWORD       read_cache_size[2];
    DWORD       hi5_cache_size;
} AdmMemoryConfig;

// Define local version that uses a key instead of fru #
typedef struct AdmReceiveDiagCmdStruct
{
    UINT_32                     param_list_length;
    ADM_RECEIVE_DIAG_PAGE_CODE  page_code;
    K10_WWID                    diskKey;
}
AdmReceiveDiagCmd;

typedef struct _FlareShrinkCapacityInfo
{
    LBA_T    ullCapacity;
    ULONG    ulFlags;
    DWORD    dwLunNumber;
}FlareShrinkCapacityInfo;

typedef struct _AdmOdfuConfig
{
	csx_uchar_t *pHeaderImage;
	ULONG		uHeaderSize;
	csx_uchar_t *pFirmwareImage;
	ULONG		uFirmwareSize;
} AdmOdfuConfig;

// used to describe a change to a disk pool
typedef struct _AdmDiskPoolParams
{
	UINT_32				number;
	UINT_32				diskCount;
	K10_WWID			diskWwn[FRU_COUNT];
} AdmDiskPoolParams;

#define ADM_RECEIVE_DIAG_EXTRA      2048    
#define ADM_RECEIVE_DIAG_BUF_SIZE (sizeof(ADM_RECEIVE_DIAG_DATA) + 2*sizeof(ADM_RCV_DIAG_FLARE_PARAM) + ADM_RECEIVE_DIAG_EXTRA)
#define ADM_INVALID_QUEUE_DEPTH     0

#define	FLARE_INVALID_POOL_ID	0xFFFFFFFF

class FbeCommand;
class FbeData;

class FlareCommandImpl
{
public:
    FlareCommandImpl(FbeData *pFbeData);
    ~FlareCommandImpl();

    HRESULT Bind( AdmLunBindData *bindInfo, DWORD &dwLun );
    HRESULT UnBind( K10_LU_ID wwnLun );
    HRESULT Verify( ADM_VERIFY_CMD *verify );
    HRESULT GroupVerify( ADM_GROUP_VERIFY_CMD *groupVerify );
    HRESULT SystemVerify( ADM_GROUP_VERIFY_CMD *SystemVerify );
    HRESULT VerifyReport( K10_WWID wwn, void *_report );
    HRESULT DiskVerify( const BOOL& bDiskVerify );
    HRESULT BackDoorPower( ADM_BACKDOOR_POWER_CMD *backDoor );
    HRESULT BackDoorDiskPower( K10_WWID DiskWwn, ADM_FRU_POWER power_on, BOOL defer );
    HRESULT StartProactiveSpare( K10_WWID DiskWwn );
    HRESULT StartDiskCopyTo( K10_WWID src, K10_WWID dest );
    HRESULT LunConfig( ADM_UNIT_CONFIG_CMD *cmd );
    HRESULT LunConfigBind( ADM_UNIT_CONFIG_CMD *cmd );
    HRESULT FlexportConfig( ADM_PORT_CONFIG_DATA *cmd );
    HRESULT IOMPortConfig( ADM_LED_CTRL_CMD *cmd, BOOL bIOM );
    HRESULT IOMSlicUpgrade(ADM_SET_SLIC_UPGRADE_DATA *cmd );
    HRESULT CancelFirmwareUpgrade();
    HRESULT WriteFirmwareODFU(csx_uchar_t* pBytes, int iDataSize, AdmOdfuConfig *cmd);
    HRESULT RGBindConfig(ADM_BIND_GET_CONFIG_CMD *cmd, ADM_BIND_GET_CONFIG_DATA *status);

    HRESULT UpdatePoolId(const AdmDiskPoolParams &dpParams);
	HRESULT DestroyDiskPool(long pool_id);

    // This method allocates the memory
    HRESULT EnclosureConfig( AdmFlashControl *cmd );
    void SystemConfigInit( ADM_SYSTEM_CONFIG_CMD *cmd );
    void PowerSavingsConfigInit( ADM_POWER_SAVING_CONFIG_CMD *cmd );
    HRESULT ClearCacheDirtyLun( K10_LU_ID wwnLun, DWORD dwPassword );
    HRESULT SystemConfig( ADM_SYSTEM_CONFIG_CMD *cmd );
    HRESULT PowerSavingsConfig( ADM_POWER_SAVING_CONFIG_CMD *cmd );
    HRESULT RaidGroupConfig( AdmRaidGroupConfigCmd *cmd );

    // The data returned is expected to run off the end of the struct, 
    // so a macro is used to compute the buffer size
    // and the actual data size returned.
    
    HRESULT ZeroDisk( K10_WWID DiskKey, BOOL bStop );
    HRESULT Commit();
    HRESULT Reboot(ADM_REBOOT_SP_CMD *cmd);

    /* queue depth per FRU functionality (interface) */
    HRESULT SetQueueDepth(CM_QUEUE_DEPTH_OP type_of_op, DWORD depth ); 

    /* Set NDU in progress flag */
    HRESULT SetNDUInProgress( bool bInProgress ); 

    /* Set NDU state data structure */
    HRESULT SetNDUState( ADM_SET_NDU_STATE_CMD *pNDUState ); 

    HRESULT GetCompatMode(K10_COMPATIBILITY_MODE_WRAP * pWrap, DWORD * pBufSize);

	// Management Port Speeds Settings
    HRESULT ManagementPortSpeeds(ADM_CONFIG_MGMT_PORTS_CMD *cmd);
    // NT/Mirror rebuild status
    HRESULT NtMirrorRebuildStatus(NTMIRROR_REBUILD_STATUS *cmd);
    HRESULT SetEMV(LBA_T EMV);

    HRESULT Shutdown();
	HRESULT SpPanic();
  
    HRESULT EstablishKEKKEK(LBA_T& encryptionNonce);
    HRESULT ActivateEncryption(LBA_T encryptionNonce);
    HRESULT InitiateBackup(char* encryptionBackupFilename);
    HRESULT NotifyBackupFileRetrieved(const char* backupFilename);
    HRESULT GetAuditLogFile(DWORD year, DWORD month, char* auditLogFile);
    HRESULT NotifyAuditLogFileRetrieved(const char* auditLogFilename);
    HRESULT GetCSumFile(const char* auditLogFile, char * csumFile);
    HRESULT NotifyCSumFileRetrieved(const char* csumFile);
    HRESULT GetActivationStatus(LBA_T& activationStatus);
 
    HRESULT DoIoctlToNtMirror( unsigned long dwIoControlCode,
            void * lpInBuffer,  unsigned long nInBufferSize,  
            void * lpOutBuffer,   unsigned long nOutBufferSize, DWORD *lpBytesReturned );


    HRESULT WhatToReturnV1( HRESULT hr);
    static HRESULT WhatToReturn( HRESULT hr, DWORD dwSunburst );
    static HRESULT GetSpId(int &iSpId);

    bool        m_bIsAsynch;            // Open was for asynch IOCTL
    char        m_spId;
    SafeHandle  m_hFlare;

	FbeCommand *mpFbeCommand;
	FbeData *mpFbeData;
};

// Class methods are pretty much 1 to 1 with the available IOCTLs.
// A few extra methods are provided for convenience.

class CSX_CLASS_EXPORT  FlareCommand
{
public:

    FlareCommand();
    FlareCommand(FbeData *pFbeData);
    ~FlareCommand();

    HRESULT Bind( AdmLunBindData *bindInfo, DWORD &dwLun );
    HRESULT UnBind( K10_LU_ID wwnLun );
    HRESULT Verify( ADM_VERIFY_CMD *verify );
    HRESULT GroupVerify( ADM_GROUP_VERIFY_CMD *GroupVerify );
    HRESULT SystemVerify( ADM_GROUP_VERIFY_CMD *SystemVerify );
    HRESULT VerifyReport( K10_WWID wwn, void *report );
    HRESULT DiskVerify( const BOOL& bDiskVerify );
    HRESULT BackDoorPower( ADM_BACKDOOR_POWER_CMD *backDoor );
    HRESULT BackDoorDiskPower( K10_WWID DiskWwn, ADM_FRU_POWER power_on, BOOL defer );
    HRESULT StartProactiveSpare( K10_WWID DiskWwn );
    HRESULT StartDiskCopyTo( K10_WWID src, K10_WWID dest );
    HRESULT LunConfig( ADM_UNIT_CONFIG_CMD *cmd );
    HRESULT LunConfigBind( ADM_UNIT_CONFIG_CMD *cmd );
    HRESULT FlexportConfig( ADM_PORT_CONFIG_DATA *cmd );
    HRESULT IOMPortConfig( ADM_LED_CTRL_CMD *cmd, BOOL bIOM);
    HRESULT IOMSlicUpgrade(ADM_SET_SLIC_UPGRADE_DATA *cmd );
    HRESULT CancelFirmwareUpgrade();
    HRESULT WriteFirmwareODFU(csx_uchar_t* pBytes, int iDataSize, AdmOdfuConfig *cmd);
    HRESULT RGBindConfig(ADM_BIND_GET_CONFIG_CMD *cmd, ADM_BIND_GET_CONFIG_DATA *status);

    HRESULT UpdatePoolId(const AdmDiskPoolParams &dpParams);
    HRESULT DestroyDiskPool(long pool_id);

    // This method allocates the memory
    HRESULT EnclosureConfig( AdmFlashControl *cmd );
    HRESULT SystemConfig( ADM_SYSTEM_CONFIG_CMD *cmd );
    HRESULT PowerSavingsConfig( ADM_POWER_SAVING_CONFIG_CMD *cmd );
    HRESULT ClearCacheDirtyLun( K10_LU_ID wwnLun, DWORD dwPassword );
    void SystemConfigInit( ADM_SYSTEM_CONFIG_CMD *cmd );
    void PowerSavingsConfigInit( ADM_POWER_SAVING_CONFIG_CMD *cmd );
    HRESULT RaidGroupConfig( AdmRaidGroupConfigCmd *cmd );
   
    HRESULT ZeroDisk( K10_WWID DiskKey );
    HRESULT Commit();
    HRESULT Reboot(ADM_REBOOT_SP_CMD *cmd);

    // queue depth per FRU functionality (interface)
    HRESULT SetQueueDepth(CM_QUEUE_DEPTH_OP type_of_op, DWORD depth=ADM_INVALID_QUEUE_DEPTH ); 

    /* Set NDU in progress flag */
    HRESULT SetNDUInProgress( bool bInProgress ); 

    HRESULT GetCompatMode(K10_COMPATIBILITY_MODE_WRAP * pWrap, DWORD * pBufSize);

    /* Set NDU state data structure */
    HRESULT SetNDUState( ADM_SET_NDU_STATE_CMD *pNDUState ); 

    // Management Port Speeds Settings
    HRESULT ManagementPortSpeeds(ADM_CONFIG_MGMT_PORTS_CMD *cmd);
    // NT/Mirror rebuild status
    HRESULT NtMirrorRebuildStatus(NTMIRROR_REBUILD_STATUS *cmd);
    //Set Expected Memory Value
    HRESULT SetEMV(LBA_T EMV);

	FbeData * GetFbeData()	{ return mpFbeData; }

    HRESULT Shutdown();
	HRESULT SpPanic();

    HRESULT EstablishKEKKEK(LBA_T& encryptionNonce);
    HRESULT ActivateEncryption(LBA_T encryptionNonce);
    HRESULT InitiateBackup(char* encryptionBackupFilename);
    HRESULT NotifyBackupFileRetrieved(const char* backupFilename);
    HRESULT GetAuditLogFile(DWORD year, DWORD month, char* auditLogFilename);
    HRESULT NotifyAuditLogFileRetrieved(const char* auditLogFilename);
    HRESULT GetCSumFile(const char* auditLogFilename, char* csumFile);
    HRESULT NotifyCSumFileRetrieved(const char* csumFile);
    HRESULT GetActivationStatus(LBA_T& activationStatus);

protected:
    HRESULT DoIoctlToNtMirror( unsigned long dwIoControlCode,
            void * lpInBuffer,  unsigned long nInBufferSize,  
            void * lpOutBuffer,   unsigned long nOutBufferSize, DWORD *lpBytesReturned );

	FbeData *mpFbeData;

private:
    FlareCommandImpl *m_impl;

};


#endif
