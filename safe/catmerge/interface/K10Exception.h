// K10Exception.h
//
// Copyright (C) EMC Corporation 1997,1999-2001,2004
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//
// Various global definitions
//
//
//	Revision History
//	----------------
//	14 Dec 99	D. Zeryck	Initial version.
//	28 Feb 00	H. Weiner	Add NTErrorCode, default it to zero.
//	11 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	 3 Aug 01	H. Weiner	Add mHowToHide variable
//	 6 Feb 04	C. Hughes   Add K10_TREE_ROOT_DIRECTORY_NAME

#ifndef _K10_EXCEPTION_H
#define _K10_EXCEPTION_H

#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#ifndef OLD_K10_EXCEPTION_DEFINITON

#ifdef BUILDING_K10_EXCEPTION_DLL
#define K10_EXCEPTION_CLASS_PUBLIC CSX_CLASS_EXPORT
#else
#define K10_EXCEPTION_CLASS_PUBLIC CSX_CLASS_IMPORT
#endif

// This define identifies the top-level directory of the source tree to
// allow the leading portion of file names to be removed.
#define K10_TREE_ROOT_DIRECTORY_NAME "catmerge\\"

//////////////////////////////////////////////////////////////////
//-----------------------------------Class----------------------//
//----------------------------------------------------K10Exception
// This code declares & implements our exception class.
class K10_EXCEPTION_CLASS_PUBLIC K10Exception {
public:
	K10Exception( 	const char *pszDescription, 
						const char *pszFileName,
						long lineNumber,
						long lErrorCode);

	virtual ~K10Exception();

	const char *Description();
	const char *FileName();
	long LineNumber();
	long ErrorCode();
	long NTErrorCode();
	long GetHide();
	void SetNTErrorCode(long NTerr);
	void SetHide(long lHide);

private:
	const char	*mpszDescription;
	const char  *mpszFileName;
	long	mlLineNumber;
	long	mlErrorCode;
	long	mlNTErrorCode;
	long	mHowToHide;
	K10Exception();
};

#else

class K10Exception {
public:
        K10Exception(   const char *pszDescription,
                                                const char *pszFileName,
                                                long lineNumber,
                                                long lErrorCode);

        ~K10Exception(){}

        const char *Description();
        const char *FileName();
        long LineNumber();
        long ErrorCode();
        long NTErrorCode();
        long GetHide();
        void SetNTErrorCode(long NTerr);
        void SetHide(long lHide);

private:
        const char      *mpszDescription;
        const char  *mpszFileName;
        long    mlLineNumber;
        long    mlErrorCode;
        long    mlNTErrorCode;
        long    mHowToHide;
        K10Exception();
};

inline K10Exception::K10Exception( 	
					 	const char *pszDescription, 
						const char *pszFileName,
						long lineNumber,
						long lErrorCode) :
							mpszDescription( pszDescription ), 
							mpszFileName( pszFileName),
							mlLineNumber(lineNumber),
							mlErrorCode( lErrorCode ),
							mlNTErrorCode(0), mHowToHide(0) {}

inline const char *K10Exception::Description() { return mpszDescription; }
inline const char *K10Exception::FileName() { return mpszFileName; }

inline long K10Exception::LineNumber() { return mlLineNumber; }
inline long K10Exception::ErrorCode() { return mlErrorCode; }
inline long K10Exception::NTErrorCode() { return mlNTErrorCode; }
inline long K10Exception::GetHide() { return mHowToHide; }
inline void K10Exception::SetNTErrorCode(long lCode) { mlNTErrorCode = lCode; }
inline void K10Exception::SetHide(long lHide) { mHowToHide = lHide; }

#endif	// OLD_K10_EXCEPTION_DEFINITON

#endif // USERDEFS_H
