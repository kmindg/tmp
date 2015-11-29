// K10AppConstants.h
//
//
// Copyright (C) 1998	Data General Corporation
//
//
//
//	Revision History
//	----------------
//	20 Oct 98	D. Zeryck	Initial version.
//	03 Mar 00	D. Zeryck	Add Critical
//	01 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//
// This file include constants used by K10App and
// also some of the classes it uses: K10LogMsg and K10TraceMsg

#ifndef _K10APPCON_H_
#define _K10APPCON_H_

// not quite redundant: if the accompanying error is not of the K10 format,
// sub with appropriate generic error & put err in payload.
//
enum K10_LOG_SEVERITY
	{
		K10_LOG_SEVERITY_MIN = 1,
		K10_LOG_INFO = K10_LOG_SEVERITY_MIN,
		K10_LOG_WARNING,
		K10_LOG_ERROR,
		K10_LOG_CRITICAL,
		K10_LOG_SEVERITY_MAX = K10_LOG_CRITICAL
	};

enum K10_TRACE_LEVEL 	
	{	
		K10_TRACE_LEVEL_MIN = 1,
		K10_TRACE_ERROR = K10_TRACE_LEVEL_MIN, 
		K10_TRACE_WARNING, 
		K10_TRACE_INFO, 
		K10_TRACE_DATA, 
		K10_TRACE_PATH,
		K10_TRACE_LEVEL_MAX = K10_TRACE_PATH
	};

#endif

