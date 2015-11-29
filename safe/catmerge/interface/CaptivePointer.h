// CaptivePointer.h
//
// Copyright (C) 1997	Data General Corporation
// Copyright (C) 2009	EMC Corp
//
//
// Safe, pan-DLL nonleaking wrapper for pointers. 
// Use C++ reference for passing through function calls.
//
//
//	Revision History
//	----------------
//	10 Jul 97	D. Zeryck	Initial version.
//	 3 Dec 98	D. Zeryck	Added CListPtr
//	17 Feb 00	D. Zeryck Fix bug in copy ctor of CListPtr
//	21 Jun 00	D. Zeryck	ASSERT removal
//	21 Sep 00	B. Yang		Task 1938
//	27 Dec 05	M. Brundage	DIM 137265 - Added CListPtr Relinquish()
//	27 May 09	R..Singh	DIM 227973 - CPtr destructor exception removal.
//
//

#ifndef CaptivePointer_h
#define CaptivePointer_h

// Disable pestiferous warnings about templated methods with
// no DLL-interface.
#ifdef _MSC_VER
#endif	//_MSC_VER

#include "UserDefs.h"

#ifndef I_AM_K10_DDK
#include <windows.h>	// GlobalAlloc stuff
#endif

#include "K10GlobalMgmtErrors.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include "EmcUTIL.h"

///////////////////////////////// Class CPtr ////////////////////////////////
//
template <class PtrClass> class CSX_CLASS_EXPORT CPtr
{

	public: ~CPtr( );

	public: CPtr( );

	public:

	CPtr( PtrClass* aPtr );

	PtrClass* operator->( ) const;

	PtrClass* operator*( ) const;

	PtrClass** operator&( ) const;

	PtrClass** Assign();

	const CPtr<PtrClass>&  operator=( PtrClass* aPtr );

	bool IsNull( ) const;

	void Delete( );

	PtrClass* Relinquish( );

	private:

	PtrClass* mPtr;
};


///////////////////////////////// Class NPtr ////////////////////////////////
//
// Uses delete, so appropriate for freeing 'new'ed pointers
//
template <class PtrClass> class CSX_CLASS_EXPORT NPtr
{

	public: ~NPtr( );

	public: NPtr( );

	public:

	NPtr( PtrClass* aPtr );

	PtrClass* operator->( ) const;

	PtrClass* operator*( ) const;

	PtrClass** operator&( ) const;

	PtrClass** Assign();

	const NPtr<PtrClass>&  operator=( PtrClass* aPtr );

	bool IsNull( ) const;

	void Delete( );

	PtrClass* Relinquish( );

	private:

	PtrClass* mPtr;
};



///////////////////////////////// Inline Methods //////////////////////////////
//
// CPtr

template<class PtrClass>
inline CPtr<PtrClass>::~CPtr( ) 
{
	Delete( );
}

template<class PtrClass>
inline CPtr<PtrClass>::CPtr( ) 
: mPtr( NULL )
{
}

template<class PtrClass>
inline CPtr<PtrClass>::CPtr( PtrClass* aPtr) 
: mPtr( aPtr )
{
}


template<class PtrClass>
inline PtrClass* CPtr<PtrClass>::operator->( ) const
{
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mPtr;
}

template<class PtrClass>
inline PtrClass* CPtr<PtrClass>::operator*( ) const
{
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mPtr;
}


template<class PtrClass>
inline PtrClass** CPtr<PtrClass>::operator&( ) const
{
	if ( mPtr == NULL ) { // if you get this, it is because you should be using Assign()
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return ( (PtrClass**) &mPtr);
}

template<class PtrClass>
inline PtrClass** CPtr<PtrClass>::Assign( )
{
	Delete( );
	return ( (PtrClass**) &mPtr);
}


template<class PtrClass>
inline const CPtr<PtrClass>& 
	CPtr<PtrClass>::operator=( PtrClass* aPtr)
{
	Delete( );
	mPtr = aPtr;
	return *this;
}


template<class PtrClass>
inline void CPtr<PtrClass>::Delete( )
{
	if (mPtr != NULL) {
		mPtr = (PtrClass *)EmcutilGlobalFree( mPtr );
	}
}

template<class PtrClass>
inline bool CPtr<PtrClass>::IsNull( ) const
{
	return ( ( bool )( mPtr == NULL ) );
}


template<class PtrClass>
inline PtrClass* CPtr<PtrClass>::Relinquish( )
{
	PtrClass* pP = mPtr;
	mPtr = NULL;
	return pP;
}

///////////////////////////////// Inline Methods //////////////////////////////
//        NPtr
//
template<class PtrClass>
inline NPtr<PtrClass>::~NPtr( ) 
{
	Delete( );
}

template<class PtrClass>
inline NPtr<PtrClass>::NPtr( ) 
: mPtr( NULL )
{
}

template<class PtrClass>
inline NPtr<PtrClass>::NPtr( PtrClass* aPtr) 
: mPtr( aPtr )
{
}


template<class PtrClass>
inline PtrClass* NPtr<PtrClass>::operator->( ) const
{
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mPtr;
}

template<class PtrClass>
inline PtrClass* NPtr<PtrClass>::operator*( ) const
{
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mPtr;
}


template<class PtrClass>
inline PtrClass** NPtr<PtrClass>::operator&( ) const
{
	// if you get this, it is because you should be using Assign()
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return ( (PtrClass**) &mPtr);
}

template<class PtrClass>
inline PtrClass** NPtr<PtrClass>::Assign( )
{
	Delete( );
	return ( (PtrClass**) &mPtr);
}


template<class PtrClass>
inline const NPtr<PtrClass>& 
	NPtr<PtrClass>::operator=( PtrClass* aPtr)
{
	Delete( );
	mPtr = aPtr;
	return *this;
}


template<class PtrClass>
inline void NPtr<PtrClass>::Delete( )
{
	if (mPtr != NULL) {
		delete mPtr;
		mPtr = NULL;
	  }
}

template<class PtrClass>
inline bool NPtr<PtrClass>::IsNull( ) const
{
	return ( ( bool )( mPtr == NULL ) );
}


template<class PtrClass>
inline PtrClass* NPtr<PtrClass>::Relinquish( )
{
	PtrClass* pP = mPtr;
	mPtr = NULL;
	return pP;
}



///////////////////////////////// Class NAPtr ////////////////////////////////
//
// Uses delete[], so appropriate for freeing 'new'ed array pointers
//
template <class PtrClass> class CSX_CLASS_EXPORT NAPtr
{

	public: ~NAPtr( );

	public: NAPtr( );

	public:

	NAPtr( PtrClass* aPtr );

	PtrClass* operator*( ) const;

	PtrClass** operator&( ) const;

	PtrClass** Assign();

	const NAPtr<PtrClass>&  operator=( PtrClass* aPtr );

	bool IsNull( ) const;

	void Delete( );

	PtrClass* Relinquish( );

	private:

	PtrClass* mPtr;
};


///////////////////////////////// Inline Methods //////////////////////////////
//        NAPtr
//
template<class PtrClass>
inline NAPtr<PtrClass>::~NAPtr( ) 
{
	Delete( );
}

template<class PtrClass>
inline NAPtr<PtrClass>::NAPtr( ) 
: mPtr( NULL )
{
}

template<class PtrClass>
inline NAPtr<PtrClass>::NAPtr( PtrClass* aPtr) 
: mPtr( aPtr )
{
}


template<class PtrClass>
inline PtrClass* NAPtr<PtrClass>::operator*( ) const
{
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mPtr;
}


template<class PtrClass>
inline PtrClass** NAPtr<PtrClass>::operator&( ) const
{
	// if you get this, it is because you should be using Assign()
	if ( mPtr == NULL ) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return ( (PtrClass**) &mPtr);
}

template<class PtrClass>
inline PtrClass** NAPtr<PtrClass>::Assign( )
{
	Delete( );
	return ( (PtrClass**) &mPtr);
}


template<class PtrClass>
inline const NAPtr<PtrClass>& 
	NAPtr<PtrClass>::operator=( PtrClass* aPtr)
{
	Delete( );
	mPtr = aPtr;
	return *this;
}


template<class PtrClass>
inline void NAPtr<PtrClass>::Delete( )
{
	if (mPtr != NULL) {
		delete [] mPtr;
		mPtr = NULL;
	  }
}

template<class PtrClass>
inline bool NAPtr<PtrClass>::IsNull( ) const
{
	return ( ( bool )( mPtr == NULL ) );
}


template<class PtrClass>
inline PtrClass* NAPtr<PtrClass>::Relinquish( )
{
	PtrClass* pP = mPtr;
	mPtr = NULL;
	return pP;
}



///////////////////////////////// Class CCharPtrList ////////////////////////////////
//
// Note: a VERY cheesy little list class, but lightweight
//
// Dialogs had difficulty with the original (CListPtr), and we only used it for
// Char*, so forego the cost of templated functions

class CSX_CLASS_EXPORT CCharPtrList
{

	public: ~CCharPtrList( );

	private: CCharPtrList( ){};	// no default!

	public:

	// count must be nonzero
	//
	CCharPtrList( unsigned int count );

	char* operator[]( unsigned int i ) const;

	void Assign( char* p, unsigned int which );

	void Delete( );

	void Delete( unsigned int which );

	void Resize( unsigned int newCount );

	private:

	unsigned int mCount;

	char**	mpp;	
};

///////////////////////////////// Class CListPtr ////////////////////////////////
//
// Note: a VERY cheesy little list class, but lightweight
//
template <class PtrClass> class CSX_CLASS_EXPORT CListPtr
{

	public: ~CListPtr( );

	private: CListPtr( ){};	// no default!

	public:

	// count must be nonzero
	//
	CListPtr( unsigned int count, bool bUseGAlloc = true );

	// copy ctor
	//
	CListPtr( const CListPtr<PtrClass>& clp );

	PtrClass* operator[]( unsigned int i ) const;

	CListPtr<PtrClass>& operator=( const CListPtr<PtrClass>& cpl );

	void Assign( PtrClass* p, unsigned int which );

	void Delete( );

	void Delete( unsigned int which );

	void Resize( unsigned int newCount );

	unsigned int	GetAllocCount() { return mCount;}

	PtrClass* Relinquish(unsigned int which);

	private:

	unsigned int	mCount;
	bool			mbUseGAlloc;

	PtrClass		**mpp;	
};

template<class PtrClass>
inline CListPtr<PtrClass>::CListPtr( unsigned int count,bool bUseGAlloc ): 
	mCount(count), mbUseGAlloc( bUseGAlloc)
{
	if (mCount == 0) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		mpp = (PtrClass **)EmcutilGlobalAllocZeroed( mCount * sizeof(PtrClass *) );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		mpp = new PtrClass *[mCount];
		for (unsigned int ix = 0; ix < mCount; ix++) {
			mpp[ix] = NULL;
		}
	}

}

// copy ctor - you can't use this with variable-length structs!
//
template<class PtrClass>
inline CListPtr<PtrClass>::CListPtr( const CListPtr<PtrClass>& clp )
{
	mbUseGAlloc = clp.mbUseGAlloc;
	mCount = clp.mCount;

#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		mpp = (PtrClass **)EmcutilGlobalAllocZeroed( mCount * sizeof(PtrClass *) );
		for (unsigned int ix = 0; ix < mCount; ix++) {

			if (clp[ix] != NULL) {			
				mpp[ix] = (PtrClass *)EmcutilGlobalAllocZeroed( sizeof(PtrClass));
				memcpy( mpp[ix], clp[ix], sizeof(PtrClass) );
			}
		}
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		mpp = new PtrClass *[mCount];
		for (unsigned int ix = 0; ix < mCount; ix++) {
			if (clp[ix] != NULL) {
				mpp[ix] = new PtrClass;
				memcpy( mpp[ix], clp[ix], sizeof(PtrClass) );
			}
		}
	}
}


template<class PtrClass>
inline CListPtr<PtrClass>::~CListPtr(  )
{
	Delete();
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		EmcutilGlobalFree( mpp );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		delete [] mpp;
	}
}

template<class PtrClass>
inline CListPtr<PtrClass>& CListPtr<PtrClass>::operator=( const CListPtr<PtrClass>& cpl )
{
	// watch out for alloc type used
	//
	Delete();
	//
	// delete internal ptr w/ old alloc type
	//
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		EmcutilGlobalFree( mpp );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		delete [] mpp;
	}
	//
	// now use new alloc type for iternal ptr
	//
	mbUseGAlloc = cpl.mbUseGAlloc;
	mCount = cpl.mCount;
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		mpp = (PtrClass **)EmcutilGlobalAllocZeroed( mCount * sizeof(PtrClass *) );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		mpp = new PtrClass *[mCount];
		for (unsigned int ix = 0; ix < mCount; ix++) {
			mpp[ix] = NULL;
		}
	}
	//
	// alloc & copy elements
	//
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		for (unsigned int ix = 0; ix < mCount; ix++) {
			if (cpl[ix] != NULL) {
				mpp[ix] = (PtrClass *)EmcutilGlobalAllocZeroed( sizeof(PtrClass));
				memcpy( mpp[ix], cpl[ix], sizeof(PtrClass) );
			}
		}
	}
	else 
#endif /* I_AM_DEBUG_EXTENSION */
    {
		for (unsigned int ix = 0; ix < mCount; ix++) {
			if (cpl[ix] != NULL) {
				mpp[ix] = new PtrClass;
				memcpy( mpp[ix], cpl[ix], sizeof(PtrClass) );
			}
		}
	}
	return *this;
}

template<class PtrClass>
inline void CListPtr<PtrClass>::Delete() 
{
	for (unsigned int ix = 0; ix < mCount; ix++) {
		if ( mpp[ix] != NULL ) {
#ifndef I_AM_DEBUG_EXTENSION
			if (mbUseGAlloc) {
				EmcutilGlobalFree( mpp[ix] );
			}
			else
#endif /* I_AM_DEBUG_EXTENSION */
            {
				delete( mpp[ix] );
			}
			 mpp[ix] = NULL;
		}
	}
}

template<class PtrClass>
inline void CListPtr<PtrClass>::Delete( unsigned int which ) 
{
	if ( which < mCount) {
		if ( mpp[which] != NULL ) {
#ifndef I_AM_DEBUG_EXTENSION
			if (mbUseGAlloc) {
				EmcutilGlobalFree( mpp[which] );
			}
			else
#endif /* I_AM_DEBUG_EXTENSION */
            {
				delete mpp[which];
			}
			 mpp[which] = NULL;
		}
	}
	else {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
}

template<class PtrClass>
inline void CListPtr<PtrClass>::Assign( PtrClass* p, unsigned int which ) 
{
#ifndef I_AM_DEBUG_EXTENSION
	if ( which < mCount) {
		Delete( which );
		mpp[which] = p;
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
}

template<class PtrClass>
inline void CListPtr<PtrClass>::Resize( unsigned int newCount ) 
{
	if ( newCount == 0) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	Delete(  );
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		EmcutilGlobalFree( mpp );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		delete [] mpp;
	}
	mCount = newCount;
#ifndef I_AM_DEBUG_EXTENSION
	if (mbUseGAlloc) {
		mpp = (PtrClass **)EmcutilGlobalAllocZeroed( mCount * sizeof(PtrClass *) );
	}
	else
#endif /* I_AM_DEBUG_EXTENSION */
    {
		mpp = new PtrClass *[mCount];
		for (unsigned int ix = 0; ix < mCount; ix++) {
			mpp[ix] = NULL;
		}
	}
}

template<class PtrClass>
inline PtrClass* CListPtr<PtrClass>::operator[](unsigned int i ) const
{
	if (!(i < mCount)) {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}
	return mpp[i];
}

template<class PtrClass>
inline PtrClass* CListPtr<PtrClass>::Relinquish(unsigned int which)
{
	PtrClass* pP = NULL;

	if ( which < mCount) {
		pP = mpp[which];
		mpp[which] = NULL;
	}
	else {
		THROW_K10_EXCEPTION("Illegal use of object",K10_GLOBALMGMT_ERROR_PROG_ERROR);
	}

	return pP;
}


#endif	// CaptivePointer_h

