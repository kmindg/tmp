//K10_IOCTL.h

/*******************************************************************************
* Copyright (C) Data General Corporation, 1997 - 1998
* All rights reserved.
* Licensed material - Property of Data General Corporation
******************************************************************************/
//
// This file is used as an include to allow K10 admin programs to use
// scsitarg.h unmodified.
//
// 04-Aug-98	D. Zeryck	Created
// 25 Aug 99	D. Zeryck	Make driver-friendly, move to nest\inc
//	 1 Mar 00	D. Zeryck	Add NTSTATUS & MAXULONG
//  17 Mar 00	J. Cook		Added SENSE_DATA

#ifndef _K10_IOCTL_H_
#define _K10_IOCTL_H_


//
// some day will go in scsitarg.h

#define FED_NAME      "\\\\.\\ScsiTarg0"


#endif // ndef K10_IOCTL
