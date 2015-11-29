// FlareData.h: interface for the FlareData class
//
// Copyright (C) 2001-2012  EMC Corporation
// 
//
// This class encapsulates the XCam Flare poll data
//
//
//  Revision History
//  ----------------
//   5 Jul 01   H. Weiner   Initial version.
//  25 Jul 01   H. Weiner   Simpler flash control,Key LUNs, FRUs, etc. by WWN
//  30 Jul 01   H. Weiner   Add size parameter to List methods
//  31 Aug 01   H. Weiner   Flare Object classes
//  10 Sep 01   H. Weiner   Add missing methods to Enclosure, LCC, & LUN classes
//  28 Sep 01   H. Weiner   Remove from FlareLocalSP methods that will actually get data from hostside or NT
//  12 Oct 01   H. Weiner   Different memory/cache size routines, EnclosureSpCount, Lun's RG WWN
//  23 Oct 01   H. Weiner   Add virtual functions to FlareArray to serve as defaults for derived classes
//  14 Dec 01   H. Weiner   Code review inspired changes
//  28 Dec 01   H. Weiner   CreateEnclosureSPSKey now requres only slot as input.
//  15 Jan 02   H. Weiner   Add FlarePersonality class.
//  11 Feb 02   C.J. Hughes Move CmmControlPoolTop, CmmControlPoolTop, and
//                          CmmControlNonCacheUsed from FlareLocalSP to
//                          FlareSP, since they are now available for
//                          both SPs
//  11 Feb 02   H. Weiner   Extensive changes to allow reporting of disks in 
//                          missing enclosures IF they are in a current RAID Group.
//  17 Apr 02   H. Weiner   DIMS 72033: Add FlareSP::IsMissing() to identify a dead SP.
//  04 Jun 02   B. Yang     DIMS 74093: Add sp timestamp back in
//  11 Jun 02   B. Yang     DIMS 74564: Add Non-Zero Queue/Sum queue Lengths in FlareLocalSP
//  12 Jun 02   K. Keisling DIMS 74567: Add LUN performance histograms
//   9 Sep 02   H. Weiner   DIMS 77796: add FlareDisk::TlaNumber()
//  26 Nov 02   H. Weiner   Add FreshlyBound()
//  10 Apr 03   H. Weiner   DIMS 85939: Add FlareSP version of WriteCacheState() 
//  25 Jun 03   C. Hopkins  Added pragmas to deal with compiler warnings
//  18 Sep 03   C. Hopkins  Modified FlareData constructor prototype
//  06 Apr 05   P. Caldwell Added UserSniffControlEnable method prototype.
//  27 Jun 05   K. Mai      DIMS 122008 - Port raid type support from Piranha to Release 22 Hammer.
//  06 Sep 05   P. Caldwell Moved FlareBusInfo class to FlareData.cpp.
//  29 Sep 05   P. Caldwell New methods added for Hammer support.
//  30 Aug 05   P. Caldwell Hammer SubFRU support. Added FlareSubFRU class.
//  02 Nov 05   R. Hicks    Add ElementsPerParityStripe()
//  16 Jan 06   R. Hicks    Add GetSfpCondition()
//  03 Apr 06   P. Caldwell Added ZeroThrottleRate and PctZeroed for FastBind
//  26 Jun 06   P. Caldwell DIMS 149349 Added prototype for VendorPartNo to FlareEnclosure class.
//  27 Jun 06   M. Khokhar  DIMS 149499 Added VendorPartNo, VendorArtworkRevisionLevel, VendorSerialNumber and VendorAssemblyRevisionLevel 
//                          functions to FlareLCC, FlareEnclosure, FlareSP, FlareSubFRU and FlarePersonality classes.
//  20 Sep 06   P. Caldwell DIMS 153074 - Added CreateEnclosureVbusKey, AssociatedSp, and VbusKey.
//  26 Jul 06   P. Caldwell Added RAID6 support
//  16 Oct 06   P. Caldwell Added SubFRU State function and Boot Peer functions
//  15 May 07   M. Khokhar  Changes made for Write Cache Availability
//  24 May 07   V. Chotaliya    _ADM_ENCLOSURE_ID_LIST structure modified for Common Flare Bundle Changes
//  10 Aug 07   D. Hesi     Added I/O Annex State functions
//  14 Aug 07   D. Hesi     Added I/O Module and Flexport State functions
//  23 Aug 07   D. Hesi     DIMS 177294 - Added IsValidPort() function to validate VBus ports 
//  30 Sep 07   H. Rajput   Modifications to incorporate Management Port Speeds Settings
//  10 Oct 07   D. Hesi     DIMS 180512 - Added new interfaces to return enclosure based I/O Module and I/O Annex Count.
//  29 Oct 07   R. Hicks    Add support for internal LUNs and Raid Groups
//  09 Nov 07   D. Hesi     DIMS 181622 - Removed interface WCACurrentMaxSize(), not used anymore.
//  18 Dec 07   R. Proulx   DIMS 183595 - Renamed maxRebuildTime to RebuildPriority
//  25 Jan 08   R. Proulx   DIMS 179755 - Renamed maxVerifyTime to VerifyPriority
//  25 Jan 08   R. Hicks    Added functions to return Product SN, PN, Rev Resume data
//  10 Mar 08   D. Hesi     DIMS 188904: Added SMBus access fault bubble to Fan, PS, IO Module, IO Annex and
//                          Management SubFRU
//  23 Apr 08   D. Hesi     DIMS 195671: Modified IO Annex Firmware Revision function to return formatted string
//  11 Aug 08   H. Rajput   DIMS 204997: Added support for Critical/Foreign/Missing drive bubbles.
//  25 Aug 08   D. Hesi     DIMS 199448: Modified "Enclosure Drive type" and "Disk Drive type" implementation
//  12 Sep 08   R. Saravanan DIMS 204997 - Added the method NeedsRebuild() in FlareDisk class 
//  19 Sep 08   D. Hesi     Added support for 8G Glacier FC I/O Module (Libra release)
//  04 Dec 08   R. Singh    Added EngineIdFault() function prototype
//  09 Dec 08   D. Hesi     DIMS 213062: Removed INVALID_DRIVE_TYPE definition
//  10 Nov 08   R. Saravanan Disk Spin Down Feature support
//  15 Dec 08   R. Saravanan Disk Spin Down phase 2 changes: Addition of idletimestandby() method.
//  11 Dec 08   D. Hesi     Added 10G Poseidon SLIC support
//  06 Jan 09   D. Hesi     Added Online Disk Firmware Upgrade (ODFU) support
//  27 Jan 09   D. Hesi     DIMS 215129: Added new LUN performance counters
//  06 Jan 09   R. Hicks    DIMS 217632: Poll performance enhancements.
//  21 Feb 09   R. Singh    DIMS 204325: Added new LCC performance counters
//  26 Feb 09   H. Weiner   Added PowerSavingCapable() to FlareRaidGroup class
//  04 Mar 09   D. Hesi     DIMS 217865: Added interface for FLU current status 
//  06 Jul 09   R. Saravanan DIMS 228751: Added new interface for checking unbound limit
//..23.Jul.09...R..Singh    DIMS 223796: Added tag TAG_UNSAFE_TO_REMOVE_SP to support property,
//                          remove sp or not.
//  10 Sep 09   E. Carter   DIMS 237451 - Add Debug code to return drive type based on values from a text file.
//  08 Oct 09   E. Carter   DIMS 236263 - Remove FlareEnclosure::EnclosureDriveType() method.
//  03 Dec 09   E. Carter   Added Scorpion Support for statistics and max. values.
//  05 Jan 10   E. Carter   DIMS 247539 - Added MaxDriveSlots( ) method.
//  15 Jan 10   E. Carter   Added Scorpion Flexport support.
//  27 Jan 10   E. Carter   Added additional PS resume and PS Firmware Upgrade method for Scorpion.
//  07 Jan 10   R. Hicks    Added FCoE support.
//  05 Mar 10   E. Carter       DIMS 238557 - Modify disk simulation
//  08 Mar 10   E. Carter   ARS 346417 - DIMS Added SFPVendorPartNumber( ) and SFPVendorSerialNumber( ) for Scorpion
//  26 Mar 10   R. Hicks    Backing out 238557; breaking bundles
//  06 Apr 10   E. Carter   ARS 346417 - Needed to add arguments to FlareFlexport::SupportedFPSpeeds( ) call.
//  22 Apr 10   E. Carter   ARS 358124 - Add Enclosure number methods to FlareBusInfo.
//  22 Apr 10   E. Carter   ARS 361303 - Add Supported method to FlarePower.
//  17 May 10   E. Carter   ARS 366043 - Added SuggestedHLU( ) and Type( ) to FlareLun.  Adjust glut counts.
//  01 Jun 10   E. Carter   ARS 369064 - Add FlareArray::StatisticsDuration( ) method.
//  23 Jul 10   R. Hicks    Stop using shared memory to hold Flare poll data
//  11 Jun 10   E. Carter   Add Voyager Support.
//  05 Apr 11   M. Gupta    ARS 406228 - Add MaxEnclDiskCount(DWORD dwEnclosure) to FlareEnclosure
//  11 May 11   E. Carter   ARS 415421 - Add support for EMV
//  22 Aug 11   E. Carter   ARS 380420 - Add FlareArray::IsCelerraLUN( ) and GetCelerraLUNCount( ) method.
//  07 Dec 11   D. Hesi     ARS 458674 - Add GetRGAttributes() method
//  28 Dec 11   D. Hesi     ARS 462183 - Add MaxReadCacheSize() method
//  20 Jan 12   D. Hesi     ARS 465813 - Add FlareArray::IsNonCelerraSystemLUN() method.
//  14 Feb 12   D. Hesi     ARS 471758 - Add opaque_info_u union to read OPAQUE_64 data
//  02 April 12 A. El-Gamel Added support for Disk-based Sniff (MCR) 
// 
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------Defines

#ifndef FLARE_DATA_H
#define FLARE_DATA_H

#include "k10defs.h"
#include "FlareCommand.h"
#include "CaptivePointer.h"
#include "dba_export_api.h"
#include "adm_sp_api.h"
#include "MManModepage.h"
#include "K10TraceMsg.h"
#include "WWIDAssocHashUtil.h"
#include "HashUtil.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define MAX_NUMBER_SP   2
typedef enum _AdmSpId
{
    IS_SP_A,
    IS_SP_B
}
AdmSpId;

//#define ADM_LBA       ULONGLONG
#define ADM_LBA             LBA_T

#define FRU_ID              K10_WWID

// Use this value for Address when calling 
// FlareArray::CreateEnclosureKey to get the XPE's key
#define LOOP_ENCLOSURE_XPE      (0xFF)
#define ENCLOSURE_ADDRESS_XPE   LOOP_ENCLOSURE_XPE
#define INVALID_FLEX_PORT_NUMBER (0xFF)
#define MAX_FIRMWARE_REV_LEN    6
#define MAX_EXPANDER_FW_COUNT      5

// Use these values for finding out the unbound unit
#define GLUT_START  0
#define GLUT_GENERAL_END  ((GLUT_START + FLARE_MAX_USER_LUNS) - 1)
#define GLUT_SP_ONLY_START (GLUT_GENERAL_END + 1)
#define GLUT_SP_ONLY_END ((GLUT_SP_ONLY_START + GLUT_SP_ONLY_COUNT) - 1)

#define GLUT_FLARE_ONLY_START (GLUT_SP_ONLY_END + 1)
#define GLUT_FLARE_ONLY_END \
    ((GLUT_FLARE_ONLY_START + GLUT_FLARE_ONLY_COUNT) - 1)

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)

#define GLUT_SP_ONLY_COUNT                   17
#define GLUT_FLARE_ONLY_COUNT                1
#define SP_ONLY_RESERVED_COUNT               17 /* Preallocated LU's */
#define FLARE_ONLY_RESERVED_COUNT            1  /* Preallocated LU's */

#else // UMODE_ENV

#define GLUT_SP_ONLY_COUNT          32
#define GLUT_FLARE_ONLY_COUNT       32
#define SP_ONLY_RESERVED_COUNT      17  /* Preallocated LU's */
#define FLARE_ONLY_RESERVED_COUNT    1  /* Preallocated LU's */

#endif // UMODE_ENV

// The following is a compile time assert to guarantee that the MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS is
// greater than or equal to SP_ONLY_RESERVED_COUNT.  If SP_ONLY_RESERVED_COUNT is modified then
// you must also update MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS in core_config.h
extern int SP_ONLY_RESERVED_COUNT_CompileTimeAssert[SP_ONLY_RESERVED_COUNT <= MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS];

// Simulated drive data
typedef struct _SIMUL_DRIVE_DATA
{
    csx_uchar_t Type;
    LBA_T uSize;
} SIMUL_DRIVE_DATA;

// How far a bind has proceeded on a LUN
typedef enum _ADM_BIND_STATE
{
    ADM_BIND_UNBOUND,
    ADM_BIND_BINDING,
    ADM_BIND_UNASSIGNED,
    ADM_BIND_ASSIGNED,
    ADM_BIND_ASSIGNED_TO_PEER
} ADM_BIND_STATE;

typedef struct _ADM_ENCLOSURE_ID_LIST
{
    DWORD   dwNumber;
    csx_uchar_t* bId;			//[ADM_MAX_ENCLOSURES];    // When XPE data is OK use this

    _ADM_ENCLOSURE_ID_LIST() {
        Allocate();
    }

    ~_ADM_ENCLOSURE_ID_LIST() {
        if(bId != NULL)
            delete [] bId ;
    }

    _ADM_ENCLOSURE_ID_LIST(const _ADM_ENCLOSURE_ID_LIST& pTemp) {
        Allocate();
        Copy(pTemp);
    }

    _ADM_ENCLOSURE_ID_LIST& operator = (const _ADM_ENCLOSURE_ID_LIST& pTemp) {
        Copy(pTemp);
        return *this;
    }
private:
    void Allocate() {
        dwNumber = (DWORD) 0;
        bId = new csx_uchar_t[ADM_MAX_ENCLOSURES];
        if (bId == NULL) {
            THROW_MEMORY_ERROR();
        }
    }

    void Copy(const _ADM_ENCLOSURE_ID_LIST& pTemp) {
        dwNumber = pTemp.dwNumber;
        for(int i = 0; i < ADM_MAX_ENCLOSURES ; i++)
        {
            bId[i] =  pTemp.bId[i];
        }
    }
} ADM_ENCLOSURE_ID_LIST;


typedef enum _ADM_ENCLOSURE_STATUS
{
    ADM_ENCLOSURE_PRESENT,
    ADM_ENCLOSURE_MISSING,
    ADM_ENCLOSURE_PHANTOM
} ADM_ENCLOSURE_STATUS;

union opaque_info_u
{
    OPAQUE_64       opaque_info;
    struct
    {
        UINT_32     opaque_32bit_first;
        UINT_32     opaque_32bit_second;
    } uint_info;
};


class CSX_CLASS_EXPORT FlareData : public FlareCommand
{
public:

    FlareData();
    FlareData(bool bDoPoll);
    ~FlareData();

    // bFull    true = complete repoll
    //          false = update
    // Generally users should call with false, as an initial poll is performed in the constructor
    // pElapsedTime: if non-NULL, will return elapsed time in msec to perform poll
    HRESULT     Repoll(bool bFull, LONGLONG *pElapsedTime = NULL);

	void BlockEventNotifications();
	void UnblockEventNotifications();

	// force a repoll of the cached MCR data
    void RepollMCR();

	bool GetFullPollRequired();
	void ClearFullPollRequired();

	static void DestroyFbeData();

//MCR_STATS
    bool setStats(bool on_off);
    UINT_32  getPerfTimeStamp();
    void InvalidateDiskPerfData();
    void InvalidateLunPerfData();
    HRESULT GetMCRDiskPerfData(K10_WWID &drvWwn, void* perfstats);
    HRESULT GetMCRLunPerfData(K10_WWID lunWwn, void* perfstats);
//MCR_STATS
    HRESULT mapPbaToLba(UINT_32 raid_group_id,
                        USHORT position,
                        UINT_64 pba,
                        UINT_64 * lba,
                        bool * is_parity);

	HRESULT GetGenPropsList(csx_uchar_t **ppOut);

#ifndef ALAMOSA_WINDOWS_ENV
    OPAQUE_PTR  GetDataPointer()  { return m_FlareData; }
#else
    OPAQUE_32   GetDataPointer()  { return m_FlareData; }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - 64 bit admin changes */

	static ADM_SP_ID m_SpId;

protected:
#ifndef ALAMOSA_WINDOWS_ENV
    // Handle used to call the dyn_* routines, valid only after the first poll
    OPAQUE_PTR          m_FlareData;
#else
    // Handle used to call the dyn_* routines, valid only after the first poll
    OPAQUE_32           m_FlareData;
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - 64 bit admin changes */

    CPtr<VOID>  m_npSharedFlareData;

    // Bytes required for m_SharedFlareData
    DWORD               m_dwSharedPollSize;

    // A copy of the poll header made at the time of the poll.  This copy
    // will contian pointers to the shared memory.  Its primary purpose is to 
    // keep a local copy of pointers set on the poll, to allow
    // multiple processes to have pointers into the poll data.
    CHAR *              m_PollHeaderBuffer;

    // Bytes required for m_PollHeaderBuffer
    DWORD               m_dwPollHeaderSize;

#pragma warning(disable:4251)
    NPtr<K10TraceMsg>   m_npTrace;
#pragma warning(default:4251)

private:
    VOID Constructor(bool bDoPoll);
};

//**********************************************************************
//              Object classes start here
//**********************************************************************

//
// This class contains methods that are global to the array plus 
// virtual functions defined for use by derived classes.
class CSX_CLASS_EXPORT FlareArray
{
public:

    FlareArray( FlareData *FdObj, WWIDAssocHashUtil *DiskPoolDiskHashUtil = NULL ); 

    virtual ~FlareArray() {}

    // ************* General Methods USE THESE INSTEAD OF DEFINES ************
    // returns the maximum number of user LUNs
    DWORD PublicLunCount();

    // returns the maximum number of private (SP-only, Flare-only, L2 cache) LUNs
    DWORD PrivateLunCount();

    // returns the number of raid groups
    DWORD RaidGroupCount();

    // returns the number of private RAID groups
    DWORD PrivateRaidGroupCount();

    // ****************** Component Count Methods  ***********************

    // returns the current number of enclosures in the system
    DWORD EnclosureCount();

    // Total number of LCCs in the array
    HRESULT LccCount(DWORD &dwNumberOfLccs);

    // Total number of SSCs in the array
    HRESULT SSCCount(DWORD &dwNumberOfSSCs);
    
    HRESULT PsCount(DWORD &dwNumberOfPowerSupplies);

    HRESULT FanCount(DWORD &dwNumberOfFans); 

    // Number of SPS's
    HRESULT SpsCount(DWORD &dwNumberOfSPSs);

    HRESULT DiskCount(DWORD &dwNumberOfDiskDrives);

    // Number of I/O Annex 
    DWORD IOAnnexCount();

    // Number of IO modules per SP
    DWORD IOModuleCount();

    // Number of DIMM slots in the array 
    DWORD DimmCount();

    // Number of SSD slots in the array 
    DWORD SsdCount();

    // Number of FC ports per SP (FE+BE ports)
    DWORD FCPortCount();

    // Number of 1 GB iSCSI ports per SP (FE ports)
    DWORD ISCSIPortCount();

    // Number of FCoE ports per SP (FE ports)
    DWORD FCoEPortCount();

    // Number of Flexports per SP
    DWORD FlexportsCount();

    // Number of FC interconnects per SP (FE+BE ports)
    DWORD FCInterconnectsCount();

    // Returns supported I/O Module types (bitmask value)
    DWORD SupportedSLICTypes();

    // Number of 10 GB iSCSI ports per SP (FE ports)
    DWORD ISCSI10GPortCount();

    // Number of SAS Back End ports per SP
    DWORD SASBEPortCount( );

    // Number of SAS Front End ports per SP
    DWORD SASFEPortCount( );

    // Number of FCOE Front End ports per SP
    DWORD FCOEFEPortCount( );

    // Number of disk drive slots on the array
    DWORD MaxDriveSlots( );

    // ****************** Component List Methods  ***********************
    // Return list of enclosure IDs. Method allocates the Memory
    HRESULT EnclosureList(  K10_WWID **ppWwnEnc, DWORD &dwNumberOfEnclosures );

    // Get list of WWNs of Dimms in entire array
    HRESULT DimmList(  K10_WWID **ppWwnDimm, DWORD &dwNumberOfDimms );

    // Get list of WWNs of SSDs in entire array
    HRESULT SsdList(  K10_WWID **ppWwnSsd, DWORD &dwNumberOfSsd );

    // Get list of WWNs for all LCCs in the array
    HRESULT LccList( K10_WWID **ppWwnLcc, DWORD &dwNumberOfLccs );

    // Get list of WWNs for all SSCs in the array
    HRESULT SSCList( K10_WWID **ppWwnSSC, DWORD &dwNumberOfSSCs);
    
    // Get list of WWNs of power supplies in entire array
    HRESULT PsList(  K10_WWID **ppWwnPs, DWORD &dwNumberOfPowerSupplies );

    // Get list of fans in array.
    HRESULT FanList(  K10_WWID **ppWwnFan, DWORD &dwNumberOfFans );
    
    // Get list and number of SPSs in array.
    HRESULT SpsList(  K10_WWID **ppWwnSps, DWORD &dwNumberOfSPSs);

    // Get a list and number of SubFRUs found on the specified SP
    HRESULT SubFRUList( K10_WWID **ppWwnSubFRU, DWORD &dwNumberOfSubFRUs, ADM_SP_ID dwSpNum);

    // Get list and number of disk drives in array.
    HRESULT DiskList(  K10_WWID **ppWwnDiskDrives, DWORD &dwNumberOfDiskDrives );

    // Get list and number of Disk Pools in array.
    HRESULT DiskPoolList( K10_WWID **ppWwnDiskPool, DWORD &dwNumberOfDiskPools );

    // Get list of RAID groups 
    HRESULT RgList(  K10_WWID **ppWwnRg, DWORD &dwNumberOfGroups );

    // Get a list of current SPs, by key. 
    HRESULT SpList(  K10_WWID **ppWwnSp, DWORD &dwNumberOfSps );

    // Get a list of current LUNS, by key. 
    HRESULT LunList( K10_LU_ID **ppWwnLun, DWORD &dwNumberOfLuns,
        HashUtil<K10_WWID, DWORD> *pLunListHashTable = NULL);

    // Get list of I/O Annex (SP)
    HRESULT IOAnnexList( K10_WWID **ppWwnIOAnnex, DWORD &dwNumberOfIOAs);
    // Get list of I/O Modules (Array)
    HRESULT IOModuleList( K10_WWID **ppWwnIOM, DWORD &dwNumberOfIOMs);

    // Get list of Flexports (SP)
    HRESULT FlexportList( K10_WWID **ppWwnFlexport, DWORD &dwNumberOfFlexports);

    // Get list of Buses
    HRESULT BusList( K10_WWID **ppWwnBus, DWORD &dwNumberOfBuses);

    // ****************** Component Key Creation Methods  ***********************
    // These are used only when servicing diagnostic routines that need to reference
    // components by their location. The diag routine 1st queries for the key via one
    // of these methods, then uses the key in a normal type call.

    // Create an Enclosure key.  For this and the following functions:
    //  dwInputBus      - the back end bus number
    //  dwEnclAddress   - value of Enclosure's thumbwheel switch
    //  dwInputBus      - the slot in the enclosure
    //  key             - in the style of a WWN
    // 
    // Note that dwLoopLoc is not the same as Flare's enclosure numbers, which imply
    // a bus number. dwLoopLoc is set via a switch on the enclosure.
    HRESULT CreateEnclosureKey( DWORD dwInputBus, DWORD dwLoopLoc, K10_WWID &key );

    HRESULT CreateEnclosureFanKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, K10_WWID &key );

    HRESULT CreateEnclosureDimmKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, DWORD dwInputSp, K10_WWID &key );

    HRESULT CreateEnclosureSsdKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, DWORD dwInputSp, K10_WWID &key );

    HRESULT CreateEnclosureLCCKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, K10_WWID &key );
    HRESULT CreateEnclosureSSCKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, K10_WWID &key );

    HRESULT CreateEnclosurePowerSupplyKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, K10_WWID &key );

    HRESULT CreateEnclosureSPSKey( DWORD dwInputSlot, K10_WWID &key );

    HRESULT CreateEnclosureDiskKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, K10_WWID &key );

    HRESULT CreateEnclosureSpKey( ADM_SP_ID SpNumber, K10_WWID &key ); 

    HRESULT CreateEnclosureVbusKey( DWORD dwInputBus, ADM_SP_ID SpNumber, K10_WWID &key );

    HRESULT CreateEnclosureRaidGroupKey( DWORD dwRaidGroupNumber, K10_WWID &key );

    HRESULT CreateEnclosureIOAnnexKey(DWORD dwAnnexSlotNo, DWORD dwSP, K10_WWID &key );

    HRESULT CreateEnclosureIOMKey( DWORD dwInputBus, DWORD dwEnclAddress, 
                                  DWORD dwInputSlot, DWORD dwSP, DWORD dwClass, K10_WWID &key );

    HRESULT CreateEnclosureFlexportKey( DWORD dwIOMSlotNo, DWORD dwPortNoInIOM, DWORD dwLogicalPort, 
                                        DWORD dwSP, DWORD dwPortRole, K10_WWID &key );

    // ****************** Other General Methods  ***********************
    // returns status of statistics logging
    BOOL StatisticsEnabled();
    ADM_LBA GetEMV();
    BOOL CMIFaulted(); 
    BOOL VaultFaultOverride();
    BOOL UserSniffControlEnable();
    BOOL DiskSniffEnable();
    BOOL IsProactiveSpareSupported(DWORD type);
    BOOL IsDefragSupported(DWORD type);
    BOOL IsRgExpansionSupported(DWORD type);
    BOOL IsNumberAvailable(DWORD lu_attributes);
    //WCA
    BOOL WCAEnabled();
    DWORD WCAMaxSize();

    //PowerSavings
    DWORD GlobalIdleTime();
    DWORD MinLatencyTolerance();
    BOOL PowerSavingsStatsEnable();
    BOOL GlobalPowerSavingsStatus();

    //Returns the executing SP id.
    ADM_SP_ID AssociatedSp();

    // Converts SP ID to WWN
    HRESULT SpIdToWWN( DWORD SPid, K10_WWID &wwn );

    // Gets WWN from SP ID, Logical Port No, and Flexport Role.
    HRESULT GetFlexportWWN(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, K10_WWID &key );
    HRESULT GetFlexportNo(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, DWORD &dwFlexportNo);

	DWORD GetFlexportNoList(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, DWORD dwFlexportNo[]);
	DWORD GetFlexportWWNList(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, K10_WWID key[] );
    BOOL    IsValidPort(DWORD dwPortLogicalNo, DWORD dwPortRole);

    // Given FRU, construct the disk key.
    bool                    DiskKeyfromFRU( DWORD dwFru, K10_WWID  &wwn);

    // Online DFU Status related interfaces
    DWORD ODFUProcessStatus();
    DWORD ODFUProcessFailCode();
    DWORD ODFUProcessOperationType();
    DWORD ODFUMaxConcurrentDiskCount();
    DWORD ODFUProcessStartTime();
    DWORD ODFUProcessStopTime();
    DWORD ODFUFirmwareFileName(DWORD buffer_size, char *buffer);

    // Encryption Related interfaces
    DWORD Get_Encryption_Mode();
    DWORD Get_Encryption_Backup_Status();
    DWORD Get_Encryption_Primary_SP_ID();
    DWORD Get_Encryption_Status();
    LBA_T Get_Encryption_Consumed_Blocks();
    LBA_T Get_Encryption_Encrypted_Blocks();

    // ******************  LCC Frumon Upgrade related methods  ***********************

    BOOL UpgradeInProgress();
    BOOL UpdateInProgress();
    BOOL PSUpgradeInProgress();
    BOOL YukonDlInProgress();
    DWORD SecBefUpgradeChk();
    DWORD CurrentStatusInterval();
    DWORD CurrentStatusCode();
    DWORD CurrentErrorCode();
    DWORD TotEnclToUgrade();
    DWORD EnclosuresCompleted();
    DWORD EnclosuresNumber();
    DWORD BusNumber();
    DWORD PercentComplete();
    DWORD QueuedUpgradeRequests();
    LARGE_INTEGER LccUpgradeStartTime();
    LARGE_INTEGER LccFirmwareCheckStartTime();

    // Gen3 Power Supply methods.
    DWORD MaximumPower( );
    DWORD CurrentPower( );
    DWORD AveragePower( );
    DWORD PowerStatus( );
    DWORD MaximumUtilization( );
    DWORD CurrentUtilization( );
    DWORD AverageUtilization( );
    DWORD UtilizationStatus( );
    DWORD StatisticsDuration( );

    // These exist only so that derived classes will inherit them as defaults
    virtual DWORD VendorPartNo(DWORD buffer_size, char *buffer); 
    virtual DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);
    virtual DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);
    virtual DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);
    virtual DWORD NumProgrammables() {  return 0;   }
    virtual DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);
    virtual DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    UINT_32       GetDriveType( UINT_32 fru );  // Method to get actual or simulated drive type.
    LBA_T         GetDriveSize( UINT_32 fru );  // Method to get actual or simulated drive size.
    static void   InitDriveTypeArray( );        // Method to initialize m_pDriveType.
    static void   EnableDriveSimulation( );     // Set drive simulation to on.
    bool          IsCelerraLUN( K10_WWID &wwn ); // Returns true if LUN is a Celerra LUN, false otherwise.
    bool          IsNonCelerraSystemLUN( K10_WWID &wwn ); // Returns true if LUN is a Non-Celerra system LUN, false otherwise.
    DWORD         GetCelerraLUNCount( );        // Returns the total number of Celerra LUNs.

	FbeData * GetFbeData()	{ return mpFbeData; }

protected:
    // These are utility functions to aid component listing & counting
    BOOL                    EnclosureIsRemoved( DWORD dwEnclosureNumber);
    ADM_ENCLOSURE_ID_LIST   EnclosureIdList();
   
    // Construct the key of the Enclousre or one of its components
    HRESULT                 CreateEnclosureComponentKey( DWORD dwInputBus, DWORD dwLoopLoc, 
                                  DWORD dwInputSlot, WORD dwCompType, K10_WWID &key, DWORD dwSP = 0 );

    FlareData           *m_DataObject;
#ifndef ALAMOSA_WINDOWS_ENV
    OPAQUE_PTR          m_FlareData;
#else
    OPAQUE_32           m_FlareData;
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - 64 bit admin changes */

	FbeData *mpFbeData;

    WWIDAssocHashUtil   *m_DiskPoolDiskHashUtil;

#pragma warning(disable:4251)
    NPtr<K10TraceMsg>   m_npKTrace;
#pragma warning(default:4251)

    static NAPtr<SIMUL_DRIVE_DATA>  m_napSimulatedDriveData; // Array of drive type values.
    static DWORD        m_dwSizeofDriveTypeArray;            // Size of m_napSimulatedDriveData array.
    static BOOL         m_bDriveSimulationEnabled;           // Drive attribute simulation turned on.
    static DWORD        m_dwCelerraLunCount;
};

//
// This class contains methods that can only refer to the local SP. 
// Putting these into a separate class adds clarity at the price of a bit of convenience.
//

class FbeLocalSpInfo;

class CSX_CLASS_EXPORT FlareLocalSP: public FlareArray
{
public:

    FlareLocalSP( FlareData *FdObj );

    ~FlareLocalSP() {}

    // returns SP signature
    DWORD SpSignature();

    // returns peer SP signature
    DWORD PeerSpSignature();

    // returns cabinet ID
    DWORD CabinetId();

    // returns SP ID (0=A 1=B);
//  AdmSpId Spid();
    ADM_SP_ID Spid();

    HRESULT LocalSpWWN(K10_WWID &wwn);

    // returns SP nport ID
    DWORD NportId();

    // returns peer SP nport ID
    DWORD PeerNportId();

    // returns SP revision number
    void RevisionNumber( DWORD *major, DWORD *minor, DWORD *pass_number);

    // returns write cache size in MB
    DWORD WriteCacheSize();

    // returns max write cache size in MB
    DWORD MaxWriteCacheSize();

    // returns write cache hit ratio (0-100)
    DWORD WriteCacheHitRatio();

    // returns write cache dirty page percentage (0-100)
    DWORD CacheDirtyPagePct();

    // returns percentage of write cache pages owned by this SP (15-85)
    DWORD CacheOwnedPagePct();

    // returns latest user request for write cache state
    BOOL WriteCacheEnable();

    // returns current write cache state, see ca_states.h
    DWORD WriteCacheState();

    // returns current peer write cache state, see ca_states.h
    DWORD PeerWriteCacheState();

    // A DWORD may not suffice here.  Subject to change!!
    DWORD PerfTimeStamp();

    // returns powersavings stats start time stamp
    DWORD PowerSavingStatsStartTimeStamp();

    // returns powersavings stats start date stamp
    DWORD PowerSavingStatsStartDateStamp();


    // returns the amount of CMM-managed memory used by the cache that does
    // not vary based on either page size or cache size
    DWORD CacheFixedOverhead();

    // returns the number of bytes of cache data per byte of cache metadata
    // this varies depending on page size & cache size
    DWORD CacheDataMetadataRatio();

    // returns TRUE if an SPS power cabling (cross-cable) problem has been
    // detected
    BOOL SpsPowerCablingError();

    // returns TRUE if an SPS serial cabling (cross-cable) problem has been
    // detected
    BOOL SpsSerialCablingError();

    // returns SP model number
    DWORD ModelNumber();

    //
    // Performance-related data
    //
    // These functions will always return zero if statistics logging is disabled.
    //

    // returns the hard error count
    DWORD HardErrorCount();

    // returns the number of times the write cache turned on flushing
    // due to reaching the high watermark
    DWORD HighWatermarkFlushOn();

    // returns the number of times the write cache turned on flushing
    // due to an idle unit
    DWORD IdleFlushOn();

    // returns the number of times the write cache turned off flushing
    // due to reaching the low watermark
    DWORD LowWatermarkFlushOff();

    // returns the number of write cache flushes performed
    DWORD WritecacheFlushes();

    // returns the number of write cache blocks flushed
    DWORD WriteBlocksFlushed();

    // returns the cache page size
    DWORD CachePageSize();

    // return low & high cache flush watermarks
    DWORD LowWatermark();
    DWORD HighWatermark();

    // The Chassis serial number length SYSTEM_ID_LENGTH
    DWORD GetSerialNumber(DWORD buffer_size, char *buffer);

    // returns the sum of the lengths of current request queues seen by each 
    // user request to this SP (from Flare's point of view).
    DWORD SumQueueLengthArrival();

    // returns the number of times a user request arrived while at least one 
    // other request was being processed.
    DWORD ArrivalsToNonZeroQueue();

    // returns the SPS test time
    void SpsTestTime(DWORD *day, DWORD *hour, DWORD *minute);

	//set SPS test time
	void SetSpsTestTime(DWORD day, DWORD hour, DWORD minute);

    // returns the Peer SP's boot state
    DWORD PeerBootState();

    // returns the Peer SP's boot fault code
    DWORD PeerBootFaultCode();

    // returns whether the Peer SP is in normal mode or not.
    BOOL IsPeerSpApplicationRunning();

    //returns the Management Port Speed Setting Supported Status
    BOOL ManagementPortSpeedSupported();

    //returns the Management Port Link Status
    BOOL  ManagementPortLinkStatus();

    // returns the SP's Management Port Requested Speed
    DWORD ManagementPortRequestedSpeed();

    // returns the SP's Management Port Requested Duplex Mode
    DWORD ManagementPortRequestedDuplexMode();

    // returns the SP's Management Port Current Speed
    DWORD ManagementPortSpeed();

    // returns the SP's Management Port Duplex Mode
    DWORD ManagementPortDuplexMode();

    // returns the SP's Management Port Auto Negotiate Mode
    BOOL ManagementPortAutoNegotiateMode();

    // returns the SP's Management Port Allowed Speeds
    UINT_32 ManagementPortAllowedSpeed();

protected:
    FbeLocalSpInfo *mpFbeLocalSPInfo;
};

class FbeEnclosureInfo;

class CSX_CLASS_EXPORT FlareEnclosure: public FlareArray
{
public:

    FlareEnclosure( FlareData *FdObj, K10_WWID wwn);

    ~FlareEnclosure(){}

    // Returns enclosure number, the pre-WWN index
    DWORD EnclosureNumber();

    DWORD Enclosure(); // The Enclosure index number

    K10_WWID EnclosureWWN();

    K10_WWID WWN();

    // Returns which bus the enclosure is on
    DWORD Bus();

    // Returns enclosure address, which is the value of the thumbwheel switch
    // EnclosureNumber is acceptable only for internal use in our classes.
    DWORD Address();

    // returns enclosure type (DPE, DAE, XPE)
    ADM_ENCL_TYPE EnclosureType();

    // returns list of drive types in an enclosure
    void EnclosureDriveType(CListPtr<DWORD> &cplDriveType);

    // returns enclosure maximum speed
    DWORD MaxSpeed();

    //returns enclosure current speed
    DWORD CurrentSpeed();

    // Useful for derived classes
    DWORD ComponentType() { return m_dwType; }

    // ID number of component. Especially useful for some derived classes.
    DWORD IdNum() { return m_dwComponentNumber; }

    /*
     * Enclosure relative count methods
     */
    // returns the number of Dimms in this enclosure
    DWORD EnclosureDimmCount();

     // returns the number of SSDs in this enclosure
    DWORD EnclosureSsdCount();

    // returns the number of LCCs in this enclosure
    DWORD EnclosureLccCount();

    /*
     * Enclosure relative count methods
     */
    // returns the number of SSCs in this enclosure
    DWORD EnclosureSSCCount();
    
    // Power supplies in the enclosure
    DWORD EnclosurePsCount();

    // Get number of fans
    DWORD EnclosureFanCount();

    DWORD EnclosureSpsCount();

    DWORD EnclosureDiskCount();
    DWORD MaxEnclDiskCount(DWORD dwEnclosure);

    // returns the number of SP slots in this enclosure
    // DPE-2, DAE-normally 0, except 2 in X-1 DAE0_0, XPE-2
    DWORD EnclosureSpCount();

    DWORD EnclosureIOModuleCount();

    DWORD EnclosureIOAnnexCount();

    DWORD EnclosureDisksPerBankCount( );
    
    // returns enclosure loop location
    DWORD LoopLocation();

    // returns enclosure chassis type (0=tower, 1=rackmount)
    int ChassisType();

    // returns enclosure state, see cm_environ_if.h for values
    int State();

    // returns TRUE if the flash masks have been sent to the LCC
    TRI_STATE FlashEnable();

    // returns the SP's current values for the flash masks (may not
    // correspond to the actual LCC settings)
    AdmFlashState FlashBit();
    AdmFlashState DiskFlashBits(int iDiskDriveSlot);

    // returns TRUE if the LCC is on the wrong loop
    BOOL CrossCable();

    // returns TRUE if the enclosure fault LED is lit
    BOOL Fault();

    // returns the enclosure fault LED reason code
    long FaultLedReason();

    // returns communications fault value
    BOOL LCCPeerToPeerCommunicationFault();

    // returns the fault symptom
    CM_ENCL_FAULT_SYMPTOM Symptom();

    //  Slot number - not useful for base class itself
    DWORD Slot();

    // SP Id - more useful for derived classes
    ADM_SP_ID GetSp();

    // Gen3 Power Supply methods.
    DWORD MaximumPower( );
    DWORD CurrentPower( );
    DWORD AveragePower( );
    DWORD PowerStatus( );
    DWORD MaximumAirInletTemperature( );
    DWORD CurrentAirInletTemperature( );
    DWORD AverageAirInletTemperature( );
    DWORD AirInletTemperatureStatus( );

    /*
     * Enclosure Resum� PROM
     */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC product serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC product part number string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC product revision number string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_PRODUCT_REVISION_SIZE)
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture(DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture(DWORD buffer_size, char *buffer);

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName(DWORD buffer_size,  char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

    //  Fills in the buffer with the Frumon revision for each enclosure. The Buffer_size 
    //  Parameter indicates the maximum size accepted by the caller. The return value indicates
    //  The actual number of bytes entered into the buffer. 
    DWORD FrumonRev(DWORD buffer_size, char *buffer);

    BOOL IsProcessorEncl();
	//Drive MidPlane Resume Prom Info
	DWORD DriveMPEmcPartNo( DWORD buffer_size, char *buffer );
	DWORD DriveMPEmcArtworkRevisionLevel(DWORD buffer_size, char *buffer);
	DWORD DriveMPEmcAssemblyRevisionLevel(DWORD buffer_size, char *buffer);
	DWORD DriveMPEmcSerialNumber(DWORD buffer_size, char *buffer);
	DWORD DriveMPEmcProductSerialNumber(DWORD buffer_size, char *buffer);
	DWORD DriveMPEmcProductPartNo( DWORD buffer_size, char *buffer );
	DWORD DriveMPEmcProductRevisionNo( DWORD buffer_size, char *buffer );
	DWORD DriveMPVendorName(DWORD buffer_size, char *buffer);
	DWORD DriveMPVendorPartNo(DWORD buffer_size, char *buffer);
	DWORD DriveMPVendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);
	DWORD DriveMPVendorSerialNumber(DWORD buffer_size, char *buffer);
	DWORD DriveMPVendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);
	DWORD DriveMPLocationOfManufacture(DWORD buffer_size, char *buffer);
	DWORD DriveMPDateOfManufacture(DWORD buffer_size, char *buffer);
	DWORD DriveMPAssemblyName(DWORD buffer_size,  char *buffer);;
	BOOL  DriveMPResumeFaulted();
	BOOL  DriveMPResumeAvailable();

	//Shutdown info
	DWORD EnclosureShutdownWarning(int index);
	DWORD EnclosureShutdownReason();
    // return whether faults were detected on the SP
    BOOL IsFaultStatusRegFault(int index);
    BOOL IsHwErrMonFault(int index);
	DWORD AmbientOvertempFault(int index);
	DWORD AmbientOvertempWarning(int index);
	DWORD Tap12VMissing(int index);
	BOOL  OverTempWarning();
	BOOL  OverTempFailure();

protected:
    K10_WWID    m_wwnEnclosure;
    K10_WWID    m_wwnObject;

    DWORD       m_dwBus,
                m_dwComponentNumber,
                m_dwEnclosure,
                m_dwSlot,
                m_dwType;
//  AdmSpId     m_dwSP;
    ADM_SP_ID   m_dwSP;

        // Returns component type
    DWORD DecodeWWN( K10_WWID wwn );
   
    void AddDriveTypeToList(CListPtr<DWORD> &cplDriveType, const DWORD dwFru);

	FbeEnclosureInfo	*mpFbeEnclosureInfo;
    };

class FbeDimmInfo;
	
class CSX_CLASS_EXPORT FlareDimm: public FlareEnclosure
{
public:

    FlareDimm( FlareData *FdObj, K10_WWID wwn );
    ~FlareDimm() {}

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD State();
    DWORD SubState();
    DWORD Size();
    DWORD SpId();
    DWORD SlotId();
    DWORD IsLocalDimm();

/*
 * DIMM Resume
 */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer );

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer );

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables() { return 0; }

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable,
                                  DWORD buffer_size,
                                  char *buffer) { return 0; }

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable,
                                      DWORD buffer_size,
                                      char *buffer) { return 0; }

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted()  { return 0; }
    
protected:
	FbeDimmInfo *mpFbeDimmInfo;
};

class FbeSsdInfo;
	
class CSX_CLASS_EXPORT FlareSsd: public FlareEnclosure
{
public:

    FlareSsd( FlareData *FdObj, K10_WWID wwn );
    ~FlareSsd() {}

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD State();
    DWORD SubState();
    DWORD Size();
    DWORD SpId();
    DWORD SlotId();
    DWORD IsLocalSsd();

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD FirmwareRevision( DWORD buffer_size, char *buffer );

/*
 * SSD Resume
 */
    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer );

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer) { return 0; }

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer.
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables() { return 0; }

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable,
                                  DWORD buffer_size,
                                  char *buffer) { return 0; }

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable,
                                      DWORD buffer_size,
                                      char *buffer) { return 0; }

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted()  { return 0; }

    
protected:
	FbeSsdInfo *mpFbeSsdInfo;
};

class FbeLCCInfo;
	
class CSX_CLASS_EXPORT FlareLcc: public FlareEnclosure
{
public:

    FlareLcc( FlareData *FdObj, K10_WWID wwn );
    ~FlareLcc() {}

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD Number();
    DWORD Address();

    // returns the microcode revision of this LCC  -- NOT Supported
//  DWORD MicrocodeRevision();

    // fills in the buffer with the serial number of this LCC
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_SERIAL_NUMBER_SIZE)
//  DWORD SerialNumber( char *buffer, DWORD buffer_size);  -- NOT supported: use EmcSerialNumber()

    // returns TRUE if this LCC is physically last in the loop
    BOOL ExpansionPortOpen();

    // returns TRUE if status has been read from the peer LCC
    // in that case, only the fault and inserted flags are valid
    BOOL PeerInformationBit();

    // returns TRUE if this LCC has a primary media converter fault
    BOOL PrimaryMediaConverterFault();

    // returns TRUE if this LCC has a primary media converter fault
    BOOL ExpansionPortMediaConverterFault();

    // returns TRUE if this LCC is logically last in the loop
    BOOL Shunted();

    // returns TRUE if this LCC is faulted
    BOOL Faulted();

    // returns TRUE if this LCC is inserted
    BOOL Inserted();

    DWORD State();
    DWORD SubState();

    // returns the upgrade operation current state 
    DWORD CurrentState();

    // returns the LCC upgrade state for an enclosure
    DWORD LccUpgradeState();


    //returns the Maximum Speed of the LCC
    DWORD MaxSpeed();

    //returns the Current Speed of the LCC
    DWORD CurrentSpeed();

    //returns the LCC type
    DWORD Type();

/*
 * LCC Resum�
 */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer );

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer );

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable,
                                  DWORD buffer_size,
                                  char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable,
                                      DWORD buffer_size,
                                      char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

    void ExpanderFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);

    //returns the LCC Firmware Revision prior to upgrade
    void PrevFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);

    //returns the LCC Current Firmware Revision
    void FirmwareRevision(char *pStrFirmwareRev, int expanderNumber);

    //returns the LCC Upgrading Firmware Revision
    void UpgradingFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);

    //returns TRUE if LCC Firmware Upgrade is in progress
    BOOL UpgradeInProgress(int expanderNumber);

    //returns the LCC Firmware Upgrade Status
    DWORD UpgradeStatus(int expanderNumber);

    //returns the LCC Firmware Upgrade Work State
    DWORD WorkState();

    //returns the LCC Expander Count
    DWORD ExpanderCount();

protected:
	FbeLCCInfo *mpFbeLCCInfo;
};

class FbeSSCInfo;
class CSX_CLASS_EXPORT FlareSSC: public FlareEnclosure
{
public:

    FlareSSC( FlareData *FdObj, K10_WWID wwn );
    ~FlareSSC() {}
    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    //DWORD Number();
    
    
    // returns TRUE if this SSC is faulted
    BOOL Faulted();

    // returns TRUE if this SSC is inserted
    BOOL Inserted();
    
    //returns the SSC type
    DWORD Type();
    
protected:
    FbeSSCInfo *mpFbeSSCInfo;
};
class FbePSInfo;

class CSX_CLASS_EXPORT FlarePowerSupply: public FlareEnclosure
{
public:
    FlarePowerSupply( FlareData*FdObj, K10_WWID wwn );
    ~FlarePowerSupply() {};

    DWORD Enclosure();
    DWORD Bus();
    DWORD Number();
    DWORD Address();

    DWORD State();
    DWORD SubState();
    DWORD ACDCInput();

    // returns TRUE if this power supply has a thermal fault
    BOOL ThermalFault();

    // returns TRUE if this power supply has shutdown
    BOOL Shutdown();

    // returns TRUE if this power supply is faulted
    BOOL Faulted();

    // returns TRUE if this power supply is inserted
    BOOL Inserted();

    // returns TRUE if this power supply is supplying 12V power
    TRI_STATE Is12vOk();

    // returns TRUE if AC power to this power supply has failed
    BOOL AcFail();

	// returns TRUE is the Power Supply has a faulted fan
    BOOL FanFaulted();

	// returns TRUE is the Power Supply has a logical fault
    BOOL LogicalFaulted();

    // Returns the physical slot associated with this PS
    ENCLOSURE_FRU_SLOT Slot();

    // Returns the physical slot associated with this PS relative to the SP
    ENCLOSURE_FRU_SLOT SpSlot();

    // Returns the interpreted value of whether or not the PS is 
    // associated with an SP.
    ENCLOSURE_SP_ID associatedSP();

    //Returns TRI_INVALID when Fully Seated Fault not supported
    ENCLOSURE_STATUS FullySeatedFault();

    //Returns TRUE if there was a partial data fault
    ENCLOSURE_STATUS PartialDataFault();

    //Returns TRUE if there was a Peer data fault
    ENCLOSURE_STATUS PeerFault();

    // Returns TRUE if Primary path Data is Valid
    BOOL PrimaryPathDataValid();

    // Returns TRUE if Secondary path Data is Valid
    BOOL SecondaryPathDataValid();

    // Return SMBus Access Fault
    DWORD SMBusAccessFault();

        // returns TRUE if this power supply is supported
    BOOL Supported();

    /*
     * Power Supply Resum�
     */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size,  char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber( DWORD buffer_size, char *buffer );

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer );

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

    void FirmwareRevision(char *pFirmwareRevision);

protected:
	FbePSInfo *mpFbePSInfo;

};

class FbeFanInfo;

class CSX_CLASS_EXPORT FlareFan: public FlareEnclosure
{
public:
    FlareFan( FlareData *FdObj, K10_WWID wwn );
    ~FlareFan() {};

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD SpSlot();
    DWORD Location();
    DWORD Number();
    DWORD Address();

    DWORD State();
    DWORD SubState();

    ENCLOSURE_SP_ID AssociatedSP();
    DWORD LogicalNumber();
    BOOL IsResumeSupported();

        // returns TRUE if this fan pack has a single fan fault
    BOOL SingleFault();

    // returns TRUE if this fan pack has a multiple fan fault
    BOOL MultipleFault();

    // returns TRUE if this fan pack is inserted
    TRI_STATE Inserted();

    // Returns validity of the Flare Fan data
    BOOL ValidData();

    // Returns SMBus Access Fault
    DWORD SMBusAccessFault();

   /*
     * Fan Resum�
     */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size,  char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }


    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer );

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

protected:
    FbeFanInfo *mpFbeFanInfo;
};

class FbeVBusInfo;

// FlareBusInfo (VBus) class
// Since Enclosures are associated with VBuses, don't make this class a derived class of FlareEnclosure
class CSX_CLASS_EXPORT FlareBusInfo: public FlareArray
{
public:
	FlareBusInfo( FlareData *FdObj, K10_WWID wwn );
    FlareBusInfo( FlareData *FdObj );

    ~FlareBusInfo() {}

    // Returns the number of VBus'es found on the array.
    DWORD GetNumberBuses() { return m_dwNumBuses; }

    // Returns the VBus BusNumber
    DWORD GetBusNum(DWORD ii);

    // Returns the Link State of the specified VBus.
    DWORD GetLinkState(DWORD busNum);

    // Returns the current speed that the given VBus is running at.
    DWORD GetCurrentSpeed(DWORD busNum);

    // Returns a bitmap that contains the speeds that the 
    // given VBus is capable of running
    DWORD GetCapableSpeeds(DWORD busNum);

    //Fetch Key for the given VBus.
    HRESULT VbusKey( K10_WWID &Key, DWORD busNum );

    // Return a list of enclosure numbers that are on the given Bbus.
    // An enclosure with a duplicate address won't be in this list since it
    // will be in the CM_ENCL_MISSING state.
    HRESULT EnclosureNumberList(DWORD **ppNumber, DWORD &dwActualNumber, DWORD Bus );

    // Return the enclosure address for thes enclosure nbased in the 
    // given enclosure number
    DWORD EnclosureAddress(DWORD EnclosureNumber);

    // returns TRUE if this VBus has enclosures with duplicate addresses
    BOOL  DuplicateAddresses(DWORD busNum);

    // Returns the number of enclosures found with an address that matches the 
    // address of the given enclosure number. (Will always return at least 1).
    DWORD DuplicateCount(DWORD EnclosureNumber);

    // Returns the SFP condition on the specified VBus
    DWORD GetSfpCondition(DWORD busNum);

    // Returns whether the maximum number of enclosures on the bus has been
    // exceeded.
    BOOL GetMaxEnclosuresExceeded(DWORD busNum);

    // Returns the maximum number of enclosures allowed on the bus.
    DWORD MaxEnclosures(DWORD busNum);

    // Returns the current number of enclosures on the bus.
    DWORD NumberOfEnclosures(DWORD busNum);

	//Returns port combination status for current VBUS
	BOOL PortCombinationStatus();

protected:
	FbeVBusInfo *mpFbeVBusInfo;

private:
    DWORD       m_dwNumBuses;
};

class FbeSPSInfo;

class CSX_CLASS_EXPORT FlareSps: public FlareEnclosure
{
public:

    FlareSps( FlareData *FdObj, K10_WWID wwn );
    ~FlareSps() {};

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD SpSlot();
    DWORD Number();
    DWORD Address();

    // Returns the interpreted value of whether or not the SPS is 
    // associated with an SP.
    ENCLOSURE_SP_ID AssociatedSP();

    // returns TRUE if this SPS has an AC line fault
    BOOL AcLineFault();

    // returns TRUE if this SPS is charging
    BOOL Charging();

    // returns TRUE if this SPS has a low battery
    BOOL LowBattery();

    // returns TRUE if this SPS has failed or its lifetime period has
    // expired
    BOOL Replace();

    // returns TRUE if this SPS is faulted
    BOOL Faulted();

    // returns TRUE if this SPS is inserted
    BOOL Inserted();

    // State of SPS
    ADM_SPS_STATE BatteryState();

    // State & SubState of SPS
    DWORD State();
    DWORD SubState();
 
    // returns SPS Type
    DWORD Type();

    // Validity of SPS cabling
    ADM_SPS_CABLING Cabling();

    // Gen3 Power Supply methods.
    DWORD MaximumPower( );
    DWORD CurrentPower( );
    DWORD AveragePower( );
    DWORD PowerStatus( );

    BOOL IsResumeAccessible();

    /*
     * SPS Resum�
     */

    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer );

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size,  char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer );

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer );

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer );

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();
    

protected:
    FbeSPSInfo *mpFbeSPSInfo;

};

class FbeDiskInfo;

class CSX_CLASS_EXPORT FlareDisk: public FlareEnclosure
{
public:

    FlareDisk( FlareData *FdObj, K10_WWID wwn );
    ~FlareDisk() {}

    /*
     ***************** Disk Drive Methods ******************************
     * These routines were originally relative to enclosure
     */

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD Number();
    DWORD Address();

    // returns TRUE if the loop A status of the disk drive in this slot is valid
    BOOL LoopAValid();

    // returns TRUE if the loop B status of the disk drive in this slot is valid
    BOOL LoopBValid();

    // returns TRUE if the drive in this slot has been taken off loop A
    BOOL LoopABypass();

    // returns TRUE if the drive in this slot has been taken off loop B
    BOOL LoopBBypass();

    // returns TRUE if the drive in this slot has requested a bypass on
    // loop A
    BOOL BypassRequestedLoopA();

    // returns TRUE if the drive in this slot has requested a bypass on
    // loop B
    BOOL BypassRequestedLoopB();

    // returns TRUE if the drive in this slot has its fault LED on
    BOOL Faulted();

    // *********** FRU Methods *************************************
    // Rename to Drive... for clarity

    // returns ODFU Disk Status
    DWORD ODFUDiskStatus();

    // returns ODFU Disk fail code
    DWORD ODFUDiskFailCode();

    // returns ODFU Prior Disk Firmware Revision
    DWORD ODFUPriorDiskFWRev(DWORD buffer_size, char *buffer);

    // returns the FRU state, see sunburst.h for values
    DWORD State();

    // returns the FRU type
    DWORD DriveType();

    // returns the Maximum Speed of the drive
    DWORD DriveMaxSpeed();

    // returns total number of drive sectors
    // Could this overflow someday??  --- NOT in FLARE SPEC!!!
    LBA_T TotalSectors();

    // WWN name for RAID group containing disk.  If none, returns FALSE
    BOOL RaidGroup( K10_WWID &wwn );

    // returns the number of blocks reserved for use by the SP
    LBA_T PrivateSpaceSectors();

    // Not currently reported by Flare
    // returns the number of blocks reserved for use by the SP
    LBA_T UnusableSectors();

    DWORD RaidGroup();

    // returns TRUE if the vendor ID, product ID, and product revision level
    // are valid
    BOOL InquiryBlockValid();

    // fills in the buffer with the vendor ID string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (ADM_PG34_VENDOR_ID_LEN)
    DWORD VendorId( DWORD buffer_size, char *buffer);

    // fills in the buffer with the product ID string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (ADM_PG34_PRODUCT_ID_LEN)
    DWORD ProductId( DWORD buffer_size, char *buffer);

    // fills in the buffer with the product revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (ADM_PG34_PRD_REV_LEN)
    DWORD ProductRev( DWORD buffer_size,  char *buffer);

    // fills in the buffer with the additional inquiry info (EMC part number) string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (ADM_PG34_ ADTL_INQ_LEN)
    DWORD AddlInquiryInfo( DWORD buffer_size, char *buffer);

    // returns TRUE if this is an expansion disk
    BOOL ExpansionDisk();

    // returns TRUE if this disk is bound as a hot spare
    BOOL HotSpare();

    // Returns wwn of Hot Spare replacing this disk. 
    // If disk is not replaced, returns FALSE
    BOOL ReplacedbyHotSpare(K10_LU_ID &wwn);

        // returns TRUE if this disk needs formatting
    BOOL FormattedState();
    // returns RPM value for the disk
    UINT_32 DriveRPM();
    // returns inserted bit for disk
    UINT_32 DiskInserted();
    // returns disk obit reason code
    UINT_32 DriveObit();
    // returns the disk end of life state
    BOOL EndOfLifeState();
    // returns disk EQZ
    DWORD PctEQZ();
    // returns TRUE if this disk contains a level 2 cache partition
    BOOL Level2Cache();

    // fills in the buffer with the serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (up to LOG_DE_PROD_SERIAL_SIZE, but may be less)
    DWORD SerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the serial number string
    // the buffer_size param indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into 
    // the buffer (up to LOG_DE_TLA_PART_NUM_SIZE, but may be less)
    DWORD TlaNumber(DWORD buffer_size, char *buffer);

    // See Page 3C doc
    ADM_LBA ZeroMark();

    DWORD PctZeroed();

    DWORD PctCopied();

    // FRU bind signature
    DWORD BindSignature();

    /*
     * FRU Performance routines
     */

    // returns the number of hard read errors on this FRU
    DWORD HardReadErrors();

    // returns the number of hard write errors on this FRU
    DWORD HardWriteErrors();

    // returns the number of soft read errors on this FRU
    DWORD SoftReadErrors();

    // returns the number of soft write errors on this FRU
    DWORD SoftWriteErrors();

    // returns the average disk request service time for this FRU
    DWORD AverageServiceTime();

    // returns the average disk sector address difference for this FRU
    DWORD AverageAddressDiff();

    // returns the number of read operations for this FRU
    DWORD ReadCount();

    // returns the number of write operations for this FRU
    DWORD WriteCount();

    // returns the number of read retries for this FRU
    DWORD ReadRetries();

    // returns the number of write retries for this FRU
    DWORD WriteRetries();

    // returns the number of remapped sectors for this FRU
    DWORD NumRemappedSectors();

    // returns the number of sectors read from this FRU
    DWORD BlocksRead();

    // returns the number of sectors written to this FRU
    DWORD BlocksWritten();

    // returns the number of sectors seeked while accessing this FRU
    ULONGLONG SumOfBlocksSeeked();

    // returns the sum of the queue lengths seen by requests arriving for
    // this FRU
    DWORD SumQueueLenOnArrival();

    // returns the number of requests which encountered at least one other
    // request in progress on arrival
    DWORD ArrivalsToNonzeroQ();

    // returns the number clock ticks this FRU was busy
    DWORD BusyTicks();

    // returns the number clock ticks this FRU was idle
    DWORD IdleTicks();

    // Indicates whether this disk needs to be rebuilt for a Raid Group
    // that has had a disk fail or removed.
    BOOL NeedsRebuild();

    // returns whether current disk is foreign or critical to this slot.
    BOOL CriticalOrForeignDisk(
           DWORD buffer_size, 
           char *expectedDiskSerialNumber, 
           char *insertedDiskSerialNumber);

    // If current disk is missing from this slot and is a member of a Raid Group,
    // this method returns the serial number of the missing disk.
    BOOL MissingDisk(
           DWORD buffer_size, 
           char *missingDiskSerialNumber);

    // returns the powersavings state
    DWORD PowerSavingsState();

    // returns the number of spinning ticks
    DWORD SpinningTicks();

    // returns the number of standby ticks
    DWORD StandByTicks();

    // returns the number of spinups
    DWORD SpinUps();

    // checks powersavings capable 
    BOOL IsPowerSavingsCapable();

    // is qualified for spindown 
    BOOL IsSpinDownQualified();

	DWORD DiskPool();

	BOOL MCR_hidden();

protected:
	FbeDiskInfo	*mpFbeDiskInfo;
};

class FbeDiskPoolInfo;

class CSX_CLASS_EXPORT FlareDiskPool: public FlareArray
{
public:

    FlareDiskPool( FlareData *FdObj, K10_WWID wwn );
    virtual ~FlareDiskPool() {}

    DWORD IdNumber();

    // Get list of disk drives in a Disk Pool 
    HRESULT DiskList( NAPtr<K10_WWID> &pWwnDisk, DWORD &dwNumberOfDisks,
		NAPtr<K10_WWID> &pWwnRG, DWORD &dwNumberOfRGs);

protected:
	FbeDiskPoolInfo	*mpFbeDiskPoolInfo;
};

        // **************** SP Methods *******************************************
class FbeSPInfo;

class CSX_CLASS_EXPORT FlareSP: public FlareEnclosure
{
public:
    FlareSP( FlareData *FdObj, K10_WWID wwn );

    virtual ~FlareSP() {};

    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD Address();

    // Return which sp we are set for
    ADM_SP_ID GetSpNum();

    // TRUE if the SP really isn't there
    BOOL IsMissing();

    BOOL IsPeerInserted();

    DWORD GetState();
    DWORD GetSubState();

    // returns the status of engine id fault
    BOOL EngineIdFault();

    // returns number of write cache pages
    DWORD WriteCachePages();

        // returns current write cache state, see ca_states.h
    DWORD WriteCacheState();

    // returns number of unassigned write cache pages
    DWORD UnassignedPages();

    // returns TRUE if the read cache for this SP is ON
    BOOL ReadCacheEnable();

    // returns current read cache state (see ca_states.h for values)
    DWORD ReadCacheState();

    // returns physical memory size in MB
    DWORD PhysicalMemorySize();

    // return max Read Cache size
    DWORD MaxReadCacheSize();

    // returns the top of CMM-managed memory in megabytes
    DWORD CmmControlPoolTop();

    // returns the bottom of CMM-managed memory (aka MAXMEM) in megabytes
    DWORD CmmControlPoolBottom();

    // returns the amount of CMM-managed memory used, not including cache
    // control structures (in megabytes)
    DWORD CmmControlNonCacheUsed();

    // returns read cache size in MB
    DWORD ReadCacheSize();

    // Return memory available for cache in MB
    DWORD PhysicalCacheSize();

    // Return data pool memory used by others in MB
    DWORD ExternalDataPoolUsersSize();

    // returns the WWN seed from the enclosure resum� PROM
    DWORD WWNseed();

    // returns whether a temperature sensor is faulted.
    BOOL TemperatureSensorFault(BOOL bThisSP);

    // returns whether a temperature sensor is faulted.
    BOOL AmbientTemperatureFault(BOOL bThisSP);

    // returns whether the SP is in the process of shutting down.
    BOOL ShuttingDown(BOOL bThisSP);

    //
    // SP Resum� PROM
    //
    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer);

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                                    char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                                            char *buffer);

    // the return value indicates the number of MAC addresses listed in the
    // resum� PROM
    DWORD NumMacAddresses();

    // Fills in the buffer with the requested MAC address, in ASCII
    // The mac_address parameter indicates which MAC address is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_MAC_ADDRESS_SIZE)
    DWORD MacAddress( DWORD mac_address, DWORD buffer_size,
                                        char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

    //  Function indicates that, it is safe to remove sp or not
    BOOL UnsafeToRemoveSP();

    // Virtual SAS Expander info 
    long LccFirmwareRev(long lccIndex, long buffer_size, char *buffer);
    bool LccFaulted(long lccIndex);
    bool LccIsLocal(long lccIndex);
    bool LccIsInserted(long lccIndex);

protected:
    FbeSPInfo *mpFbeSPInfo;
    BOOL m_dwMissing;   // SP is not really present - missing or powered off

};

class FbeSubFRUInfo;

class CSX_CLASS_EXPORT FlareSubFRU: public FlareEnclosure
{
public:
    FlareSubFRU( FlareData *FdObj, K10_WWID wwn );
    virtual ~FlareSubFRU() {};

    // Type of SubFRU
    DWORD getType();

    // The slot number of this SubFRU, relative to its slot number type.
    DWORD getSlot();

    // returns TRUE if this SubFRU is inserted
    DWORD Inserted();

    // returns TRUE if this SubFRU has Resume Prom data
    BOOL IsResumeSupported();

    // returns SubFRU state, see adm_sp_api.h for values
    DWORD State();

    // Return SMBus Access Fault
    DWORD SMBusAccessFault();

    //
    // FlareSubFRU Resum� PROM
    //
    // fills in the buffer with the EMC part number string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo(DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer);

    // fills in the buffer with the EMC serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // fills in the buffer with the vendor name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the location of manufacture string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the date of manufacture string (yyyymmdd)
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture( DWORD buffer_size, char *buffer);

    // fills in the buffer with the assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName( DWORD buffer_size, char *buffer);

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // Fills in the buffer with the name of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    // Fills in the buffer with the revision of the requested programmable
    // The programmable parameter indicates which programmable is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    // the return value indicates the number of MAC addresses listed in the
    // resum� PROM
    DWORD NumMacAddresses();

    // Fills in the buffer with the requested MAC address, in ASCII
    // The mac_address parameter indicates which MAC address is requested
    // The buffer_size parameter indicates the maximum size accepted by the caller
    // The return value indicates the actual number of bytes entered into
    // the buffer (ADM_MAC_ADDRESS_SIZE)
    DWORD MacAddress( DWORD mac_address, DWORD buffer_size,
                           char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();

protected:

	FbeSubFRUInfo *mpFbeSubFRUInfo;
};

class CSX_CLASS_EXPORT FlarePersonality: public FlareLocalSP
{
public:
    FlarePersonality( FlareData *FdObj);

    ~FlarePersonality() {};

    HRESULT PersonalityKey( K10_WWID &Key );

    // Fills in the buffer with the personality card EMC part number string
    // The buffer_size parameter indicates the maximum size accepted by the 
    // caller The return value indicates the actual number of bytes entered 
    // into the buffer (ADM_EMC_PART_NUMBER_SIZE)
    DWORD EmcPartNo(DWORD buffer_size, char *buffer);

    // Fills in the buffer with the personality card EMC artwork revision
    // level string. The buffer_size parameter indicates the maximum size 
    // accepted by the caller. The return value indicates the actual number 
    // of bytes entered into the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD EmcArtworkRevisionLevel(DWORD buffer_size,  char *buffer);

    // Fills in the buffer with the personality card EMC assembly revision
    // level string. The buffer_size parameter indicates the maximum size 
    // accepted by the caller. The return value indicates the actual number 
    // of bytes entered into the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD EmcAssemblyRevisionLevel(DWORD buffer_size,  char *buffer);

    // Fills in the buffer with the personality card EMC serial number string
    // The buffer_size parameter indicates the maximum size accepted by the 
    // caller. The return value indicates the actual number of bytes entered 
    // into the buffer (ADM_EMC_SERIAL_NO_SIZE)
    DWORD EmcSerialNumber(DWORD buffer_size,  char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    // Fills in the buffer with the personality card vendor name string
    // The buffer_size parameter indicates the maximum size accepted by the 
    // caller. The return value indicates the actual number of bytes entered 
    // into the buffer (ADM_VENDOR_NAME_SIZE)
    DWORD VendorName(DWORD buffer_size,  char *buffer);

    // fills in the buffer with the vendor part number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor artwork revision level string
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ARTWORK_REVISION_SIZE)
    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor serial number string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    // fills in the buffer with the vendor assembly revision level string
    // the buffer_size parameter indicates the maximum size accepted by the caller
    // the return value indicates the actual number of bytes entered into
    // the buffer (ADM_ASSEMBLY_REVISION_SIZE)
    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    // Fills in the buffer with the personality card location of manufacture
    // string. The buffer_size parameter indicates the maximum size accepted 
    // by the caller. The return value indicates the actual number of bytes 
    // entered into the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
    DWORD LocationOfManufacture(DWORD buffer_size, char *buffer);

    // Fills in the buffer with the personality card date of manufacture
    // string (yyyymmdd). The buffer_size parameter indicates the maximum 
    // size accepted by the caller. The return value indicates the actual 
    // number of bytes entered into the buffer (ADM_DATE_MANUFACTURE_SIZE)
    DWORD DateOfManufacture(DWORD buffer_size, char *buffer);

    // Fills in the buffer with the personality card assembly name string
    // the buffer_size parameter indicates the maximum size accepted by the 
    // caller. The return value indicates the actual number of bytes entered 
    // into the buffer (ADM_ASSEMBLY_NAME_SIZE)
    DWORD AssemblyName(DWORD buffer_size, char *buffer);

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();
    
};

class CSX_CLASS_EXPORT FlareIOAnnex: public FlareEnclosure
{
    // Constuctor and Destructor:
public:
    FlareIOAnnex( FlareData *FdObj, K10_WWID wwn );
    virtual ~FlareIOAnnex() {};

    // Returns I/O Annex inserted state
    DWORD Inserted();

    // Returns I/O Annex power state
    DWORD PowerState();

    // Returns I/O Annex state
    DWORD State();

    // Returns I/O Annex firmware revision
    void FirmwareRevision(char *pStrFirmwareRev);

    // Return SMBus Access Fault
    DWORD SMBusAccessFault();

    // Returns BOOL value to indicate Resume Prom support
    BOOL IsResumeSupported();

    //
    // I/O Annex Resum� PROM
    //
    // All Resume Prom functions listed below 
    // fill the buffer with requested Resume Prom field
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer
    DWORD EmcPartNo(DWORD buffer_size, char *buffer);

    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer);

    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer);

    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    DWORD VendorName( DWORD buffer_size, char *buffer);

    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    DWORD DateOfManufacture( DWORD buffer_size, char *buffer);

    DWORD AssemblyName( DWORD buffer_size, char *buffer);

    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    DWORD MacAddress( DWORD mac_address, DWORD buffer_size,
                           char *buffer);

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // the return value indicates the number of MAC addresses listed in the
    // resum� PROM
    DWORD NumMacAddresses();

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();
};

class FbeIOMInfo;

class CSX_CLASS_EXPORT FlareIOModule: public FlareEnclosure
{
    // Constuctor and Destructor:
public:
    FlareIOModule( FlareData *FdObj, K10_WWID wwn );
    virtual ~FlareIOModule() {};

    // Member functions:
    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD Number();
    DWORD Address();

    // Returns the IOM type
    DWORD Type();

   // Returns the IOM class
    DWORD Class();

    // Returns TRUE if IOM is inserted
    BOOL Inserted();

    // Returns TRUE if peer has mezzanine and if it is inserted
    BOOL PeerMezzanineInserted();

    // Returns TRUE if IOM is plugged into Carrier
    BOOL InCarrier();

    // Returns IOM marked state (TRUE=IOM (All Ports) marked, FLASE=IOM not marked)
    BOOL FlashEnable();

    // Returns IOM state 
    DWORD IOMState();

    // Returns IOM substate 
    DWORD IOMSubState();

    // Returns IOM Power State
    DWORD IOMPowerStatus();

    // Returns IOM Fault LED state
    BOOL IOMFaultLED();

    // Return port count within IOM
    DWORD PortCount();

    DWORD LabelName(DWORD buffer_size, char *buffer);

    // Return Flash state of individual ports within IOM
    BOOL PortFlashState(DWORD dwPortNumber);

    // Return SMBus Access Fault
    DWORD SMBusAccessFault();

    // Returns BOOL value to indicate Resume Prom support
    BOOL IsResumeSupported();

    // Returns expected I/O Module SLIC types (bitmask value)
    DWORD ExpectedSLICTypes();

    // Returns I/O Module current SLIC type
    DWORD IOMSLICType();

    // Returns I/O Module FRU Id
    DWORD IOMFRUId();

    // Returns a unique ID for this I/O Module
    DWORD UniqueID( );

    //I/o Module Resum� Data
    // All Resume Prom functions listed below 
    // fill the buffer with requested Resume Prom field
    // the buffer_size parameter indicates the maximum size accepted by the
    // caller
    // the return value indicates the actual number of bytes entered into
    // the buffer
    DWORD EmcPartNo(DWORD buffer_size, char *buffer);

    DWORD EmcArtworkRevisionLevel( DWORD buffer_size, char *buffer);

    DWORD EmcAssemblyRevisionLevel( DWORD buffer_size, char *buffer);

    DWORD EmcSerialNumber( DWORD buffer_size, char *buffer);

    // these functions just return 0, to indicate the component 
    // does not have "product" values
    DWORD EmcProductSerialNumber(DWORD buffer_size, char *buffer) { return 0; }
    DWORD EmcProductPartNo( DWORD buffer_size, char *buffer ) { return 0; }
    DWORD EmcProductRevisionNo( DWORD buffer_size, char *buffer ) { return 0; }

    DWORD VendorName( DWORD buffer_size, char *buffer);

    DWORD VendorPartNo( DWORD buffer_size, char *buffer);

    DWORD VendorArtworkRevisionLevel(DWORD buffer_size, char *buffer);

    DWORD VendorSerialNumber(DWORD buffer_size, char *buffer);

    DWORD VendorAssemblyRevisionLevel(DWORD buffer_size, char *buffer);

    DWORD LocationOfManufacture( DWORD buffer_size, char *buffer);

    DWORD DateOfManufacture( DWORD buffer_size, char *buffer);

    DWORD AssemblyName( DWORD buffer_size, char *buffer);

    DWORD ProgrammableName( DWORD programmable, DWORD buffer_size,
                                 char *buffer);

    DWORD ProgrammableRevision( DWORD programmable, DWORD buffer_size,
                                     char *buffer);

    DWORD MacAddress( DWORD mac_address, DWORD buffer_size,
                           char *buffer);

    // the return value indicates the number of programmables listed in the
    // resum� PROM
    DWORD NumProgrammables();

    // the return value indicates the number of MAC addresses listed in the
    // resum� PROM
    DWORD NumMacAddresses();    

    //  This function returns TRUE it the last resume PROM read was not successful and FALSE
    //  if the last read was successful. If TRUE is returned than CM is lighting a
    //  FLT light.
    BOOL ResumeFaulted();
    DWORD LinkState();
protected:
    FbeIOMInfo* mpFbeIOMInfo;

private:
    DWORD m_dwUniqueID;
};


class FbeFPInfo;

class CSX_CLASS_EXPORT FlareFlexport: public FlareEnclosure
{
    // Constuctor and Destructor:
public:
    FlareFlexport( FlareData *FdObj, K10_WWID wwn );
    virtual ~FlareFlexport() {};

    // Member Functions:
    DWORD Enclosure();
    DWORD Bus();
    DWORD Slot();
    DWORD Number();
    DWORD Address();

    // Returns IOM slot number
    DWORD IOMSlotNumber(DWORD dwPortLogicalNo, DWORD dwPortRole);

    // Returns port physical number
    DWORD PortPhysicalNumber(DWORD dwPortLogicalNo, DWORD dwPortRole);

    // Returns role of the port
    DWORD PortRole();

    // Returns sub role of the port
    DWORD PortSubRole(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns protocol of the port
    DWORD PortProtocol(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns Flexport state
    DWORD PortState(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns Flexport sub state
    DWORD PortSubState(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns Flexport SFP state
    DWORD PortSFPState(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns Flexport IOM Group
    DWORD PortIOMGroup(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    HRESULT IOModuleWWN(DWORD dwIOMSlot, K10_WWID &wwn);

    BOOL IsIOMInserted(DWORD dwIOMSlot);

    // Returns supported Port SFP speeds (bitmask value)
    DWORD SupportedSFPSpeeds(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns supported Port SFP protocols (bitmask value)
    DWORD SupportedSFPProtocols();

    // Returns BOOL value to indicate sfp support
    BOOL IsSFPSupported(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Returns supported I/O Module speeds (bitmask value)
    DWORD SupportedFPSpeeds(DWORD dwIOMSlot, DWORD dwPortNoInIOM);

    // Fills in SFP EMC Part Number
    DWORD SFPEMCPartNumber(DWORD buffer_size, char *buffer, DWORD dwPortNoInIOM);

    // Fills in SFP EMC Serial Number
    DWORD SFPEMCSerialNumber(DWORD buffer_size, char *buffer, DWORD dwPortNoInIOM);

    // Fills in SFP Vendor Part Number
    DWORD SFPVendorPartNumber(DWORD buffer_size, char *buffer, DWORD dwPortNoInIOM);

    // Fills in SFP Vendor Serial Number
    DWORD SFPVendorSerialNumber(DWORD buffer_size, char *buffer, DWORD dwPortNoInIOM);

    // Returns the logical port number
    DWORD GetLogicalPortNumber();

    // Returns a unique ID for this Flexport
    DWORD UniqueID( );

protected:
    FbeFPInfo* mpFbeFPInfo;

private:
    DWORD m_dwUniqueID;
};

//**********************************************************************
//              Logical Object classes start here
//**********************************************************************


    // ************ RAID Group Class **********************************

class FbeRGInfo;

class CSX_CLASS_EXPORT FlareRaidGroup: public FlareArray
{
public:
    FlareRaidGroup( FlareData *FdObj, K10_WWID wwn );

    virtual ~FlareRaidGroup() {};

    // Get list of LUNs in a RAID group 
    HRESULT LunList( NAPtr<K10_LU_ID> &pWwnLun, DWORD &dwNumberOfLuns );

    // Get list of disk drives in a RAID group 
    HRESULT DiskList( NAPtr<K10_WWID> &pWwnDisk, DWORD &dwNumberOfDisks );

    // Check Whether all FRUs in a RG are PowerSavings Capable 
    HRESULT PowerSavingsCapableDisks( BOOL &bPowerSavingsCapable );

    // RAID group ID number
    DWORD IdNumber();

    // Status of RG
    DWORD State();
    
	// Raid Type
    MMAN_RAID_TYPE RaidType();

    // Lun count in RG
    DWORD LunCount();

    // returns the number of logical blocks available on this RAID group.
    // depends on RAID group type & thus indetermite until one LUN is defined.
    LBA_T LogicalCapacity();

    // returns the number of free sectors available on this RAID group
    LBA_T FreeCapacity();

    // returns the total number of sectors available on this RAID group
    LBA_T PhysicalCapacity();

    // returns the number of sectors available in the largest free space on
    // this RAID group
    LBA_T LargestContigFreeSpace();

    // returns percentage complete for expansion or defragmentation (0-99)
    // returns ADM_INVALID_PERCENTAGE if no expansion or defragmentation
    // is in progress
    DWORD PercentExpanded();

    DWORD ExpansionRate();

    DWORD Hi5State();

    BOOL Busy();

    // RAID group  is expanding ==> all LUNs are being spread out, but not increased in size 
    BOOL Expanding();

    BOOL Defragging();

    // RAID group contains a single LUN which is increasing in size.
    BOOL ExpandingLun();

    // Expansion or defrag suspended
    BOOL Suspended();

    // Explicit remove
    BOOL Remove();

    // Not enough memory for Hi5 LUNs to function as such
    BOOL Hi5NotEnoughMemory();

    // Hi5 L2 cache LUN is present
    BOOL Hi5L2();

    // RAID group is internal
    BOOL Internal();

    // RAID group Attributes
    DWORD GetRGAttributes();

    // Latency Tolerance for a RAID group 
    DWORD LatencyTolerance();

    // Is Power Saving on for RG?
    BOOL PowerSavingEnabled();

     // Idle Time StandBy 
    DWORD IdleTimeStandby();

    // Checks whether RG is in standby
    BOOL InStandBy();

    // Checks whether the RG is capable of being spun down
    BOOL PowerSavingCapable();

    // Returns the element size of Raid Group
    DWORD ElementSize();


protected:
    DWORD DecodeWWN( K10_WWID wwn );

    DWORD       m_dwRgNum;
    K10_WWID    m_wwn;

	FbeRGInfo	*mpFbeRGInfo;
};



    // ****************** Unit Methods **************************************
    //
    // **********************************************************************
class FbeLunInfo;

class CSX_CLASS_EXPORT FlareLun: public FlareArray
{
public:
    FlareLun( FlareData *Fdobj, K10_LU_ID wwn);
    FlareLun( FlareData *Fdobj, K10_LU_ID wwn,
        HashUtil<K10_WWID, DWORD> *pLunListHashTable);
    virtual ~FlareLun() {}

    static bool LunExists(FlareData *FdObj, const K10_LU_ID &wwnLun);
    static void RemoveFromMap( FbeData *FdObj, K10_LU_ID wwn );

    K10_LU_ID WWN() { return m_wwn; }

    DWORD IdNum()  { return m_dwLunNum; }

    void RemoveFromMap( );
    LU_LUN_TYPE Type( );

    DWORD SuggestedHLU( );

    MMAN_RAID_TYPE RaidType();

    // Check if the AA bit is set to assign.
    BOOL AutoAssign();

    // Check if the AT bit is set to AT on.
    BOOL AutoTresspass();

    // Warning NULL termination may not be guarranteed. 
    void NiceName( K10_LOGICAL_ID *buf );

    TIMESTAMP BindStamp();

    ADM_SP_ID DefaultOwner();

    // May need to check with hostside to make sure this is true
    BOOL IsCurrentOwner();      // Returns True if this SP is the owner.
                                // False if it is not the owner

    // # sectors in an LUN element
    DWORD SectorsPerElement();

    DWORD ElementsPerParityStripe();

    DWORD StripeSize();

    // RAID group LUN is in
    DWORD RaidGroup();
    K10_WWID RaidGroupWWN();

    // TRUE if LUN is rebuilding
    BOOL IsRebuilding();

    // TRUE if LUN is expanding
    BOOL IsExpanding();

    // TRUE if LUN is internal
    BOOL Internal();

    // Get list of disks in a LUN 
    HRESULT DiskList( NAPtr<K10_WWID> &pWwnDisk, DWORD &dwNumberOfDisks );

    // Get disk count in a LUN 
    DWORD DiskCount();

    // Fill in a UNIT_CACHE_PARAMS struct
    void CacheParams( UNIT_CACHE_PARAMS &ucache_p );

    ADM_LBA UserCapacity();

    ADM_LBA FlareCapacity();

    ADM_LBA OverallCapacity();

    ADM_LBA AddressOffset();

    ADM_LBA LogicalOffset();

    ADM_LBA BindOffset();

    DWORD AlignedSector();

    //Rebuild priority (percentage of bandwidth to use for rebuild)
    DWORD RebuildPriority();
    // Background verify priority
    DWORD VerifyPriority();
    DWORD SniffRate();
    BOOL SniffEnabled();
    DWORD ZeroThrottleRate();
    DWORD PctZeroed();
    
    // TRUE if a background verify is in progress
    BOOL BackgroundVerify();

    // Is LUN bound, binding or already assigned?
    ADM_BIND_STATE BindState();


    // returns the percent completion of a rebuild on this unit (0-100)
    DWORD PctRebuilt();

    // returns the percent completion of a bind on this unit (0-100)
    // returns ADM_INVALID_PERCENTAGE if no bind is in progress
    DWORD PctBound();

    // returns the percent completion of an expansion on this unit (0-100)
    // returns ADM_INVALID_PERCENTAGE if no expansion is in progress
    DWORD PctExpanded();

    // returns the physical capacity of the unit in blocks
    LBA_T PhysicalCapacity();

    // returns the write cache hit ratio of the unit (0-100)
    DWORD WriteHitRatio();

    // returns the percentage of the cache pages owned by this unit that
    // are dirty (0-100)
    DWORD PctDirtyPages();

    // returns the number of write requests that accessed only pages 
    // already in the write cache
    DWORD WriteHits();

    // returns the bind alignment offset of this unit
    DWORD BindAlignmentOffset();

    // returns the number of blocks of unusable user space for this unit
    LBA_T UnusableUserSpace();


    /*
     *  Performance
     */

    // returns the read cache hit ratio of the unit (0-100)
    DWORD ReadHitRatio();

    // returns the time that the performance data was generated, in the form
    // 0xhhmmssxx, where hh is hours (0-23), mm is minutes (0-59), ss is
    // seconds (0-59), and xx is hundredths of a second
    DWORD Timestamp();

    // returns the number of read requests that accessed only pages 
    // already in the read or write cache
    DWORD ReadCacheHits();

    // returns the number of read requests that accessed pages not
    // already in the read or write cache
    DWORD ReadCacheMisses();

    // returns the number of blocks that were prefetched into the read cache
    DWORD BlocksPrefetched();

    // returns the number of blocks that were prefetched into the read cache
    // but never accessed
    DWORD UnreadPrefetchedBlocks();

    // returns the number of times a write had to flush a page to make room
    // in the cache
    DWORD ForcedFlushes();

    // returns the number of times an I/O crossed a stripe boundary
    DWORD StripeCrossings();

    // returns the number fast write cache hits for the unit
    DWORD FastWriteHits();

    // returns the sum of lengths of current request queues seen by each 
    // user request to this LUN
    ULONGLONG SumQueueLengthArrival();

    // returns the number of times a user request arrived while at least one 
    // other request was being processed by this unit
    DWORD ArrivalsToNonZeroQueue();

    // returns the number of blocks read
    DWORD BlocksRead();
    // return the number of blocks written
    DWORD BlocksWritten();

    // returns the hist_index (0-10) element of the read histogram array
    // Element n of the array contains the number of reads that were 
    // between 2**n-1 and 2**n blocks. Element 10 contains number of reads
    // that were larger than 1023 (2**10) blocks.
    DWORD ReadHistogram(DWORD hist_index);
    // returns the hist_index (0-10) element of the write histogram array
    // Element n of the array contains the number of writes that were 
    // between 2**n-1 and 2**n blocks. Element 10 contains number of writes
    // that were larger than 1023 (2**10) blocks.
    DWORD WriteHistogram(DWORD hist_index);

    // Returns number of times back end write was an entire stripe (i.e. 
    // traditional RAID 3 style write).
    DWORD Raid3StyleWrites();

    // returns number times the read cache was statisfied from the write cache data
    DWORD ReadCacheHitsFromWriteCache();

    // returns average read time for a LUN
    ULONGLONG AverageReadTime();

    // returns average write time for a LUN
    ULONGLONG AverageWriteTime();

    //  True if LUN freshly bound
    BOOL FreshlyBound();

    // True if LUN is Cache Dirty
    BOOL CacheDirty();

    // returns current state of the LUN
    DWORD CurrentState();


    // Per Disk Performance counters for LUs

    // Reads from disk specified by key
    DWORD DiskReadCount( int diskIndex );

    // Writes to disk specified by key
    DWORD DiskWriteCount( int diskIndex );

    // Blocks read from disk specified by key 
    LBA_T DiskBlockReads( int diskIndex );

    // Blocks written to disk specified by key 
    LBA_T DiskBlockWrites( int diskIndex );

protected:
    K10_LU_ID   m_wwn;
    DWORD       m_dwLunNum;

	FbeLunInfo	*mpFbeLunInfo;
};

#endif
