//**************************************************************************
// Copyright (C) EMC Corporation 2007-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//*************************************************************************

//	The WWIDAssocHashUtil class provides an implementation of
//  a hash table to do a one-to-one mapping of a WWID to a value 
//  it is associated with.
//
//	Revision History
//	----------------
//   12 04 07    R. Hicks    Initial version.
//   08 04 08    R. Hicks    Make it possible to associate to a value other 
//                           than a WWID.
//   12 24 08    R. Singh    Added constructor, destructor and Init function prototypes
//                           to WWIDAssocHashUtil class.
//   01 06 09    R. Hicks    Make it possible to associate to a WWIDAssocValue.

#ifndef WWID_ASSOC_HASH_UTIL_H
#define WWID_ASSOC_HASH_UTIL_H

#include "k10defs.h"
#include "CaptivePointer.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

typedef union _WWIDAssocValue
{
	K10_WWID	wwid;
	LONG		lValue;
	ULONGLONG	llValue;
	void		*pObj;
} WWIDAssocValue;

// hash table entry mapping an assocation to some value
typedef struct _WWIDAssocHashEntry
{
	bool			inUse;
	bool			collision;
	K10_WWID		wwid;
	WWIDAssocValue	assocVal;
} WWIDAssocHashEntry;

//------------------------------------------------------------WWIDAssocHashUtil

class  CSX_CLASS_EXPORT WWIDAssocHashUtil
{
public:

	// constructor
	WWIDAssocHashUtil(DWORD baseSize, 
		K10TraceMsg *pTraceMsg, const char *idStr);

	//constructor
	WWIDAssocHashUtil(DWORD baseSize, 
		const char *componentStr, const char *idStr);

	//destructor
	~WWIDAssocHashUtil();

	// clear the hash table
	void ClearHashTable();

	// return the WWID associated with the given WWID
	void GetAssoc(const K10_WWID &wwid, K10_WWID **ppAssocVal);

	// add a new assocation in the hash table for the given WWID pair
	void AddEntry(const K10_WWID &wwid, const K10_WWID &assocWwid);

	// remove the hash table entry having the given WWID
	void RemoveEntry(const K10_WWID &wwid);

	// remove all hash table entries associated to the given WWID
	void PurgeAssocEntries(const K10_WWID &wwid);

	// return the ULONGLONG value associated with the given WWID
	void GetAssoc(const K10_WWID &wwid, ULONGLONG **ppAssocVal);

	// add a new assocation in the hash table for the given WWID to 
	// ULONGLONG mapping
	void AddEntry(const K10_WWID &wwid, const ULONGLONG &assocVal);

	// return the WWID associated with the given WWID
	void GetAssoc(const K10_WWID &wwid, WWIDAssocValue **ppAssocVal);

	// add a new assocation in the hash table for the given WWID to 
	// pointer mapping
	void AddEntry(const K10_WWID &wwid, const WWIDAssocValue &assocVal);

private:

	// get the hash table location for the given WWID
	DWORD GetHashKey(const K10_WWID &wwid);

	// return the hash table entry having the given WWID, or find its new location
	// if bAdd is true
	WWIDAssocHashEntry * FindHashEntry(const K10_WWID &wwid, bool bAdd);

	// initialization of hash table
	void Init(DWORD baseSize);

	// Attributes

	NAPtr <WWIDAssocHashEntry>	mnpHashTable;
	DWORD mHashTableSize;		// number of hash table entries

	const char *mIdStr;
	long mHashCollisions;
	K10TraceMsg *mpK;
	bool mbFreeKTraceMsg;
};

#endif

