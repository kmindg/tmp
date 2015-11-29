/*******************************************************************************
 * Copyright (C) Data General Corporation, 1998
 * All rights reserved.
 * Licensed material - Property of Data General Corporation
 ******************************************************************************/

/*******************************************************************************
 * minipath.h
 *
 * This file defines a structure that is used to specify an Initiator-to-LUN 
 * path.
 *
 * Notes:
 *
 * History:
 *
 *	 07/98	Dan Lewis created.
 *
 ******************************************************************************/

#ifndef _MINIPATH_H_
#define _MINIPATH_H_

typedef struct _INITIATOR_ID
{
	
	ULONGLONG	IdHi;	// Node WWN for FibreChannel, 0 for ParallelScsi.
	ULONGLONG	IdLo;	// Port WWN for FibreChannel, Init. ID for ParallelScsi.

} INITIATOR_ID, *PINITIATOR_ID;

typedef UCHAR BUS_ID;
typedef UCHAR TARGET_ID;
typedef ULONGLONG LUN, *PLUN;

#define	AMODE_PERIPHERAL_DEVICE_ADDRESSING	0x00
#define	AMODE_VOLUME_SET_ADDRESSING			0x01
#define	AMODE_LOGICAL_UNIT_ADDRESSING		0x02

#define PATH_EQ(b1, b2) ((b1).login_context == (b2).login_context	\
   && (b1).lun.lun == (b2).lun.lun && (b1).tag_id == (b2).tag_id)

#define InitiatorLun_EQ(b1, b2)						\
	(((b1).login_context == (b2).login_context) &&	\
	((b1).lun.lun == (b2).lun.lun))

#endif //_MINIPATH_H_
