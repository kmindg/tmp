// DiskPoolList.h: interface for the DiskPoolList class
//
// Copyright (C) 2009	EMC Corporation
//
//
//
//	Revision History
//	----------------
//	11 May 07	R. Hicks	Initial version.
//	07 Oct 08	R. Hicks	DIMS 210210: Enforce new Virtual Provisioning platform limits.
//	15 Jan 09	D. Hesi		Added Scrub Disk Pool support
//	15 Jan 09	V. Chotaliya Code changes to return TLD data for CREATE and SET operation
//
//////////////////////////////////////////////////////////////////////

#ifndef DISK_POOL_LIST_H
#define DISK_POOL_LIST_H

#include <windows.h>

#include "user_generics.h"
#include "CaptivePointer.h"
#include "FlareData.h"
#include "WWIDAssocHashUtil.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//------------------------------------------------------------Constants

#define MAX_RGS_PER_DISK_POOL		FRU_COUNT

#define MAX_DISK_POOLS				RG_USER_COUNT

//------------------------------------------------------------Typedefs

typedef enum disk_pool_opcode_tag
{
	DISK_POOL_CREATE = 1,
	DISK_POOL_EXPAND,
	DISK_POOL_SHRINK,
	DISK_POOL_DELETE,
	DISK_POOL_SCRUB
} DISK_POOL_OPCODE;

typedef enum disk_pool_dumpcode_tag
{
	DISK_POOL_DUMP_ALL = 1,
	DISK_POOL_DUMP_ORPHAN_DISKS
} DISK_POOL_DUMPCODE;

// used to describe a change to a disk pool
typedef struct _DiskPoolParams
{
	DISK_POOL_OPCODE	opcode;

	K10_WWID			id;
	bool				bNumberSet;
	UINT_32				number;
	bool				bNameSet;
	K10_LOGICAL_ID		name;

	UINT_32				diskCount;
	K10_WWID			diskWwn[FRU_COUNT];
} DiskPoolParams;

// an entry in the list of disk pools
typedef struct _DiskPoolEntry {
	K10_WWID			id;
	UINT_32				number;
	K10_LOGICAL_ID		name;

	LBA_T				totalDiskPhysCapacity;
	LBA_T				totalDiskUserCapacity;
	LBA_T				actualDiskUserCapacity;

	LBA_T				totalRGPhysCapacity;
	LBA_T				totalRGUserCapacity;
	LBA_T				freeRGUserCapacity;

	UINT_32				diskCount;
	K10_WWID			diskWwn[FRU_COUNT];

	UINT_32				rgCount;
	K10_WWID			rgWwn[MAX_RGS_PER_DISK_POOL];
} DiskPoolEntry;

//-----------------------------------------------------------Forward Declarations

class DiskPoolListImpl;

//-----------------------------------------------------------DiskPoolList

class CSX_CLASS_EXPORT DiskPoolList
{
public:

	// constructor; initially populates the list
	DiskPoolList(FlareData *pFlareData);

	// destructor
	~DiskPoolList();

	// refresh the disk pool data
	void Refresh();

	// modify a disk pool entry as described by the given DiskPoolParams
	// and using the given Flare poll data
	
	K10_WWID Config(const DiskPoolParams &dpParams);

	// returns count of objects
	long GetCount();		

	// returns a pointer to the indexed object
	DiskPoolEntry * operator[](unsigned long) const;

	// returns index of object if return is true
	bool FindObjectInList( const K10_LU_ID &objectId, int &idx);

	// returns disk pool associated with given disk, or NULL if none
	K10_WWID * GetDiskAssoc(const K10_WWID &wwid);

	// returns disk pool associated with given raid group, or NULL if none
	K10_WWID * GetRGAssoc(const K10_WWID &wwid);

	// dumps the disk pool list to the given file name
	void Dump(const char *pFileName, DISK_POOL_DUMPCODE dumpCode);

	// returns the WWIDAssocHashUtil object for disk associations
	WWIDAssocHashUtil * GetDiskHashUtil();

	// returns the WWIDAssocHashUtil object for raid group associations
	WWIDAssocHashUtil * GetRGHashUtil();

private:

	NPtr <DiskPoolListImpl>	mnpImpl;

};

#endif
