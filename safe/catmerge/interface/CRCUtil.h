//**************************************************************************
// Copyright (C) EMC Corporation 2007-2008
// All rights reserved.
// Licensed material -- property of EMC Corporation
//*************************************************************************

//	The CRCUtil class provides a generic implementation of
//  table-driven CRC-32 and CRC-64 algorithms.
//
//	Revision History
//	----------------
//   1 29 07    R. Hicks    Initial version.
//   8 04 08	R. Hicks	DIMS 204797: Added K10_objVersion structure.


#ifndef CRC_UTIL_H
#define CRC_UTIL_H

#include "k10defs.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

typedef DWORD CHECKSUM32;
typedef ULONGLONG CHECKSUM64;


class  CSX_CLASS_EXPORT CRCUtil
{
public:
	static CHECKSUM32 crc32(csx_uchar_t *buf, DWORD len);

	static CHECKSUM64 crc64(csx_uchar_t *buf, DWORD len);


private:
    static bool init_xor_tables();

	static CHECKSUM32 crc32_xor_table[256];

	static CHECKSUM64 crc64_xor_table[256];

	static bool xor_tables_init;
};

typedef struct K10_objVersion {
	K10_WWID	id;
	ULONGLONG	version;
} K10_ObjVersion, *PK10_ObjVersion;

#endif

