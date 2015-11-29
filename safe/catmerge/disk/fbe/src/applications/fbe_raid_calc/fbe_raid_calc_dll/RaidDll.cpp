/*******************************************************************
 * Copyright (C) EMC Corporation 1997 - 2002
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 * RaidDll.cpp : Defines the entry point for the DLL application.
 *
 * RaidDll - this is a very simple Dll, which is used to
 *           gain access to the Calculations used by the 
 *           Raid Calculator.
 *
 * Use:     The following interface functions are provided:
 *
 *          RaidCalcLogicalToPhysical()
 *          RaidCalcPhysicalToLogical()
 *          RaidCalcSetWidth()
 *          RaidCalcInit()
 *          
 *
 *          Use of the Raid Dll is further described in the
 *          header fbe_raid_calc.h.
 *
 *
 *******************************************************************/

#include "StdAfx.h"

#define RAIDDLL_EXPORTS 1

#include "CRaid.h"
#ifndef ALAMOSA_WINDOWS_ENV
#include <new>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

extern "C" 
{
    void __cdecl Generate( unsigned int width, 
                           unsigned int sectors_per_element,
                           ULONGLONG address_offset,
                           unsigned int raid_type,
                           ULONGLONG lba,
                           unsigned int blocks, 
                           unsigned int opCode,
                           RAID_GEN_STRUCT *RaidGenStruct);
        
};

#ifdef ALAMOSA_WINDOWS_ENV
static int reference_count = 0;
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (reference_count > 0) {
                return FALSE;
            }
            else {
                reference_count++;
            }
            break;

        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:

            if (reference_count < 1) {
                return FALSE;
            }
            else {
                reference_count--;
            }
            break;
    }
    return TRUE;
}
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

class CRaidException
{
public:
    CRaidException(){};
    ~CRaidException(){};
    const char *ShowReason() const { return "Exception in RaidCalcDll.  PBA < Address Offset !!"; }

};
unsigned int FlarePosToCalcPos( unsigned int mWidth, unsigned int mPos )
{   
    unsigned int newPos;
    unsigned int hWidth = mWidth / 2;
    
    if ( mPos < hWidth )
    {
        newPos = (mPos * 2);
    }
    else
    {
        // Subtract hWidth to get a pair number.
        // mult by 2 and add one to get secondary.
        newPos = ((mPos - hWidth) * 2 ) + 1;
    }
    return newPos;
}

unsigned int CalcPosToFlarePos( unsigned int mWidth, unsigned int mPos )
{
    unsigned int newPos;
    unsigned int hWidth = mWidth / 2;
    
    if ( (mPos % 2) == 0 )
    {
        newPos = mPos / 2;
    }
    else
    {
        newPos = (mPos / 2) + hWidth;
    }
    return newPos; 
}

/****************************************************************
 * RaidCalcLogicalToPhysical()
 ****************************************************************
 * DESCRIPTION:
 *  Do a logical to physical lba (logical block address)
 *  conversion using the information contained in the
 *  input CalcStruct.
 *
 * PARAMETERS:
 *  CalcStruct, [IO] - ptr to CalcStructure.
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   06/06/02 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcLogicalToPhysical(RAID_CALC_STRUCT &CalcStruct)
{
    
    *CalcStruct.raidObj = CalcStruct;
#if 0
    // First make sure that the Layout in the RaidObject is setup
    // according to the platform field in the CalcStruct

    if (CalcStruct.platform != RAID_CALC_LONGBOW) {
        CalcStruct.raidObj->raidLayout = (R5_DATA_LAYOUT) RAID_RIGHT_SYMMETRIC;
    }
    else {
        CalcStruct.raidObj->raidLayout = (R5_DATA_LAYOUT) RAID_RIGHT_ASYMMETRIC;
    }

    // For the next set of fields, copy them directly from the
    // CalcStruct. 

    CalcStruct.raidObj->raidType = (RAID_TYPE) CalcStruct.raidType;
    
    CalcStruct.raidObj->sectorsPerElement = CalcStruct.sectorsPerElement;
    
    CalcStruct.raidObj->addressOffset[0] = CalcStruct.AddressOffset;
    CalcStruct.raidObj->SetArrayWidth(CalcStruct.ArrayWidth);
    
    CalcStruct.raidObj->lba = CalcStruct.Lba;
    
    CalcStruct.raidObj->pba = CalcStruct.Pba;
    CalcStruct.raidObj->segmentOffset = CalcStruct.Hi5SegmentOffset;
    CalcStruct.raidObj->twoGB = (CalcStruct.platform != RAID_CALC_LONGBOW);
    CalcStruct.raidObj->calcSegDb = CalcStruct.Hi5SegmentDb;
    CalcStruct.raidObj->dataPos = CalcStruct.dataPos;
    CalcStruct.raidObj->parityPos = CalcStruct.parityPos;
    CalcStruct.raidObj->parityPos2 = CalcStruct.parityPos2;
#endif  
    // Since we are doing a Logical -> Physical conversion, we
    // will set the logicalMode flag in the raidObj to TRUE.

    CalcStruct.raidObj->logicalMode = TRUE;

    // Do the calculations.

    CalcStruct.raidObj->calcLba();
    

    // For Raid 10 we will do some manipulations

    if ( CalcStruct.raidObj->raidType == RAID10 )
    {
        CalcStruct.raidObj->dataPos = CalcPosToFlarePos( CalcStruct.raidObj->arrayWidth,
                                                         CalcStruct.raidObj->dataPos );
        CalcStruct.raidObj->parityPos = CalcPosToFlarePos( CalcStruct.raidObj->arrayWidth,
                                                           CalcStruct.raidObj->parityPos );
    }
    
    // Copy the results of the calculations
    // back into the CalcStruct.

    CalcStruct.sectorsPerElement = CalcStruct.raidObj->sectorsPerElement;
    
    CalcStruct.AddressOffset = CalcStruct.raidObj->addressOffset[0];
    CalcStruct.ArrayWidth = CalcStruct.raidObj->arrayWidth;
        
    CalcStruct.Pba = CalcStruct.raidObj->pba;
    CalcStruct.Hi5SegmentOffset = CalcStruct.raidObj->segmentOffset;
    CalcStruct.Hi5SegmentDb = CalcStruct.raidObj->calcSegDb;
    CalcStruct.dataPos = CalcStruct.raidObj->dataPos;
    CalcStruct.parityPos = CalcStruct.raidObj->parityPos;
    CalcStruct.parityPos2 = CalcStruct.raidObj->parityPos2;
    return TRUE;
}
/* end RaidCalcLogicalToPhysical() */



/****************************************************************
 * RaidCalcPhysicalToLogical()
 ****************************************************************
 * DESCRIPTION:
 *  Do a physical to logical lba (logical block address)
 *  conversion using the information contained in the
 *  input CalcStruct.
 *
 * PARAMETERS:
 *  CalcStruct, [IO] - ptr to CalcStructure.
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   06/06/02 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcPhysicalToLogical(RAID_CALC_STRUCT &CalcStruct)
{
    *CalcStruct.raidObj = CalcStruct;

    // Since we are doing a Physical -> Logical conversion, we
    // will set the logicalMode flag in the raidObj to FALSE.
    
    CalcStruct.raidObj->logicalMode = FALSE;

    if ( CalcStruct.raidObj->pba < CalcStruct.raidObj->addressOffset[0] )
    {
#ifdef ALAMOSA_WINDOWS_ENV
        MessageBox(NULL, "PBA is less than Address Offset", "Error in RaidCalcDll", MB_OK );
#else
        fprintf(stderr, "Error in RaidCalcDll: %s\n", "PBA is less than Address Offset");
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
        
        throw CRaidException();
    }

    // For Raid 10 we will do some manipulations to
    // validate and convert our primary/secondary position.
    //
    // The calculator and Flare always displays the disk set as:
    //  Pri Pri Pri Sec Sec Sec
    //
    // array
    // position:    0     1     2     3     4     5     6     7
    //            Pri-0 Pri-1 Pri-2 Pri-3 Sec-0 Sec-1 Sec-2 Sec-3

    if ( CalcStruct.raidObj->raidType == RAID10 )
    {
        CalcStruct.raidObj->dataPos = FlarePosToCalcPos( CalcStruct.raidObj->arrayWidth,
                                                         CalcStruct.raidObj->dataPos );

        // If the user gave us a secondary position, then
        // convert it to a primary position.

        if ( (CalcStruct.raidObj->dataPos % 2) != 0 )
        {
            CalcStruct.raidObj->dataPos--;
        }
        // Always just assign the secondary position based on what our primary position is.
        CalcStruct.raidObj->parityPos = CalcStruct.raidObj->dataPos + 1;
    }
    
    // For Raid 1, if we got a secondary position, convert it to primary.

    if ( CalcStruct.raidObj->raidType == RAID1 )
    {
        CalcStruct.raidObj->dataPos = 0;
        CalcStruct.raidObj->parityPos = 1;
    }
    // Do the calculations.

    CalcStruct.raidObj->calcLba();
    

    // For Raid 10 we will do some manipulations

    if ( CalcStruct.raidObj->raidType == RAID10 )
    {
        CalcStruct.raidObj->dataPos = CalcPosToFlarePos( CalcStruct.raidObj->arrayWidth,
                                                         CalcStruct.raidObj->dataPos );
        CalcStruct.raidObj->parityPos = CalcPosToFlarePos( CalcStruct.raidObj->arrayWidth,
                                                           CalcStruct.raidObj->parityPos );
    }
    // Copy the results of the calculations
    // back into the CalcStruct.

    CalcStruct.Lba = CalcStruct.raidObj->lba - CalcStruct.raidObj->bindOffset;
    CalcStruct.Pba = CalcStruct.raidObj->pba;
    CalcStruct.dataPos = CalcStruct.raidObj->dataPos;
    CalcStruct.parityPos = CalcStruct.raidObj->parityPos;
    CalcStruct.parityPos2 = CalcStruct.raidObj->parityPos2;

    return TRUE;
} 
/* end RaidCalcPhysicalToLogical() */


/****************************************************************
 * RaidCalcSetWidth()
 ****************************************************************
 * DESCRIPTION:
 *  Setup the width in the CalcStruct using an input width.
 *  Note that we make several validation checks along the way.
 *
 * PARAMETERS:
 *  CalcStruct, [IO] - ptr to CalcStructure.
 *  width,      [I]  - new width to set in CalcStruct
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   06/06/02 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcSetWidth(RAID_CALC_STRUCT &CalcStruct,
                                  unsigned int width)
{
    // First make sure that the Layout in the RaidObject is setup
    // according to the platform field in the CalcStruct

    if (CalcStruct.platform == RAID_CALC_LONGBOW) {
        CalcStruct.raidObj->raidLayout = RAID_RIGHT_ASYMMETRIC;
    }
    else {
        CalcStruct.raidObj->raidLayout = RAID_RIGHT_SYMMETRIC;
    }

    // Copy values from the CalcStruct to the raidObject.

    CalcStruct.raidObj->raidType = (RAID_TYPE) CalcStruct.raidType;
    
    CalcStruct.raidObj->sectorsPerElement = CalcStruct.sectorsPerElement;
    
    CalcStruct.raidObj->addressOffset[0] = CalcStruct.AddressOffset;
    CalcStruct.raidObj->SetArrayWidth(CalcStruct.ArrayWidth);
    
    CalcStruct.raidObj->lba = CalcStruct.Lba;
    
    CalcStruct.raidObj->pba = CalcStruct.Pba;
    CalcStruct.raidObj->segmentOffset = CalcStruct.Hi5SegmentOffset;
    CalcStruct.raidObj->twoGB = (CalcStruct.platform != RAID_CALC_LONGBOW);
    CalcStruct.raidObj->calcSegDb = CalcStruct.Hi5SegmentDb;
    CalcStruct.raidObj->dataPos = CalcStruct.dataPos;
    CalcStruct.raidObj->parityPos = CalcStruct.parityPos;
    CalcStruct.raidObj->parityPos2 = CalcStruct.parityPos2;
    CalcStruct.raidObj->SetArrayWidth(width);
    CalcStruct.ArrayWidth = CalcStruct.raidObj->arrayWidth;
    return TRUE;
}
/* end RaidCalcSetWidth() */

/****************************************************************
 * RaidCalcSetElementSize()
 ****************************************************************
 * DESCRIPTION:
 *  Setup the ElementSize in the CalcStruct using an input width.
 *  Note that we make several validation checks along the way.
 *
 * PARAMETERS:
 *  CalcStruct,  [IO] - ptr to CalcStructure.
 *  ElementSize, [I]  - new ElementSize to set in CalcStruct
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   06/18/03 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcSetElementSize(RAID_CALC_STRUCT &CalcStruct,
                                  unsigned int ElementSize)
{
    // First make sure that the Layout in the RaidObject is setup
    // according to the platform field in the CalcStruct

    if (CalcStruct.platform == RAID_CALC_LONGBOW) {
        CalcStruct.raidObj->raidLayout = RAID_RIGHT_ASYMMETRIC;
    }
    else {
        CalcStruct.raidObj->raidLayout = RAID_RIGHT_SYMMETRIC;
    }

    // Copy values from the CalcStruct to the raidObject.

    CalcStruct.raidObj->raidType = (RAID_TYPE) CalcStruct.raidType;
    
    CalcStruct.raidObj->sectorsPerElement = CalcStruct.sectorsPerElement;
    
    CalcStruct.raidObj->addressOffset[0] = CalcStruct.AddressOffset;
    CalcStruct.raidObj->SetArrayWidth(CalcStruct.ArrayWidth);
    
    CalcStruct.raidObj->lba = CalcStruct.Lba;
    
    CalcStruct.raidObj->pba = CalcStruct.Pba;
    CalcStruct.raidObj->segmentOffset = CalcStruct.Hi5SegmentOffset;
    CalcStruct.raidObj->twoGB = (CalcStruct.platform != RAID_CALC_LONGBOW);
    CalcStruct.raidObj->calcSegDb = CalcStruct.Hi5SegmentDb;
    CalcStruct.raidObj->dataPos = CalcStruct.dataPos;
    CalcStruct.raidObj->parityPos = CalcStruct.parityPos;
    CalcStruct.raidObj->parityPos2 = CalcStruct.parityPos2;
    CalcStruct.raidObj->SetElementSize(ElementSize);
    CalcStruct.sectorsPerElement = CalcStruct.raidObj->sectorsPerElement;
    return TRUE;
}
/* end RaidCalcSetWidth() */

/****************************************************************
 * RaidCalcInit()
 ****************************************************************
 * DESCRIPTION:
 *  Setup the CalcStruct using default values for all fields.
 *
 * PARAMETERS:
 *  CalcStruct, [IO] - ptr to CalcStructure.
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   06/06/02 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcInit(RAID_CALC_STRUCT &CalcStruct)
{
    CalcStruct.raidObj = (CRaid *)CalcStruct.Reserved;
    
    if (CalcStruct.raidObj->Initialized == FALSE)
    {
#ifndef ALAMOSA_WINDOWS_ENV
        new (CalcStruct.raidObj) CRaid();
#else
        CalcStruct.raidObj->CRaid::CRaid();
#endif /* ALAMOSA_WINDOWS_ENV - C++MESS - placement new differences */
        CalcStruct.raidObj->Initialized = TRUE;
    }
    
    CalcStruct.AddressOffset = RAID_DEFAULT_ADDRESS_OFFSET;
    CalcStruct.elementsPerParityStripe = RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE;
    CalcStruct.AlignmentLba = 0;
    CalcStruct.sectorsPerElement = RAID_DEFAULT_SECTORS_PER_ELEMENT;
    CalcStruct.ArrayWidth = RAID_DEFAULT_ARRAY_WIDTH;
    CalcStruct.raidType = RAID_DEFAULT_RAID_TYPE;
    CalcStruct.Lba = 0;
    CalcStruct.Pba = 0;
    CalcStruct.dataPos = 0;
    CalcStruct.parityPos = 0;
    CalcStruct.parityPos2 = 0;
    CalcStruct.platform = RAID_CALC_CX;
    CalcStruct.Hi5SegmentOffset = 0; /* Just for Hi-5 Stores */
    return TRUE;
}
/* end RaidCalcInit() */

/****************************************************************
 * RaidCalcGenerate()
 ****************************************************************
 * DESCRIPTION:
 *  Calculate the exact sizes that the raid driver generates.
 *
 * PARAMETERS:
 *  CalcStruct,    [IO] - ptr to RAID_CALC_STRUCT.
 *  RaidGenStruct, [IO] - ptr to RAID_GEN_STRUCT.
 *
 * RETURN VALUE:
 *   TRUE
 *
 * HISTORY:
 *   04/11/03 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API bool RaidCalcGenerate(RAID_CALC_STRUCT &CalcStruct,
                                  RAID_GEN_STRUCT &RaidGenStruct)
{  
	#if 0/*REMOVE FLARE*/
    *CalcStruct.raidObj = CalcStruct;
    
    Generate( CalcStruct.ArrayWidth,
              CalcStruct.sectorsPerElement,
              CalcStruct.AddressOffset,
              CalcStruct.raidType,
              CalcStruct.raidObj->lba, /* Use this LBA, since it includes bind offset */
              RaidGenStruct.TotalBlocks,
              RaidGenStruct.OpCode,
              &RaidGenStruct );
	#endif
   
    return TRUE;
}
/* end RaidCalcGenerate() */


/****************************************************************
 * RaidCalcGetDataDisks()
 ****************************************************************
 * DESCRIPTION:
 *  Calculate the number of data drives
 *
 * PARAMETERS:
 *  CalcStruct,    [IO] - ptr to RAID_CALC_STRUCT.
 *
 * RETURN VALUE:
 *   unsigned int, number of data drives
 *
 * HISTORY:
 *   06/20/03 - Created. RPF
 *
 ****************************************************************/

RAIDDLL_API unsigned int RaidCalcGetDataDisks(RAID_CALC_STRUCT &CalcStruct)
{
    *CalcStruct.raidObj = CalcStruct;
    return CalcStruct.raidObj->GetDataDisks();
}
/* end RaidCalcGetDataDisks() */

// This is the constructor of a class that has been exported.
// see RaidDll.h for the class definition
CRaidDll::CRaidDll()
{ 
    return; 
}

/*******************************
 * end RaidDll.cpp
 *******************************/
