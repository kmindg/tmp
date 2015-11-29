// K10CommandProcessing.h
//
// Copyright (C) 2009 EMC Corp
//
//
// Header for K10CommandProcessing exports
//
//	Revision History
//	----------------
//	17 Dec 09	R. Hicks	Initial version
//	23 Dec 09	R. Hicks	Replace Command() with separate methods

#ifndef _K10CommandProcessing_h_
#define _K10CommandProcessing_h_

//----------------------------------------------------------------------INCLUDES

#include "k10defs.h"
#include "flare_cpu_throttle.h"
#ifndef CaptivePointer_h
#include "CaptivePointer.h"
#endif
// this header can't be included in the Navi version of this file
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//--------------------------------------------------------------K10CommandProcessing

enum K10_COMMAND_PROCESSING_CLIENT_TYPE {
	K10_COMMAND_PROCESSING_CLIENT_TYPE_NONE = 0,
	K10_COMMAND_PROCESSING_CLIENT_TYPE_NAVI_USER_COMMAND = NAVICIMOM_ID,
	K10_COMMAND_PROCESSING_CLIENT_TYPE_NDU = NDU_ID,
	K10_COMMAND_PROCESSING_CLIENT_TYPE_MAX = K10_COMMAND_PROCESSING_CLIENT_TYPE_NDU
};

#ifndef CSX_CLASS_EXPORT
#define CSX_CLASS_EXPORT CSX_MOD_EXPORT
#endif

class K10CommandProcessingImpl;

class CSX_CLASS_EXPORT K10CommandProcessing  
{
public:
	K10CommandProcessing(K10_COMMAND_PROCESSING_CLIENT_TYPE clientID);
	~K10CommandProcessing();

	HRESULT GetCPUThrottleFeature(BOOL &enabled) const;
	HRESULT SetCPUThrottleFeature(BOOL enabled) const;

	HRESULT NeedsCPU(long timer) const;

	HRESULT ProcessingCompleted() const;

	HRESULT ResetCPUThrottle() const;

private:
	NPtr <K10CommandProcessingImpl> mpImpl;
};

#endif
