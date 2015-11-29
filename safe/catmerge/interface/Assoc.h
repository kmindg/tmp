//**************************************************************************
// Copyright (C) EMC Corporation 2001-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//*************************************************************************

//	Revision History
//	----------------
//	10 22 01	H. Weiner		Initial version
//	03 27 02	C. Hopkins		Added support for TLD references
//  01 20 03    J. Jin          Move this file under K10GlobalMgmt
//	19 Aug 09	R. Hicks		DIMS 235303: use AddPeer() to append to the
//								end of the list of associations
//

#ifndef _Assoc_H
#define _Assoc_H

#include <stdio.h>
#include <stddef.h>
#include <windows.h>
#include "UserDefs.h"
#include "k10defs.h"
#include "tld.hxx"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

class CSX_CLASS_EXPORT Assoc
{
public:
	// Constructor that sets up the association branch.  Requires that Add() be called
	// at least once.
	// pTldAssociation	- pointer to an Association tag, to which the new branch will be attached
	// dwObjectTypeTag	- Tag indentifying type of object being associated: TAG_SP, TAG_ENCLOSURE, TAG_LUN, etc.
	// dwParentTag		- Tag identifying the general class of object:  TAG_PHYSICAL or TAG_LOGICAL
	// iSize			- Number of bytes of key data
	// pkeyData			- Pointer to the key data
	Assoc( TLD *pTldAssociation, DWORD dwObjectTypeTag, DWORD dwParentTag );

	// All in one constructor.  Sets up association branch with the key wwn.  Additional keys
	// can be added via the Add() method.
	Assoc( TLD *pTldAssociation, DWORD dwTag, DWORD dwObjectTypeTag, int iSize, unsigned char*pkeyData );

	~Assoc() {};


	// Add a key to the association branch
	void Add( int iSize, unsigned char*pkeyData );
	void * Add( int iSize, unsigned char*pkeyData, TLD * pRef );
	void Add( DWORD pkeyData );
	void * Add( DWORD pkeyData, TLD * pRef );

protected:
	// Set up the Association branch, but without any Keys yet.
	void BuildBranch();

	TLD		*mpTldHead;				// where we save pTldAssociation
	TLD		*mnpTldAttachPoint;		// where key sub-branch is attached
	TLD		*mpTldEndOfList;			// current end of list 

	DWORD	mdwObjectTypeTag;
	DWORD	mdwParentTag;

};

#endif
