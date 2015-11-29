// MManModepage.h
//
// Copyright (C) EMC Corporation 2001-2006
//
//
// This defines our symbolic refs to modpage type & subtypes
//	TODO: Get AEP headers promoted to \nest\inc, refer directly
//
//
//	Revision History
//	----------------
//	19 Aug 97	D. Zeryck	Initial version.
//	14 Jan 98	D. Zeryck	Add page sizes, etc. for Flare suite
//	03 Nov 98	D. Zeryck	Moved TCD defines into HostAdminLib header
//	17 Dec 98	D. Zeryck	Added bind/unbind defines
//	29 Apr 99	D. Zeryck	Added support for new modepages
//	 9 Dec 99	D. Zeryck	Added support for rev1 bind/unbind modepages
//	15 Feb 00	D. Zeryck	Add stuff for page 3B
//	29 Feb 00	D. Zeryck	Add trespass bytes
//	 2 Jun 00	D. Zeryck	Bug 902
//	14 Aug 00	D. Zeryck	Bug 1617 - add P2B_FLAGS_NOEXIST
//	08 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	15 Dec 00	B. Yang		Add page29 and 3B bytes
//	12 Jan 01	B. Yang		Add send/receive diagnostic page code
//	27 Feb 01	H. Weiner	Add Hi5 RAID types
//  28 Feb 01   K. Keisling Remedy 11206 - Add page 29 LUN & Bind Type bytes
//	10 May 01	H. Weiner	Bug 61365: Allow mode page 0x35 - MM_MPCODE_UNIT_PERF_EXT
//  24 May 01   K. Keisling Performance Stats - add mode page idices
//	12 Jul 01	B. Yang		Added P26_SP_MODEL_NUM
//	24 May 06	P. Caldwell	RAID6 Support

#ifndef _MManModepage_h_
#define _MManModepage_h_

#ifndef SUNBURST_ERRORS_H
#include "sunburst_errors.h"
#include "csx_ext.h"
#endif

//
// A Helper in Bind verification
//
#define MM_FLARE_DB_DISK_COUNT		3
#define MM_FLARE_LUN_MAX_DISK_COUNT 16

//
// CDB codes
//
#define MM_SCSI_SENSE_6_PAGE_CODE	(csx_uchar_t)0x1A
#define MM_SCSI_SELECT_6_PAGE_CODE	(csx_uchar_t)0x15

#define MM_SCSI_SENSE_10_PAGE_CODE	(csx_uchar_t)0x5A
#define MM_SCSI_SELECT_10_PAGE_CODE	(csx_uchar_t)0x55


#define MM_SCSI_WRITEBUF_PAGE_CODE	(csx_uchar_t)0x3B
#define MM_SCSI_READBUF_PAGE_CODE	(csx_uchar_t)0x3C

#define	MM_SCSI_SENDDIAG_PAGE_CODE	(csx_uchar_t)0x1D
#define MM_SCSI_RECVDIAG_PAGE_CODE	(csx_uchar_t)0x1C

#define MM_TLD_CODE			(csx_uchar_t)0xA0
//
// Vendor-unique modepage codes. If they are not in this list, the Redirector
// cannot forward them. This rev-locks to Flare, but the Flare group should be
// using the advanced aray config methodology from now on.
//
//								hex		Func			Recipient	Path
//								----	--------		---------	-------
#define MM_MPCODE_CACHE			0x08	//	Cache			CM			SM-H
#define MM_MPCODE_BIND			0x20	//	Bind			CM			SM-H
#define MM_MPCODE_UNBIND		0x21	//	Unbind			CM			SM-H
#define MM_MPCODE_TRESSPASS		0x22	//	Trespass		CM			SM-H
#define MM_MPCODE_VERIFY		0x23	//	Verify			CM			SM-H
#define MM_MPCODE_MS_POWER		0x24	//	PowerOffDrive	CM			SM-H
#define MM_MPCODE_PEER			0x25	//	PeerSP			CM			SM-H
#define MM_MPCODE_SP_RPT		0x26	//	SP Rpt.			CM			SM-H
#define MM_MPCODE_LUN_RPT		0x27	//	LUN Rpt			CM			SM-H
#define MM_MPCODE_LUN_RPT_EXT	0x29	//	Ext LUN Rpt		CM			SM-H
#define MM_MPCODE_U_CFG_OLD		0x2B	//	Unit Cfg, old	CM			SM-H
#define MM_MPCODE_SP_PERF		0x2C	//	SP Perf Rpt		CM			SM-H
#define MM_MPCODE_UNIT_PERF		0x2D	//	Unit Perf Rpt.	CM			SM-H
#define MM_MPCODE_LOG_CTL		0x2E	//	Unsol.Log Ctl	Logger		Pipe
#define MM_MPCODE_DISK_CRU		0x2F	//	Disk CRU		CM			SM-H
#define MM_MPCODE_DISK_PASS		0x30	//	Disk passthru	CM			SM-H
#define MM_MPCODE_UNSOL_LOG		0x31	//	Unsol Log		Logger		Pipe
#define MM_MPCODE_ENC_CFG		0x33	//	Enclos. Conf.	CM			SM-H
#define MM_MPCODE_DISK_CRU_EXT	0x34	//	Disk CRU Ext.	CM			SM-H
#define MM_MPCODE_UNIT_PERF_EXT	0x35	//	Ext Unit Perf	CM			SM-
#define MM_MPCODE_SYS_CFG		0x37	//	SysCfg.(multi)	CM			SM-H
#define MM_MPCODE_MEM_CFG		0x38	//	MemCfg.(multi)	CM			SM-H
#define MM_MPCODE_RG_INFO		0x3A	//	RaidGrp Info	CM			SM-H
#define MM_MPCODE_RG_CFG		0x3B	//	RaidGrp Cfg		CM			SM-H
#define MM_MPCODE_DISK_CRU_CFG	0x3C	//	Disk CRU Cfg	CM			SM-H

// Send/Recv Diagnostic page codes.
#define MM_MPCODE_DIAG_P80		0x80

// Send/Receive Diagnostic page stuff
#define MM_DIAG_SELFTEST_MASK		0x40
#define MM_DIAG_PCV_MASK			0x01

//-------------------------------- Modepage sizes
// Sizes of modepages built by Admin Engine.

// Actual size of buffers needed. Note that the PageLength is different!
// In decimal or hex depending on how aep_types.h lists them
//
#define MM_PAGE_08_SIZE		12		// cache page - for DAQ
#define MM_PAGE_20_SIZE		0x38	// bind
#define MM_PAGE_21_SIZE		4		// unbind
#define MM_PAGE_21_REV1_SIZE	8		// unbind
#define MM_PAGE_22_SIZE		4		// trespass
#define MM_PAGE_24_SIZE		4		// power page - for DAQ
#define MM_PAGE_25_SIZE		16		// Peer SP
#define MM_PAGE_27_SIZE		2136	// LUN Rpt, still using OLD size
#define MM_PAGE_27_SIZE_NEW 2346	// LUN Rpt, Flare5 addition...
#define MM_PAGE_2B_SIZE		32		// unit Rpt
#define MM_PAGE_2F_SIZE_NEW	0x50	// cru Rpt
#define MM_PAGE_2F_SIZE		0x50	// cru Rpt
#define MM_PAGE_33_MIN_SIZE	12		// Enclosure Rpt
#define MM_PAGE_38_MIN_SIZE	12		// Memory Rpt
#define MM_PAGE_3A_SIZE		212		// Raid group info


// you get this before the data in a real scsi i/f
//
#define MM_MP_10BYTE_PARAM_HEADER_LEN		8
#define MM_MP_BLOCK_DESCR_LEN				8

// values for 'page length' encoded in the pages, which does not include
// the first two bytes. Go figure...
// Anyway, in hex as that's how it is shown in the spec
//
#define MM_MPCODE_CACHE_LEN			0x0A
#define MM_MPCODE_BIND_LEN			0x36
#define MM_MPCODE_UNBIND_LEN		0x02
#define MM_MPCODE_UNBIND_REV1_LEN	0x06
#define MM_MPCODE_TRESPASS_LEN		0x02
#define MM_MPCODE_MSPOWER_LEN		0x02
#define MM_MPCODE_PEER_LEN			0x0E
#define MM_MPCODE_UNIT_LEN			0x1E
#define MM_MPCODE_LUNRPT_LEN		0x0855	// two-byte len field
#define MM_MPCODE_LUNRPT_LEN_NEW	0x0927	// two-byte len field
#define MM_MPCODE_DISK_CRU_LEN_NEW	0x4E
#define MM_MPCODE_DISK_CRU_LEN		0x4E


// Stuff for the 0x3C/0x3B multipurpose page
//
#define MM_3X_MODE_FIELD_EXTENDED	((UCHAR)0x01)

// For page 20 - Bind
// offsets
#define P20_RAIDTYPE_BYTE	3
#define P20_NUMCRUS_BYTE	5
#define P20_REV0_LUN_BYTE	6
#define P20_AA_BYTE			7
#define P20_DEFOWN_BYTE		16
#define P20_AT_FLAG_BYTE	17
#define P20_REV1_LUN_MSB	18
#define P20_CRU_LIST		20
#define P20_FMODE_BYTE		53
#define P20_REV_BYTE		54

// values
#define P20_MAX_CRU			16
#define P20_ATE_MASK		0x04
#define P20_WCE_MASK		0x02

// for page 21 - unbind

#define P21_SIZE_BYTE		1

#define P21_FMODE_BYTE		6

#define P21_REV0_SIZE		2
#define P21_REV1_SIZE		6

#define P21_REV0_LUN_BYTE	2
#define P21_REV1_LUN_MSB	2

// These definitions had better correspond to Flare's
enum MMAN_RAID_TYPE {
	MMAN_RAID_TYPE_MIN = 0,
	MMAN_RAID_TYPE_RAIDNONE = 0,
	MMAN_RAID_TYPE_RAID5,
	MMAN_RAID_TYPE_IDISK,
	MMAN_RAID_TYPE_RAID1,
	MMAN_RAID_TYPE_RAID0,
	MMAN_RAID_TYPE_RAID3,
	MMAN_RAID_TYPE_HS,
	MMAN_RAID_TYPE_RAID10,
	MMAN_RAID_TYPE_Hi5,
	MMAN_RAID_TYPE_Hi5_CACHE,
	MMAN_RAID_TYPE_RAID6,
	MMAN_RAID_TYPE_MAX = MMAN_RAID_TYPE_RAID6
};


// For page 21 - Unbind

#define P21_LUN_BYTE	2

// For page 22, Trespass

#define P22_TCODE_BYTE	2
#define P22_LUN_BYTE	3

// See LIC 4 p 118
enum MMAN_TRESPASS_TYPE {
	MMAN_TRESPASS_TYPE_MIN = 0,
	MMAN_TRESPASS_TYPE_VLU_SG_DEFOWN = 0,
	MMAN_TRESPASS_TYPE_VLU_ONLY,
	MMAN_TRESPASS_TYPE_VLU_SG_CURROWN,
	MMAN_TRESPASS_TYPE_RG,
	MMAN_TRESPASS_TYPE_FLU_SG_DEFOWN = 8,
	MMAN_TRESPASS_TYPE_FLU_ONLY,
	MMAN_TRESPASS_TYPE_FLU_SG_CURROWN,
	MMAN_TRESPASS_TYPE_RG2,
	MMAN_TRESPASS_TYPE_MAX = MMAN_TRESPASS_TYPE_RG2
};

// For 25, Peer page
#define P25_SIG_MSB			4
#define P25_PEER_MSB		8

// For 26, SP Report
#define P26_MICRO_MAJOR_REV			 9
#define P26_MICRO_MINOR_REV			10
#define P26_MICRO_PASS_REV			11
#define P26_MAX_OUTSTANDING_REQS	16
#define P26_AVG_OUTSTANDING_REQS	20
#define P26_BUSY_TICKS				24
#define P26_IDLE_TICKS				28
#define P26_NUM_READ_REQS			32
#define P26_NUM_WRITE_REQS			36
#define P26_SP_MODEL_NUM			 4

// For page 29 - Extended Lun Report
#define P29_LUN_MSB         3
#define P29_RG_MSB			5
#define P29_BIND_TYPE       14


// For Page 2B - unit config

#define P2B_LUN_BYTE		2
#define P2B_FLAGS_BYTE		4

#define P2B_FLAGS_RCE		0x1
#define P2B_FLAGS_WCE		0x2
#define P2B_FLAGS_AA		0x4
#define P2B_FLAGS_AT		0x8
#define P2B_FLAGS_BCV		0x10
#define P2B_FLAGS_NOEXIST	0x20
#define P2B_FLAGS_OWN		0x40

#define P2B_DEFOWN_BYTE		10

// For Page 2C - SP Performance Report
//offsets
#define P2C_TIMESTAMP		10
#define P2C_BUSY_TICKS		62
#define P2C_IDLE_TICKS		66
#define P2C_READ_REQS		70
#define P2C_WRITE_REQS		74
#define P2C_BLOCKS_READ		78
#define P2C_BLOCKS_WRITTEN	82
#define P2C_SUM_QUE_LENGTHS	86
#define P2C_ARRIVALS_NZ_QUE	90

// For Page 2D - Unit Performance Report
//offsets
#define P2D_TIMESTAMP	  	  6
#define P2D_READ_REQS	 	 10
#define P2D_WRITE_REQS	 	 14
#define P2D_BLOCKS_READ		 18
#define P2D_BLOCKS_WRITTEN	 22
#define P2D_LUN				142
#define P2D_MAX_REQS_COUNT	144
#define P2D_SUM_QUE_LENGTHS	152
#define P2D_ARRIVALS_NZ_QUE	160


// For 33 - encl config
#define P33_ENCL_SIZE_EXTRA_BYTES 4

// For Page 35 - Extended Unit Performance Page
//offsets
#define P35_TIMESTAMP	  	  7
#define P35_READ_REQS	 	 11
#define P35_WRITE_REQS	 	 15
#define P35_BLOCKS_READ		 19
#define P35_BLOCKS_WRITTEN	 23
#define P35_LUN				143
#define P35_MAX_REQS_COUNT	145
#define P35_SUM_QUE_LENGTHS	153
#define P35_ARRIVALS_NZ_QUE	161
#define P35_BUSY_TICKS		177
#define P35_IDLE_TICKS		181


// For 3A - Raid info
#define P3A_RG_MSB		2
#define P3A_PER_EXPANDED_BYTE	180
#define P3A_LUN_COUNT_BYTE		39
#define P3A_LUN_LIST			40
#define P3A_FLAGS_BYTE			4
#define P3A_XL_BIT				0x04 


// For page 3B - RAID group config

// offsets
#define P3B_OPCODE_BYTE		3
#define P3B_FLAGS_BYTE		4
#define P3B_RG_MSB			5
#define P3B_NUMCRUS_BYTE	7
#define P3B_CRU_LIST		8

// values
#define P3B_XL_BIT		1

enum P3B_OPCODES {
	P3B_OPCODES_MIN = 1,
	P3B_OPCODES_CREATE = 1,
	P3B_OPCODES_REMOVE,
	P3B_OPCODES_EXPAND,
	P3B_OPCODES_DEFRAG,
	P3B_OPCODES_CONFIG,
	P3B_OPCODES_MAX = P3B_OPCODES_CONFIG
};

// System params
#define K10_ENCLOSURE_SLOTS 10

////////////////////////// From AEP /////////////////////
#define AEP_SYS_CFG_PARAM_INTERFACE_REV_TYPE	1
#define INTERFACE_REVISION_NUMBER				1

/*
 * Parameter ID values for System Configuration mode page
 */
/* General feature */
#define AEP_SYS_CFG_PARAM_CURR_DATE          1
#define AEP_SYS_CFG_PARAM_DAY_OF_WEEK        2
#define AEP_SYS_CFG_PARAM_CURR_TIME          3
/* keep AEP_SYS_CFG_PARAM_SP_INDIRECTION for future use */
/* #define AEP_SYS_CFG_PARAM_SP_INDIRECTION    4 */
#define AEP_SYS_CFG_PARAM_LUN_INDIRECTION    5
#define AEP_SYS_CFG_PARAM_CACHE_PAGE_SIZE    6
#define AEP_SYS_CFG_PARAM_STAT_LOGGING       7
#define AEP_SYS_CFG_PARAM_AUTO_TRESPASS      8
#define AEP_SYS_CFG_PARAM_AUTO_FORMAT        9
#define AEP_SYS_CFG_PARAM_RAID_OPTIMIZATION 10
/* skip classic flare params already used */
#define AEP_SYS_CFG_PARAM_DISK_WRITE        15
#define AEP_SYS_CFG_PARAM_R3_WRITE_BUFFERING  16



#endif
