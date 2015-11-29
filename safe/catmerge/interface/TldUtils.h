#ifndef TLDUTILS_H
#define	TLDUTILS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * TldUtils.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains 
 *
 * NOTES:
 *
 * HISTORY:
 *
 * 12/21/2001 - BMR - Created.
 * 01/07/2002 - BMR - took HRESULT out of the picture on finds; now throw
 * 01/20/2003 - JJ  - Move class TldUtils to K10GlobalMgmt
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include <stddef.h>
#include "tld.hxx"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#include <windows.h>	

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

class CSX_CLASS_EXPORT TldUtils
{
  public:
    enum TU_OPT
    {
        TU_NO_OPTION,
        TU_ADD_LIST_TLD_ALSO
    };
    
    TldUtils() {};
    ~TldUtils() {};
    
    TLD *tuFindList(const TLD *pTld);
    TLD *tuFindId(const TLD *pListTld);
    TLD *tuFindAssoc(const TLD *pTld);
    
    TLD *tuFindNestedTag(const TLD *pSourceTld,
                         TLDtag searchTag);
    
    void tuBuildAltIdKeyReturnTree(TLDdata *pKey,
                                   const ULONG keyLength,
                                   TLD **ppTldRet,
                                   const TU_OPT addList = TU_NO_OPTION);

    const TLD *tuFindKeyForAssoc(const TLD *pAssocTld,
                                 TLDtag searchTag);
    
  private:

};


/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/*
 * End 
 */

#endif //TLDUTILS_H
