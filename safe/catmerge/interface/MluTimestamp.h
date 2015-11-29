//  MLU Timestamp Definition
//  
//  This module defines the timestamp type used by the MLU driver.
//  
//  Copyright (c) 2009, EMC Corporation.  All rights reserved.
//  Licensed Material -- Property of EMC Corporation.

#ifndef  _MLU_TIMESTAMP_H_
#define  _MLU_TIMESTAMP_H_

#ifdef ALAMOSA_WINDOWS_ENV
#include <basetsd.h>
#else
#include "csx_ext.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */


//  Timestamping
//  
//  The time of occurrence of an MLU event (a "timestamp") is
//  represented as an MLU_TIMESTAMP value.  This value gives the number
//  of 100-nanosecond intervals since midnight GMT on January 1, 1601.
//  (Note that this is the format returned by Windows NT's
//  KeQuerySystemTime function.)

typedef ULONG64 MLU_TIMESTAMP;


#endif  // _MLU_TIMESTAMP_H_
