// safehandle.h
//
// Copyright (C) 1997	Data General Corporation
//
//
// Safe wrapper for handles.
//
//
//	Revision History
//	----------------
//	10 Jul 97	D. Zeryck	Initial version.
//	12 Apr 99	D. Zeryck	Add safe mutex
//	 3 Feb 00	H. Weiner	Add safe find handle
//	 7 Mar 00	B. Yang		move EVENT_ERROR_CREATE_FAILURE to K10GlobalMgmtExport.h
//	18 Nov 04	R. Hicks	Add safe mapview
//   6 Apr 12   M. Dagon    Add mOverlapped to identify handles open with FILE_FLAG_OVERLAPPED
//                          Add OpenSafeHandle

#ifndef _safehandle_h_
#define _safehandle_h_

#include "UserDefs.h"
#include "K10GlobalMgmtErrors.h"

class  SafeHandle {

   public:
		SafeHandle( );
		SafeHandle( void * );
		~SafeHandle();


		long CloseSafeHandle();
		void * operator*( );
		SafeHandle& operator=( SafeHandle & );
#if defined(SAFE_HANDLE_ASSIGNMENT_OP) || (SAFE_HANDLE_USE_WINDOWS)
		SafeHandle& operator=( void *  );
#endif
		bool IsInitialized() {return mInitialized;}
		long OpenSafeHandle(char * deviceName, DWORD ShareMode,DWORD fileAttribute,bool overlapped_io);

   protected:
#ifdef ALAMOSA_WINDOWS_ENV
		void * mHandle;
#else
		void * mHandle;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */
		bool mInitialized;
		bool mOverlapped;
};

#if defined(ALAMOSA_WINDOWS_ENV) || (SAFE_HANDLE_USE_WINDOWS)
// Needs to be a separate class because it uses FindClose() instead
// of CloseHandle().  Would be more elegant if both classes derived
// from a common base class.
class  SafeFindHandle {

   public:
		SafeFindHandle( );
		SafeFindHandle( void * );
		~SafeFindHandle();

		long CloseSafeHandle();
		void * operator*( );
		SafeFindHandle& operator=( SafeFindHandle & );
		SafeFindHandle& operator=( void *  );

		bool IsInitialized() {return mInitialized;}

   protected:
		void * mHandle;
		bool mInitialized;
};
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - DEADCODE ? */

#if defined(ALAMOSA_WINDOWS_ENV) || (SAFE_HANDLE_USE_WINDOWS)
class  SafeMutex {

   public:
		SafeMutex( );
		SafeMutex( void * );
		~SafeMutex();

		long ReleaseSafeMutex();
		long GetSafeMutex( long dwWait );
		void * operator*( );
		SafeMutex& operator=( void *  );
		bool IsInitialized();
		bool IsOwned() {return mOwned;}

   protected:

		void * mHandle;
		bool		mOwned;
};
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - DEADCODE for non-Windows ? */

#if defined(ALAMOSA_WINDOWS_ENV) || (SAFE_HANDLE_USE_WINDOWS)
class  SafeMapView {

   public:
		SafeMapView( );
		SafeMapView( void * );
		~SafeMapView();

		long UnmapSafeMapView();
		void * operator*( );
		SafeMapView& operator=( void *  );
		bool IsInitialized() {return mInitialized;}

   protected:
		void * mBaseAddress;
		bool mInitialized;
};
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - DEADCODE for non-Windows ? */

#if defined(ALAMOSA_WINDOWS_ENV) || (SAFE_HANDLE_USE_WINDOWS)
class MyEvent{
private:
	void *hEvent;

public:
	MyEvent();
	~MyEvent();
	void * operator*();
};
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - DEADCODE ? */




#endif
