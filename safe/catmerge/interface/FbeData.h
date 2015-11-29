#ifndef FBE_DATA_H
#define FBE_DATA_H
 
#include "k10defs.h"
#include "FlareCommand.h"
#include "fbe/fbe_api_perfstats_interface.h"//MCR_STATS

#include <vector>
#include <map>
#include <queue>
#include <string>

#define MCR_ADMIN_INVALID                   0xffff
#define MCR_ADMIN_INVALID_PERCENTAGE        0xff
#define MCR_ADMIN_NOT_SUPPORTED             "NOT_SUPPORTED"

// default is for disks to hibernate after 30 minutes.
#define ADM_FBE_SECS_PER_MINUTE 60
#define ADM_FBE_POWER_SAVING_IDLE_TIME_IN_SEC_DEFAULT  (30*ADM_FBE_SECS_PER_MINUTE)
#define ADM_FBE_POWER_SAVING_LATENCY_IN_SEC_DEFAULT 120

//
// SPS type definitions for Navi
//
typedef enum adm_sps_type_enum
{
    ADM_SPS_TYPE_LEGECY,
    ADM_SPS_TYPE_BBU,
    ADM_SPS_TYPE_MODULE,
    ADM_SPS_TYPE_BATTERY
}
ADM_SPS_TYPE;

typedef std::vector<unsigned long> FbeObjList;
typedef std::vector<unsigned long>::iterator FbeObjListIterator;

typedef std::vector<K10_WWID> FbeWWNList;
typedef std::vector<K10_WWID>::iterator FbeWWNIterator;
typedef std::vector<long> FbeEnclDriveTypeList;
typedef std::vector<long>::iterator FbeEnclDriveTypeListIterator;

typedef std::vector<DWORD> FbeDWordList;
typedef std::vector<DWORD>::iterator FbeDWordIterator;

//
// FbeArrayInfo
//
class FbeDataImpl;

class FbeArrayInfo
{
public:
	FbeArrayInfo() : mpFbeDataImpl(NULL) {}
	FbeArrayInfo(FbeDataImpl *pFbeDataImpl);

protected:
	FbeDataImpl	*mpFbeDataImpl;
};

//
// FbeEnclosureInfo
//
class FbeEnclosureInfoImpl;

class FbeEnclosureInfo : public FbeArrayInfo
{
public:
	FbeEnclosureInfo() : mpImpl(NULL) {}
	FbeEnclosureInfo(FbeEnclosureInfoImpl *pImpl, FbeDataImpl *pFbeDataImpl);
	~FbeEnclosureInfo();

	K10_WWID & get_wwid();

	long getAddress();
	bool isEnclosurePresent();
	bool isProcessorEncl();
	long getEnclosureLccCount();
	long getEnclosureSSCCount();
	long getEnclosurePsCount();
	long getEnclosureFanCount();
	long getEnclosureDiskCount();
	long getEnclosureSpCount();
	long getEnclosureSpsCount();
	K10_WWID getWWN();
	long getEnclosure();
	long getBus();
	long getSlot();
	long getEnclosureType();
	long getEnclosureIOModuleCount();
	long getEnclosureIOAnnexCount();
	long getEnclosureDimmCount();
    long getEnclosureSsdCount();
	long getCurrentSpeed();
	long getMaxSpeed();
	long getState();
	bool getFault();
	bool getCrossCable();
	long getSymptom();
	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcProductSerialNumber(long buffer_size, char *buffer);
	long getEmcProductPartNo(long buffer_size, char *buffer);
	long getEmcProductRevisionNo(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	bool getResumeFaulted();
	long getFlashEnable();
	long getFlashBit();
	long getDiskFlashBits(long diskSlot);
	long getDriveType();
	long getFrumonRev(long buffer_size, char *buffer);
	long getEnclosureDisksPerBankCount();
	long getMaximumPower();
	long getCurrentPower();
	long getAveragePower();
	long getPowerStatus();
	long getMaximumAirInletTemperature();
	long getCurrentAirInletTemperature();
	long getAverageAirInletTemperature();
	long getAirInletTemperatureStatus();
	long getChassisType() {return 1;}
	FbeEnclDriveTypeList getDriveTypeList();
	long getEnclosureFaultLedReason();
	fbe_u32_t getEnclosureShutdownWarning(int index);
	fbe_u32_t getEnclosureShutdownReason();
	fbe_bool_t isFaultStatusRegFault(int index);
	fbe_bool_t isHwErrMonFault(int index);
	fbe_u32_t getAmbientOvertempFault(int index);
	fbe_u32_t getAmbientOvertempWarning(int index);
	fbe_u32_t getTap12VMissing(int index);
	fbe_bool_t getOverTempWarning();
	fbe_bool_t getOverTempFailure();

	FbeEnclosureInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

	 // Drive MidPlane Resume Info
	 long getDriveMPEmcPartNo(long buffer_size, char *buffer);
	 long getDriveMPEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	 long getDriveMPEmcSerialNumber(long buffer_size, char *buffer);
	 long getDriveMPEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	 long getDriveMPEmcProductSerialNumber(long buffer_size, char *buffer);
	 long getDriveMPEmcProductPartNo(long buffer_size, char *buffer);
	 long getDriveMPEmcProductRevisionNo(long buffer_size, char *buffer);
	 long getDriveMPAssemblyName(long buffer_size, char *buffer);
	 long getDriveMPVendorPartNo(long buffer_size, char *buffer);
	 long getDriveMPVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	 long getDriveMPVendorSerialNumber(long buffer_size, char *buffer);
	 long getDriveMPVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	 long getDriveMPVendorName(long buffer_size, char *buffer);
	 long getDriveMPLocationOfManufacture(long buffer_size, char *buffer);
	 long getDriveMPDateOfManufacture(long buffer_size, char *buffer);
	 bool getDriveMPResumeFaulted();
	 bool isDriveMPResumeAvailable();

private:
	FbeEnclosureInfoImpl *mpImpl;
};


//
// FbeDiskInfo
//
class FbeDiskInfoImpl;

class FbeDiskInfo : public FbeArrayInfo
{
public:
	FbeDiskInfo() : mpImpl(NULL) {}
	FbeDiskInfo(FbeDiskInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeDiskInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getNumber();
	long getAddress();
	bool isLoopAValid();
	bool isLoopBValid();
	bool isBypassEnabledLoopA();
	bool isBypassEnabledLoopB();
	bool isBypassRequestedLoopA();
	bool isBypassRequestedLoopB();
	bool isFaulted();
	long getDriveType();
	long getSpeedCapability();
	long long getGrossCapacity();
	long long getDefaultOffset();
	const long getVendorID(long buffer_size, char *buffer);
	const long getProductID(long buffer_size, char *buffer);
	const long getProductRev(long buffer_size, char *buffer);
	const long getProductSerialNumber(long buffer_size, char *buffer);
	const long getTLAPartNumber(long buffer_size, char *buffer);
	bool isSpinDownQualified();
	bool isInserted();
	long DeathReason();
    bool getEndOfLifeState();
	const long getDGpartNumberAscii(long buffer_size, char *buffer);
	K10_WWID getEnclosureWWID();
	long getPctZeroed();
	long getPctCopied();
	bool isHotSpare();
	long getPowerSavingState();
	bool isPowerSavingsCapable();
	long getRaidGroup();
	bool isFormattedState() const {return false;}
	bool hasLevel2Cache() const {return false;}
	bool isInquiryBlockValid () const {return true;}

	bool MCR_hidden();

	/* F B E C O M   T B D */

	/* F B E C O M   T B D */

	/* S T U B S */

	/* N O T  U S E D*/
	const long getDriveDescriptionBuff(long buffer_size, char *buffer);
	const long getBridgeHWRev(long buffer_size, char *buffer);
	long getLifecycleState(); // New for MCR
	long long getLastLog() const; // New for MCR
	long getBlockSize(); // New for MCR
	long getDriveQDepth(); // New for MCR
	long getDriveRPM(); // New for MCR    
	/* N O T  U S E D*/

	/* N O T   S U P P O R T E D*/
	long getBindSignature () {return 0;}
	long long getZeroMark() {return 0;}
	bool  isExpansionDisk() const {return false;}
	bool  isReplacedbyHotSpare(K10_WWID& ) const {return false;}
	bool  isCriticalOrForeignDisk(ULONG buffer_size, char *expectedDiskSerialNumber, char *insertedDiskSerialNumber) {return false;}
	bool  isMissingDisk(ULONG buffer_size, char *missingDiskSerialNumber) {return false;}
	bool  isRebuildNeeded() {return false;}
	/* N O T   S U P P O R T E D*/

	/* F B E   T B D */
	/* S T U B B E D */
	long long getUnusableSectors() {return 0;}
	long getODFUDiskStatus();
	long getODFUDiskFailCode();
	const long getODFUPriorDiskFWRev(long buffer_size, char *buffer);
	long getState();
	/* S T U B B E D */
	/* F B E   T B D */

	/* P E R F O R M A N C E   D E F E R R E D*/
	ULONG getHardReadErrors() {return 0;}
	ULONG getHardWriteErrors() {return 0;}
	ULONG getSoftReadErrors() {return 0;}
	ULONG getSoftWriteErrors() {return 0;}
	ULONG getAverageServiceTime() {return 0;} // NOT USED
	ULONG getAverageAddressDiff() {return 0;} // NOT USED
//MCR_STATS
	LBA_T getReadCount();
	LBA_T getWriteCount();
//MCR_STATS
	ULONG getReadRetries() {return 0;} // NOT USED
	ULONG getWriteRetries() {return 0;} // NOT USED
	ULONG getNumRemappedSectors() {return 0;} // NOT USED
//MCR_STATS

	LBA_T getBlocksRead();
	LBA_T getBlocksWritten();
	LBA_T getSumOfBlocksSeeked ();
	LBA_T getSumQueueLenOnArrival();
	LBA_T getArrivalsToNonzeroQ();
	LBA_T getBusyTicks();
	LBA_T getIdleTicks();
//MCR_STATS
	long getSpinUpTimeInMinutes();  //IMPLEMENTED
	long getStandByTimeInMinutes(); //IMPLEMENTED
	long getSpinUpCount(); //IMPLEMENTED
	/* P E R F O R M A N C E   D E F E R R E D*/
	/* S T U B S */

	long getDiskPool();

	FbeDiskInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeDiskInfoImpl	*mpImpl;
};

//
// FbeDiskPoolInfo
//
class FbeDiskPoolInfoImpl;

class FbeDiskPoolInfo : public FbeArrayInfo
{
public:
	FbeDiskPoolInfo() : mpImpl(NULL) {}
	FbeDiskPoolInfo(FbeDiskPoolInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeDiskPoolInfo();

	K10_WWID & get_wwid();

	long get_number();

	FbeObjList & get_disk_list();

	FbeDiskPoolInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeDiskPoolInfoImpl	*mpImpl;
};

//
// FbeLocalSPInfo
//
class FbeLocalSpInfoImpl;

class FbeLocalSpInfo : public FbeArrayInfo
{
public:
	FbeLocalSpInfo() : mpImpl(NULL) {}
	FbeLocalSpInfo(FbeLocalSpInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeLocalSpInfo();

	long CabinetId();

	long GetSerialNumber(DWORD buffer_size, char *buffer);
	void SpsTestTime(DWORD *day, DWORD *hour, DWORD *minute);
	void SetSpsTestTime(DWORD day, DWORD hour, DWORD minute);

	long PeerBootState();
	long PeerBootFaultCode();
    bool IsPeerSpApplicationRunning();

	bool ManagementPortSpeedSupported();
	bool ManagementPortLinkStatus();
	long ManagementPortRequestedSpeed();
	long ManagementPortRequestedDuplexMode();
	long ManagementPortSpeed();
	long ManagementPortDuplexMode();
	bool ManagementPortAutoNegotiateMode();
	long ManagementPortAllowedSpeed();

	FbeLocalSpInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeLocalSpInfoImpl *mpImpl;
};

//
// FbeSPInfo
//
class FbeSPInfoImpl;

class FbeSPInfo : public FbeArrayInfo
{
public:
	FbeSPInfo() : mpImpl(NULL) {}
	FbeSPInfo(FbeSPInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeSPInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getAddress();
	K10_WWID getEnclosureWWID();

	BOOL isInserted();
	long getState();
	long getSubState();
	bool isPeerInserted();
	bool isPeerPresent();
	long getSPid();
	bool isEngineFaulted();
	long getPhysicalMemorySize();
	long getCmmControlPoolTop();
	long getCmmControlPoolBottom();
	long getCmmControlNonCacheUsed();
	long getExternalDataPoolUsersSize();
	long getWWNseed();

	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getNumProgrammables();
	long getProgrammableName(long programmable, long buffer_size, char *buffer);
	long getProgrammableRevision(long programmable, long buffer_size, char *buffer);
	long getNumMacAddresses();
	long getMacAddress(long mac_address, long buffer_size, char *buffer);
	bool isResumeFaulted();
	bool isUnsafeToRemoveSP();
	bool isTemperatureSensorFaulted();
	bool isPeerTemperatureSensorFaulted();
	bool isAmbientTemperatureFaulted();
	bool isPeerAmbientTemperatureFaulted();
	bool isSPShuttingDown();
	bool isPeerSPShuttingDown();

	const UCHAR* getFirmwareRev();
    long lccFirmwareRev(long lccIndex, long buffer_size, char *buffer);
    bool lccFaulted(long lccIndex);
    bool lccIsLocal(long lccIndex);
    bool lccIsInserted(long lccIndex);

	FbeSPInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeSPInfoImpl *mpImpl;
};

//
// FbeLCCInfo
//
class FbeLCCInfoImpl;

class FbeLCCInfo : public FbeArrayInfo
{
public:
	FbeLCCInfo() : mpImpl(NULL) {}
	FbeLCCInfo(FbeLCCInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeLCCInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getAddress();
	K10_WWID getEnclosureWWID();

	bool isExpansionPortOpen();
	bool getPeerInformationBit();
	bool isPrimaryMediaConverterFault(){return 0;}
	bool isExpansionPortMediaConverterFault(){return 0;}
	bool isShunted();
	bool isFaulted();
	bool isInserted();
	long getState();
	long getSubState();
	long getMaxSpeed();
	long getCurrentSpeed();
	long getType();
	long getCurrentState(){return 0;}
	long getLccUpgradeState();
	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getNumProgrammables();
	long getProgrammableName(long programmable, long buffer_size, char *buffer);
	long getProgrammableRevision(long programmable, long buffer_size, char *buffer);
	bool isResumeFaulted();
//      This routine replaces the firmware routines that follow to support CDES 2 format.
        void getExpanderFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);
	void getPrevFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);
	void getFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);
	void getUpgradingFirmwareRevision(char *pStrFirmwareRev, int expanderNumber);
	bool isUpgradeInProgress(int expanderNumber);
	long getUpgradeStatus(int expanderNumber);
	long getWorkState();
	long getExpanderCount();

	const UCHAR* getFirmwareRev();

	FbeLCCInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeLCCInfoImpl *mpImpl;
};

class FbeSSCInfoImpl ;
//
// FbeSSCInfo
//
class FbeSSCInfo : public FbeArrayInfo
{
public:
	FbeSSCInfo() : mpImpl(NULL) {}
	FbeSSCInfo(FbeSSCInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeSSCInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	//long getAddress();
	K10_WWID getEnclosureWWID();
	bool isFaulted();
	bool isInserted();
	long getType();
	FbeSSCInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }
private:
	FbeSSCInfoImpl *mpImpl;
};
//
// FbePSInfo
//
class FbePSInfoImpl;

class FbePSInfo : public FbeArrayInfo
{
public:
	FbePSInfo() : mpImpl(NULL) {}
	FbePSInfo(FbePSInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbePSInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getSpSlot();
	long getAddress();
	K10_WWID getEnclosureWWID();

	long getState();
	long getSubState();
        long getACDCInput();

	long isThermalFault();
	bool isShutdown();
	bool isInserted();
	long isFaulted();
	bool is12vOk(){return 0;}//not supported by MCR
	long isAcFail();
	bool isFanFaulted();
	bool isLogicalFaulted();
	long getassociatedSP();
	long getPartialDataFault();
	long getSMBusAccessFault();
	bool isSupported();
	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getNumProgrammables();
	long getProgrammableName(long programmable, long buffer_size, char *buffer);
	long getProgrammableRevision(long programmable, long buffer_size, char *buffer);
	bool isResumeFaulted();
	void getFirmwareRevision(char *pFirmwareRevision);

	const UCHAR* getFirmwareRev();

	FbePSInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbePSInfoImpl *mpImpl;
};


//
// FbeFanInfo
//
class FbeFanInfoImpl;

class FbeFanInfo : public FbeArrayInfo
{
public:
	FbeFanInfo() : mpImpl(NULL) {}
	FbeFanInfo(FbeFanInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeFanInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getSpSlot();
	long getLocation();
	long getAddress();
	long getAssociatedSP();
	long getLogicalNumber();
	bool isResumeSupported();
	K10_WWID getEnclosureWWID();
	long getState();
	long getSubState();
	long isInserted();
	long isSingleFault();
	long isMultipleFault();
	long getSMBusAccessFault();
	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcProductSerialNumber(long buffer_size, char *buffer);
	long getEmcProductPartNo(long buffer_size, char *buffer);
	long getEmcProductRevisionNo(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	long getNumProgrammables();
	long getProgrammableName(long programmable, long buffer_size, char *buffer);
	long getProgrammableRevision(long programmable, long buffer_size, char *buffer);
	bool getResumeFaulted();
	BOOL isFaulted();
	const UCHAR* getFirmwareRev();

	FbeFanInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeFanInfoImpl *mpImpl;
};

//
// FbeSPSInfo
//
class FbeSPSInfoImpl;

class FbeSPSInfo : public FbeArrayInfo
{
public:
	FbeSPSInfo() : mpImpl(NULL) {}
	FbeSPSInfo(FbeSPSInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeSPSInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getSpSlot();
	long getAssociatedSP();
	long getAddress();
	K10_WWID getEnclosureWWID();


	bool isAcLineFaulted();
	bool isCharging();
	bool isLowBattery();
	bool isReplace();
	bool isFaulted();
	bool isInserted();
	long getBatteryState();

	long getState();
	long getSubState();

	long getType();

	long getCabling();
	long getMaximumPower( );
	long getCurrentPower( );
	long getAveragePower( );
	long getPowerStatus( );
	bool IsResumeAccessible();
	long getEmcPartNo( long buffer_size, char *buffer );
	long getEmcArtworkRevisionLevel( long buffer_size, char *buffer );
	long getEmcAssemblyRevisionLevel( long buffer_size,  char *buffer);
	long getEmcSerialNumber( long buffer_size, char *buffer);
	long getEmcProductSerialNumber(long buffer_size, char *buffer) { return 0; }
	long getEmcProductPartNo( long buffer_size, char *buffer ) { return 0; }
	long getEmcProductRevisionNo( long buffer_size, char *buffer ) { return 0; }
	long getVendorPartNo( long buffer_size, char *buffer);
	long getVendorName( long buffer_size, char *buffer );
	long getLocationOfManufacture( long buffer_size, char *buffer);
	long getDateOfManufacture( long buffer_size, char *buffer );
	long getAssemblyName( long buffer_size, char *buffer );
	long getNumProgrammables();
	long getProgrammableName( long programmable, long buffer_size,
		char *buffer);
	long getProgrammableRevision( long programmable, long buffer_size,
		char *buffer);
	bool isResumeFaulted();

	FbeSPSInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeSPSInfoImpl *mpImpl;
};

//
// FbeVBusInfo
//
class FbeVBusInfoImpl;

class FbeVBusInfo : public FbeArrayInfo
{
public:
	FbeVBusInfo() : mpImpl(NULL) {}
	FbeVBusInfo(FbeVBusInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeVBusInfo();

	K10_WWID & get_wwid();

	long getBus();
	long getSlot();
	long getCurrentSpeed(long busNum);
        long getLinkState(long busNum);
	long getCapableSpeeds(long busNum);
	long getEnclosureBus(long bId);
	long getEnclosureAddress(long enclosureNumber);
	bool isDuplicateAddresses(long bus);
	long getDuplicateCount(long enclosureNumber);
	long getSfpCondition();
	long getMaxEnclosuresExceeded(long busNum);
	long getMaxEnclosures(long busNum);
	long getNumberOfEnclosures(long busNum);
	bool getPortCombinationStatus();

	FbeVBusInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeVBusInfoImpl *mpImpl;
};

//
// FbeIOMInfo
//
class FbeIOMInfoImpl;

class FbeIOMInfo : public FbeArrayInfo
{
public:
	FbeIOMInfo() : mpImpl(NULL) {}
	FbeIOMInfo(FbeIOMInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeIOMInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getAddress();
	K10_WWID getEnclosureWWID();

    long getType();
    long getClass();
    bool isInserted();
    bool isPeerMezzanineInserted();
    bool isInCarrier();
    bool isFlashEnable();
    long getIOMState();
    long getIOMSubState();
    long getIOMPowerStatus();
    bool isIOMFaultLED();
    long getPortCount();
    long getLabelName( long buffer_size, char *buffer);
    bool getPortFlashState(long dwPortNumber);
    long getSMBusAccessFault();
    bool isResumeSupported();
    long getExpectedSLICTypes();
    long getIOMSLICType();
    long getIOMFRUId();
    long getUniqueID( );
    long getEmcPartNo(long buffer_size, char *buffer);
    long getEmcArtworkRevisionLevel( long buffer_size, char *buffer);
    long getEmcAssemblyRevisionLevel( long buffer_size, char *buffer);
    long getEmcSerialNumber( long buffer_size, char *buffer);
    long getEmcProductSerialNumber(long buffer_size, char *buffer) { return 0; }
    long getEmcProductPartNo( long buffer_size, char *buffer ) { return 0; }
    long getEmcProductRevisionNo( long buffer_size, char *buffer ) { return 0; }
    long getVendorName( long buffer_size, char *buffer);
    long getVendorPartNo( long buffer_size, char *buffer);
    long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
    long getVendorSerialNumber(long buffer_size, char *buffer);
    long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
    long getLocationOfManufacture( long buffer_size, char *buffer);
    long getDateOfManufacture( long buffer_size, char *buffer);
    long getAssemblyName( long buffer_size, char *buffer);
    long getProgrammableName( long programmable, long buffer_size,
                                 char *buffer);
    long getProgrammableRevision( long programmable, long buffer_size,
                                     char *buffer);
    long getMacAddress( long mac_address, long buffer_size,
                           char *buffer);
    long getNumProgrammables();
    long getNumMacAddresses();    
    bool isResumeFaulted();
    long getLinkState();

	FbeIOMInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeIOMInfoImpl *mpImpl;
};

//
// FbeDimmInfo
//
class FbeDimmInfoImpl;

class FbeDimmInfo : public FbeArrayInfo
{
public:
	FbeDimmInfo() : mpImpl(NULL) {}
	FbeDimmInfo(FbeDimmInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeDimmInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	
	K10_WWID getEnclosureWWID();

	long getState();
	long getSubState();
	long getSize();
	long getEmcSerialNumber( long buffer_size, char *buffer);
	long getEmcPartNo(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getVendorName( long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorPartNo( long buffer_size, char *buffer);
	long getEmcProductSerialNumber(long buffer_size, char *buffer) { return 0; }
	long getEmcProductPartNo( long buffer_size, char *buffer ) { return 0; }
	long getProgrammableRevision( long programmable, long buffer_size, char *buffer) { return 0; }
	long getSpId();
	long getSlotId();
    bool isLocalDimm();

	FbeDimmInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeDimmInfoImpl *mpImpl;
};

//
// FbeSsdInfo
//
class FbeSsdInfoImpl;

class FbeSsdInfo : public FbeArrayInfo
{
public:
	FbeSsdInfo() : mpImpl(NULL) {}
	FbeSsdInfo(FbeSsdInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeSsdInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	
	K10_WWID getEnclosureWWID();

	long getState();
	long getSubState();
	long getEmcSerialNumber( long buffer_size, char *buffer);
	long getEmcPartNo(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
    long getSpId();
	long getSlotId();
    bool isLocalSsd();
    long getFirmwareRevision(long buffer_size, char *buffer);

	FbeSsdInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeSsdInfoImpl *mpImpl;
};

//
// FbeFPInfo
//
class FbeFPInfoImpl;

class FbeFPInfo : public FbeArrayInfo
{
public:
	FbeFPInfo() : mpImpl(NULL) {}
	FbeFPInfo(FbeFPInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeFPInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getSP();
	long getAddress();
	K10_WWID getEnclosureWWID();

    long getIOMSlotNumber();
    long getPortPhysicalNumber();
    long getPortRole();
    long getPortSubRole();
    long getPortProtocol();
    long getPortState();
    long getPortSubState();
    long getPortSFPState();
    long getPortIOMGroup();
    K10_WWID encodeIOModuleWWN();
    bool isIOMInserted();
    long getSupportedSFPSpeeds();
    long getSupportedSFPProtocols();
    bool isSFPSupported();
    long getSupportedFPSpeeds();
    long getCurrentFPSpeed();
    long getFPLinkState();
    long getSFPEMCPartNumber(long buffer_size, char *buffer);
    long getSFPEMCSerialNumber(long buffer_size, char *buffer);
    long getSFPVendorPartNumber(long buffer_size, char *buffer);
    long getSFPVendorSerialNumber(long buffer_size, char *buffer);
    long getLogicalPortNumber();
    long getUniqueID( );
	bool getPortCombinationStatus();

	FbeFPInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeFPInfoImpl *mpImpl;
};

//
// FbeSubFRUInfo
//
class FbeSubFRUInfoImpl;

class FbeSubFRUInfo : public FbeArrayInfo
{
public:
	FbeSubFRUInfo() : mpImpl(NULL) {}
	FbeSubFRUInfo(FbeSubFRUInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeSubFRUInfo();

	K10_WWID & get_wwid();

	long getEnclosure();
	long getBus();
	long getSlot();
	long getAddress();
	K10_WWID getEnclosureWWID();
	long getSP();
	bool isInserted();
	bool isResumeSupported();
	long getState();
	long getSMBusAccessFault();
	long getEmcPartNo(long buffer_size, char *buffer);
	long getEmcArtworkRevisionLevel(long buffer_size, char *buffer);
	long getEmcAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getEmcSerialNumber(long buffer_size, char *buffer);
	long getVendorName(long buffer_size, char *buffer);
	long getVendorPartNo(long buffer_size, char *buffer);
	long getVendorArtworkRevisionLevel(long buffer_size, char *buffer);
	long getVendorSerialNumber(long buffer_size, char *buffer);
	long getVendorAssemblyRevisionLevel(long buffer_size, char *buffer);
	long getLocationOfManufacture(long buffer_size, char *buffer);
	long getDateOfManufacture(long buffer_size, char *buffer);
	long getAssemblyName(long buffer_size, char *buffer);
	long getNumProgrammables();
	long getProgrammableName(long programmable, long buffer_size, char *buffer);
	long getProgrammableRevision(long programmable, long buffer_size, char *buffer);
	long getNumMacAddresses();
	long getMacAddress(long mac_address, long buffer_size, char *buffer);
	bool isResumeFaulted();

	FbeSubFRUInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeSubFRUInfoImpl *mpImpl;
};

//
// FbeLunInfo
//
class FbeLunInfoImpl;

class FbeLunInfo : public FbeArrayInfo
{
public:
	FbeLunInfo() : mpImpl(NULL) { for (int i = 0; i < 256; i++) {mStrBuf[i] = 0;}}
	FbeLunInfo(FbeLunInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeLunInfo();

	K10_WWID & get_wwid();
	int get_lun_number();
	const char * get_lun_name();
	int get_rg_id();
    long getVerifyPriority();
    long getZeroThrottleRate();
    long getSniffRate();
    bool isSniffEnabled();
    bool isBackgroundVerifyInProgress();
	int get_rg_number();

	int get_type();
	int get_suggested_hlu();
	int get_raid_type();
	int get_bind_stamp();
	int get_sectors_per_element();
	int get_elements_per_parity_stripe();
	int get_stripe_size();
	bool is_rebuilding();
	bool is_internal();
	LBA_T get_user_capacity();
	LBA_T get_flare_capacity();
	LBA_T get_overall_capacity();
	long get_logical_offset();
	long get_pct_zeroed();
	long get_pct_rebuilt();
	bool is_freshly_bound();
	long get_current_state();

//MCR_STATS
	//performance
	LBA_T get_time_stamp();
	LBA_T get_stripe_crossings();
	LBA_T get_sum_queue_length_arrival();
	LBA_T get_arrivals_to_non_zero_queue();
	LBA_T get_blocks_read();
	LBA_T get_blocks_written();
	LBA_T get_read_histogram(long hist_index);
	LBA_T get_write_histogram(long hist_index);
	LBA_T get_raid3_style_writes();
	LBA_T get_average_read_time();
	LBA_T get_average_write_time();
	LBA_T get_disk_read_count(int diskIndex);
	LBA_T get_disk_write_count(int diskIndex);
	LBA_T get_disk_block_reads(int diskIndex);
	LBA_T get_disk_block_writes(int diskIndex);
//MCR_STATS

	bool isFlarePrivate();
	bool isCelerra();

	FbeLunInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeLunInfoImpl *mpImpl;
    char mStrBuf[256];
};


//
// FbeRGInfo
//

class FbeRGInfoImpl;

class FbeRGInfo : public FbeArrayInfo
{
public:
	FbeRGInfo() : mpImpl(NULL) {}
	FbeRGInfo(FbeRGInfoImpl *mpImpl, FbeDataImpl *pFbeDataImpl);
	~FbeRGInfo();

	K10_WWID & get_wwid();
	int get_number();

	int get_state();
	int get_raid_type();
	int getElementSize();
	
	LBA_T get_logical_capacity();
	LBA_T get_free_capacity();
	LBA_T get_physical_capacity();
	LBA_T getLargestContigFreeSpace ();
	LBA_T getLatencyTolerance ();
	LBA_T getIdleTimeStandby ();
	long GetRGAttributes();

	bool isInternal ();
	bool isInStandyBy();
	bool isPowerSavingCapable();
	bool getPowerSavingEnabled();

	bool isFlarePrivate();

	FbeObjList & get_lun_list();
	FbeObjList & get_disk_list();
	
	FbeRGInfoImpl * get_impl()	{ return mpImpl; }
	void clear_impl() { mpImpl = NULL; }

private:
	FbeRGInfoImpl *mpImpl;
};

//
// FbeData
//

typedef std::map<K10_WWID, FbeEnclosureInfo> FbeEnclosureMap;
typedef FbeEnclosureMap::iterator FbeEnclosureMapIterator;

typedef std::map<K10_WWID, FbeDiskInfo> FbeDiskMap;
typedef FbeDiskMap::iterator FbeDiskMapIterator;

typedef std::map<K10_WWID, FbeDiskPoolInfo> FbeDiskPoolMap;
typedef FbeDiskPoolMap::iterator FbeDiskPoolMapIterator;

typedef std::map<K10_WWID, FbeSPInfo> FbeSpMap;
typedef FbeSpMap::iterator FbeSpMapIterator;

typedef std::map<K10_WWID, FbeSSCInfo> FbeSSCMap;
typedef FbeSSCMap::iterator FbeSSCMapIterator;

typedef std::map<K10_WWID, FbeLCCInfo> FbeLccMap;
typedef FbeLccMap::iterator FbeLccMapIterator;

typedef std::map<K10_WWID, FbeFanInfo> FbeFanMap;
typedef FbeFanMap::iterator FbeFanMapIterator;

typedef std::map<K10_WWID, FbePSInfo> FbePsMap;
typedef FbePsMap::iterator FbePsMapIterator;

typedef std::map<K10_WWID, FbeSPSInfo> FbeSpsMap;
typedef FbeSpsMap::iterator FbeSpsMapIterator;

typedef std::map<K10_WWID, FbeVBusInfo> FbeVbusMap;
typedef FbeVbusMap::iterator FbeVbusMapIterator;

typedef std::map<K10_WWID, FbeIOMInfo> FbeIomMap;
typedef FbeIomMap::iterator FbeIomMapIterator;

typedef std::map<K10_WWID, FbeDimmInfo> FbeDimmMap;
typedef FbeDimmMap::iterator FbeDimmMapIterator;

typedef std::map<K10_WWID, FbeSsdInfo> FbeSsdMap;
typedef FbeSsdMap::iterator FbeSsdMapIterator;

typedef std::map<K10_WWID, FbeFPInfo> FbeFpMap;
typedef FbeFpMap::iterator FbeFpMapIterator;

typedef std::map<K10_WWID, FbeSubFRUInfo> FbeSubFruMap;
typedef FbeSubFruMap::iterator FbeSubFruMapIterator;

typedef std::map<K10_WWID, FbeLunInfo> FbeLunMap;
typedef FbeLunMap::iterator FbeLunMapIterator;

typedef std::map<K10_WWID, FbeRGInfo> FbeRGMap;
typedef FbeRGMap::iterator FbeRGMapIterator;

// maps from fbe_object_id_t to K10_WWID
typedef std::map<unsigned long, K10_WWID> FbeObjIdMap;
typedef FbeObjIdMap::iterator FbeObjIdMapIterator;

class FbeDataImpl;

class FbeData
{
public:
	FbeData(bool bDoPoll);
	~FbeData();

	void Repoll(bool force_poll);

	bool GetFullPollRequired();
	void ClearFullPollRequired();

	FbeLunMap & get_lun_map();
	FbeObjIdMap & get_fbe_objid_lun_map();
	void refresh_lun_map(K10_WWID *pWwn, bool is_destroyed);

	FbeRGMap & get_RG_map();
	FbeObjIdMap & get_fbe_objid_RG_map();

	FbeDiskMap & get_disk_map();
	FbeObjIdMap & get_fbe_objid_disk_map();

	FbeDiskPoolMap & get_disk_pool_map();

	FbeEnclosureMap & get_enclosure_map();
	FbeSpMap & get_sp_map();
	FbeLccMap & get_lcc_map();
	FbeSSCMap & get_ssc_map();
	FbeFanMap & get_fan_map();
	FbePsMap & get_ps_map();
	FbeSpsMap & get_sps_map();
	FbeVbusMap & get_vbus_map();
	FbeIomMap & get_iom_map();
	FbeDimmMap & get_dimm_map();
    FbeSsdMap & get_ssd_map();
	FbeFpMap & get_fp_map();
	FbeSubFruMap & get_subfru_map();

	FbeLocalSpInfo & get_local_sp();

	long get_iom_max_count();
	long get_fc_max_count();
	long get_fc_be_max_count();
	long get_iscsi_max_count();
	long get_iscsi_10g_max_count();
	long get_fcoe_max_count();
	long get_fcoe_fe_max_count();
	long get_sas_be_max_count();
	long get_sas_fe_max_count();
	long get_drive_slot_max_count();
	long get_supported_slic_types();
	long get_public_lun_max_count();
	long get_private_lun_max_count();
	long get_rg_max_count();
	long get_private_rg_max_count();

	void BlockEventNotifications();
	void UnblockEventNotifications();

	unsigned long GetRgNum(unsigned long disk_id);

	long MaximumPower();
	long AveragePower();
	long CurrentPower();
	long PowerStatus();


	long CelerraLunCount();

	bool DiskSniffEnable();

	HRESULT GetFlexportNo(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, DWORD &dwFlexportNo);
	HRESULT GetFlexportWWN(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, K10_WWID &key );

	DWORD GetFlexportNoList(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, DWORD dwFlexportNo[]);
	DWORD GetFlexportWWNList(DWORD dwPortLogicalNo, DWORD dwSP, DWORD dwPortRole, K10_WWID key[] );
	BOOL IsValidPort(DWORD dwPortLogicalNo, DWORD dwPortRole);

	bool SystemPowerSavingsOn();
    unsigned long PowerSavingIdleTime();
    unsigned long PowerSavingMinLatencyTolerance();

	LBA_T GetSharedExpectedMemoryInfo();
    long  _mapPbaToLba(fbe_object_id_t raid_group_id,
                       fbe_u16_t position,
                       fbe_lba_t pba,
                       fbe_lba_t * lba,
                       bool * is_parity);
//MCR_STATS
    bool setStats(bool on_off);
    unsigned int getPerfTimeStamp();
    unsigned int getPerfDateStamp();
    void InvalidateDiskPerfData();
    void InvalidateLunPerfData();
    long  _GetDiskPerfData(K10_WWID &drvWwn, fbe_perfstats_pdo_sum_t* perfstats);
    long  _GetLunPerfData(K10_WWID &lunWwn, fbe_perfstats_lun_sum_t* perfstats);
    bool PowerSavingStatisticsEnabled();
	bool StatisticsEnabled();
    long getDiskPerfDataContainer ();
    long getLunPerfDataContainer ();
//MCR_STATS
	// ODFU Process related interfaces

    int ProcessDriveDownloadOperationType();

	int ProcessDriveDownloadStatus();

	int ProcessDriveDownloadFailCode();

	ULONG ProcessDriveDownloadStartTime();

	ULONG ProcessDriveDownloadStopTime();

	int ProcessDriveDownloadFirmwareFileName(int buffer_size, char *buffer);

	int ProcessDriveMaxConcurrentCount();

	bool anyUpgradeInProgress();

	FbeDataImpl *GetImpl()	{ return mpImpl; }

	static FbeData *FbeDataInstance;
	static int RefCnt;
	static bool did_initial_poll;

//Encryption Config from MCR

        DWORD Get_Encryption_Mode();
	DWORD Get_Encryption_Backup_Status();
	DWORD Get_Encryption_Primary_SP_ID();
	DWORD Get_Encryption_Status();
	LBA_T Get_Encryption_Consumed_Blocks();
	LBA_T Get_Encryption_Encrypted_Blocks();

private:
	FbeDataImpl *mpImpl;
};

#endif
