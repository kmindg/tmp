/*
 * mut_exception_processing.h
 * 
 * Created On: Feb 21, 2012
 * Author    : londhn
 */

#ifndef MUT_EXCEPTION_PROCESSING_H
#define MUT_EXCEPTION_PROCESSING_H

#include <windows.h>

#include "simulation/Program_Option.h"

class MutExceptionProcessing: public Bool_option {
public:

	static void setHandler();

private:

	static void addUnhandledExceptionHandler();

	static void removeUnhandledExceptionHandler();

	static long WINAPI UnhandledExceptionHandler(LPEXCEPTION_POINTERS ExceptionInfo);
	 
	static LPTOP_LEVEL_EXCEPTION_FILTER pUnhandledExceptionHandler;

};

#endif