/*******************************************************************
 * Copyright (C) EMC Corporation 2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/
/***************************************************************************
 *  CRaid.cpp
 ***************************************************************************
 *
 *
 * DESCRIPTION
 *
 *  This file contains functions for the CRAID Class.
 *
 * TABLE OF CONTENTS
 *
 *  CRaid::CRaid()
 *  CRaid::InitDefaults()
 *  CRaid::SetArrayWidth()
 *  CRaid::calcLba()
 *  CRaid::calcRaid0Logical()
 *  CRaid::calcRaid0Physical()
 *  CRaid::calcRaid10Logical()
 *  CRaid::calcRaid10Physical()
 *  CRaid::calcRaid5Logical()
 *  CRaid::calcRaid5Physical()
 *  CRaid::calcHi5()
 *
 * HISTORY
 *
 *  5/2000 - Rob Foley, Created.
 *
 ***************************************************************************/
#include "StdAfx.h"
#include "CRaid.h"

#ifndef ASSERT
#define ASSERT(f) \
	do \
	{ \
	if (!(f)) \
		DebugBreak(); \
	} while (0)
#else
#pragma message("Assert already included craid.cpp")
#endif
#ifndef TRACE
//#define TRACE              DebugPrint
void TRACE(char *fmt, ...){return; }
#else
#pragma message("TRACE already included craid.cpp")
#endif

CRaid::CRaid()
{
	InitDefaults();
}
void CRaid::InitDefaults()
{
	int i;
	for (i=0; i < RAID_MAX_DISK_ARRAY_WIDTH; i++)
	{
		addressOffset[i] = RAID_DEFAULT_ADDRESS_OFFSET;
	}

	sectorsPerElement = RAID_DEFAULT_SECTORS_PER_ELEMENT;
	arrayWidth = RAID_DEFAULT_ARRAY_WIDTH;
	elementsPerParityStripe = RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE;
	raidType = RAID_DEFAULT_RAID_TYPE;
	raidProduct = RAID_LONGBOW_PHASE2;
	lba = 0;
	pba = 0;
	dataPos = 0;
	parityPos = 0;
	twoGB = 1;
	logicalMode = 1;
	segmentOffset = 0; /* Just for Hi-5 Stores */
	raidLayout = RAID_RIGHT_SYMMETRIC;
	return;
}
bool CRaid::SetArrayWidth( const unsigned int ArrayWidth)
{
	/* Most of the time it is not invalid
	 * Just because its nonstandard.
	 */
	bool m_bValid = RAID_WIDTH_VALID(raidType, ArrayWidth);

	switch (raidType)
	{
		case AGGREGATE:	
		case RAID0:
			if (( ArrayWidth == 1 ) ||
				( ArrayWidth == 2 )) {
				//m_nCheckStandard = TRUE;
			}
			break;

		case IND_DISK:
		case HI5:
		case RAID5:
		case RAID1:
        case RAID6:
			break;

		case RAID10:
			if ((ArrayWidth % 2) != 0) {
				m_bValid = FALSE;
			}
			break;

		case RAID3:
			if (((ArrayWidth != 4) &&(ArrayWidth != 5) && (ArrayWidth != 9)) && !m_bValid &&
				  RAID_WIDTH_VALID(RAID5, ArrayWidth)) {
				  /* Standard RAID3s are 5 or 9 in width 
				   * This has nothing to do with raid level 3 though...
				   */
				//m_nCheckStandard = TRUE;
			}
			if ((ArrayWidth != 4) && (ArrayWidth != 5) && (ArrayWidth != 9)) {
				m_bValid = FALSE;
			}
			break;

		default:
			ASSERT(0);	
	}
	
//	if ( !m_bValid  && (m_nCheckStandard == FALSE)) {

	if ( !m_bValid ) {
		arrayWidth = RAID_DEFAULT_WIDTH[raidType];
	}
	else {
		arrayWidth = ArrayWidth;
	}

	return m_bValid;
}
/////////////////////////////////////////////////////////////////
// GetDataDisks()
// Determine the number of drives in a unit with data.
//
/////////////////////////////////////////////////////////////////
unsigned int CRaid::GetDataDisks()
{
	unsigned int DataDisks;

	switch (raidType)
	{
		case AGGREGATE:	
		case RAID0:
			DataDisks = arrayWidth;
			break;

		case RAID1:
		case IND_DISK:
			DataDisks = 1;
			break;

		case HI5:
		case RAID5:
		case RAID3:
			DataDisks = arrayWidth - 1;
			break;

        case RAID6:
			DataDisks = arrayWidth - 2;
            break;
            
		case RAID10:
			DataDisks = arrayWidth / 2;
			break;

		default:
			ASSERT(0);	
	}
	return DataDisks;
}

void CRaid::SetElementSize( const unsigned int m_nElementSize)
{
	// Depending on the Raid Type we may allow any element size.

	switch (raidType)
	{
		case AGGREGATE:	
		case RAID0:
		case RAID5:
		case RAID6:
		case RAID10:
		case RAID3:
			// For these, we allow any element size.
			sectorsPerElement = m_nElementSize;
			break;
		
		case RAID1:
		case IND_DISK:
			// For these, we only allow the default
			sectorsPerElement = RAID_DEFAULT_SECTORS_PER_ELEMENT;
			break;

		case HI5:
			// For this type it's always fixed.
			sectorsPerElement = RAID_DEFAULT_HI5_SECTORS_PER_ELEMENT;
			break;

		default:
			ASSERT(0);	
	}

	return;
}


/////////////////////////////////////////////////////////////////
// GetDataStripeSize()
// Determine the number of blocks in the stripe.
//
// This is an easy calculation, just:
// 
//  sectors_per_element * data_disks
/////////////////////////////////////////////////////////////////

ULONG CRaid::GetDataStripeSize( void )
{
	return( sectorsPerElement * GetDataDisks() );
}
// END GetDataStripeSize()

/////////////////////////////////////////////////////////////////
// GetParityStripeSize()
// Determine the number of blocks in the parity stripe.
//
// This is an easy calculation, just:
// 
//  sectorsPerElement * data_disks
/////////////////////////////////////////////////////////////////

ULONG CRaid::GetParityStripeSize( void )
{
	ULONG ParityStripeSize;

	ParityStripeSize = GetElementsPerParityStripe() * sectorsPerElement * GetDataDisks();

	return ParityStripeSize;
}
// END GetParityStripeSize()

/////////////////////////////////////////////////////////////////
// GetElementsPerParityStripe()
// Determine the number of elements in the parity stripe.
//
// This only makes sense for a Raid 5/3, so for all other unit types
// we just return 1.
/////////////////////////////////////////////////////////////////
ULONG CRaid::GetElementsPerParityStripe( void )
{
	unsigned int elements_per_parity_stripe;
    
	ASSERT(arrayWidth != (unsigned int) -1);

	// By default we have one element per parity stripe.
	elements_per_parity_stripe  = 1;

	// For Small elements we have many elements per parity stripe.

	if (sectorsPerElement < defaultSectorsPerElement)
	{
		elements_per_parity_stripe =
			(defaultSectorsPerElement / sectorsPerElement) +
			((defaultSectorsPerElement % sectorsPerElement) ? 1 : 0);
	}

	if ( raidType != RAID5 && raidType != RAID6 && 
          raidType != RAID3 )
	{
		// Just one for all other unit types.
		elements_per_parity_stripe = 1;
	}
	return elements_per_parity_stripe;
}
// END GetElementsPerParityStripe()


void CRaid::calcLba()
{
	switch (raidType)
	{
		case IND_DISK:
			ASSERT(arrayWidth == 1);

			/* Fall through.
			 */
		case AGGREGATE:
		case RAID0:
			if (logicalMode) {
				calcRaid0Logical();
			}
			else {
				
				calcRaid0Physical();
			}
			break;
			
		case RAID1:
		case RAID10:
			if (logicalMode) {
				calcRaid10Logical();
			}
			else {
				
				calcRaid10Physical();
			}
			break;

		case RAID3:
			if (logicalMode) {
				calcRaid5Logical(TRUE);
			}
			else {
				
				calcRaid5Physical(TRUE);
			}
			break;

		case RAID5:
		default:
			if (logicalMode) {
				calcRaid5Logical();
			}
			else {
				
				calcRaid5Physical();
			}
			break;
		
	}
		
}

void CRaid::calcRaid0Logical()
{
	unsigned int sectors_per_stripe;
		
	sectors_per_stripe  = sectorsPerElement * arrayWidth;
	
	dataPos = (unsigned int)((lba / sectorsPerElement) % arrayWidth);
	TRACE("Data disk is %d\n",dataPos);
			
	pba = ((lba / sectors_per_stripe) * sectorsPerElement);
	pba += lba % sectorsPerElement;
	pba += addressOffset[0];
	
	return;
}
void CRaid::calcRaid0Physical()
{
	unsigned int sectors_per_stripe;
		
	sectors_per_stripe  = sectorsPerElement * arrayWidth;
	
	TRACE("Data disk is %d\n",dataPos);
    // calc lba of beginning of this stripe.
	lba = ((pba - addressOffset[0]) / sectorsPerElement) * (arrayWidth * sectorsPerElement);
	// add on the leftover blocks in element
	lba += (pba - addressOffset[0]) % sectorsPerElement;
	// add on blocks to this data position
	lba += (sectorsPerElement * dataPos);
	return;
}

void CRaid::calcRaid10Logical()
{
	/* Validation checks.
	 * Width is always even.
	 * dataPos is primary.
	 * parityPos is secondary.
	 */
	unsigned int sectors_per_stripe;
	ASSERT((arrayWidth % 2) == 0);
	sectors_per_stripe  = sectorsPerElement * (arrayWidth / 2);
	
	dataPos = (unsigned int)(2 * ((lba / sectorsPerElement) % (arrayWidth / 2)));
		
	TRACE("Primary is %d\n",dataPos);
	
	parityPos = dataPos + 1;
	
	TRACE("Primary is %d\n",parityPos);
				
	pba = (lba / sectors_per_stripe) * sectorsPerElement;
	pba += lba % sectorsPerElement;
	pba += addressOffset[0];
	
	ASSERT(dataPos < (arrayWidth - 1));
	ASSERT(parityPos <= (arrayWidth - 1));
	return;
}
void CRaid::calcRaid10Physical()
{
	/* Validation checks.
	 * Width is always even.
	 * dataPos is primary.
	 * parityPos is secondary.
	 */
	unsigned int sectors_per_stripe;
	ASSERT((arrayWidth % 2) == 0);
	sectors_per_stripe  = sectorsPerElement * (arrayWidth / 2);
		
	TRACE("Primary is %d\n",dataPos);
	if (dataPos >= arrayWidth)
	{
		dataPos = arrayWidth - 2;
	}	
	
	// if data position is not really a primary, then
	// make it so.
	if ((dataPos % 2) != 0)
	{
		dataPos -= (dataPos % 2);
	}
	parityPos = dataPos + 1;
	
	TRACE("Primary is %d\n",parityPos);
		
	// calc lba of beginning of this stripe.
	lba = ((pba - addressOffset[0]) / sectorsPerElement) * ((arrayWidth / 2)* sectorsPerElement);
	// add on the leftover blocks in element
	lba += (pba - addressOffset[0]) % sectorsPerElement;
	// add on blocks to this data position
	lba += (sectorsPerElement * (dataPos / 2));
	ASSERT(dataPos < (arrayWidth - 1));
	ASSERT(parityPos <= (arrayWidth - 1));
	return;
}

void CRaid::calcRaid5Logical( const bool fixedParity )
{
	unsigned int elements_per_parity_stripe;
	unsigned int sectors_per_stripe;
	unsigned int sectors_per_parity_stripe;
	ULONGLONG parityStripeOffset;
    
    // We init this to zero since we might not be using it.
    parityPos2 = 0;
    
	ASSERT(arrayWidth != (unsigned int) -1);

	elements_per_parity_stripe = GetElementsPerParityStripe();

	sectors_per_stripe  = sectorsPerElement * GetDataDisks();
	sectors_per_parity_stripe = sectors_per_stripe * elements_per_parity_stripe;
	
	parityStripeOffset = (lba / sectors_per_parity_stripe) * 
		elements_per_parity_stripe * sectorsPerElement;

	if (fixedParity && raidLayout != RAID_RIGHT_SYMMETRIC)
	{
        if ((raidProduct == RAID_LONGBOW_PHASE2) || 
			(raidProduct == RAID_CATAPULT )) {
			/* Parity is the first drive.
			 * Skip first drive when calculating data pos.
			 */
            parityPos = 0;
			dataPos = 1 + (unsigned int)((lba / sectorsPerElement) % (arrayWidth - 1));
        }
        else {
			/* Parity is last drive.
			 * Skip last drive when calculatin data pos.
			 */
            parityPos = (arrayWidth - 1);
			dataPos = (unsigned int)(((lba / sectorsPerElement) % (arrayWidth - 1)));
        }
        TRACE("Parity disk is %d\n", parityPos);
    	
		TRACE("Data disk is %d\n",dataPos);
	}
    else if (raidType == RAID6)
    {
        /* Rt Symmetric */
        ULONGLONG parity_stripe_number;
        parity_stripe_number = (lba / sectors_per_parity_stripe);

        /* Setup parity position.  Since the parity position
         * rotates two at a time, there are half as many possible
         * positions for parity.
         */
        parityPos = (unsigned int) ((parity_stripe_number * 2) % arrayWidth);
        parityPos2 = (unsigned int) (((parity_stripe_number * 2) + 1) % arrayWidth);
        dataLogToPhys[arrayWidth - 2] = parityPos;
        dataLogToPhys[arrayWidth - 1] = parityPos2;
                         
        TRACE("Parity disk is %d\n", parityPos);
    
        /* Get logical data position.
         */
        logDataPos = (unsigned int)((lba / sectorsPerElement) % (arrayWidth - 2));
        
        unsigned int index, physArrayPos;
        
        physArrayPos = parityPos;
        for ( index = 0; index < arrayWidth - 1; index++)
        {
            physArrayPos = (physArrayPos == 0) 
                ? (arrayWidth - 1) 
                : (physArrayPos - 1);
            
            dataLogToPhys[index] = physArrayPos;
            physicalParityStart[index] = addressOffset[physArrayPos] + parityStripeOffset;
        }
        logDataPos = (unsigned int) ((lba / sectorsPerElement) % (arrayWidth - 2));
        dataPos = dataLogToPhys[logDataPos];
        TRACE("Data disk is %d\n",dataPos);
    }
    else
	{
		ULONGLONG parity_stripe_number;
		parity_stripe_number = (lba / sectors_per_parity_stripe);
		if (twoGB) {	
			if (raidLayout == RAID_RIGHT_SYMMETRIC) {
				/* Rt Symmetric */
                /* Setup parity position.
                 */
                if (fixedParity) {
                    parityPos = 0;
                }
                else {
                    parityPos = (unsigned int) (parity_stripe_number % arrayWidth);
                }
                dataLogToPhys[arrayWidth - 1] = parityPos;
	
                TRACE("Parity disk is %d\n", parityPos);
	
                /* Get logical data position.
                 */
                logDataPos = (unsigned int)((lba / sectorsPerElement) % (arrayWidth - 1));
		
                unsigned int i, physArrayPos;

                physArrayPos = parityPos;
                for ( i = 0; i < arrayWidth - 1; i++)
                {
                    physArrayPos = (physArrayPos == 0) 
                        ? (arrayWidth - 1) 
                        : (physArrayPos - 1);

                    dataLogToPhys[i] = physArrayPos;
                    physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
                }
            } /* End Right Symmetric */
            else {
                
				/* Left Symmetric */
                /* Setup parity position.
                 */
                parityPos = (unsigned int)((arrayWidth - 1) - (parity_stripe_number % arrayWidth));
                dataLogToPhys[arrayWidth - 1] = parityPos;
	
                TRACE("Parity disk is %d\n", parityPos);
	
                /* Get logical data position.
                 */
                logDataPos = (unsigned int)((lba / sectorsPerElement) % (arrayWidth - 1));
		
                unsigned int i, physArrayPos;

                physArrayPos = parityPos;
                for ( i = 0; i < arrayWidth - 1; i++)
                {
                    physArrayPos = (physArrayPos + 1) % arrayWidth;

                    dataLogToPhys[i] = physArrayPos;
                    physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
                }
            } /* End Left Symmetric */
        }
		else
		{
	        /* For Right asymmetric, the parity rotates "left" to "right"
		     */
			parityPos = (unsigned int)(parity_stripe_number % arrayWidth);

			dataLogToPhys[arrayWidth - 1] = parityPos;

			TRACE("Parity disk is %d\n", parityPos);
	
			/* Get logical data position.
             */
			logDataPos = (parityPos == 0) ? 1 : 0;
		
			unsigned int i, physArrayPos;

			physArrayPos = 0;

			for ( i = 0; i < arrayWidth - 1; i++)
			{
				if (i == parityPos) {
					// Skip over parity
					physArrayPos++;
				}

				dataLogToPhys[i] = physArrayPos;
				physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
				physArrayPos++;
			}
		}
		logDataPos = (unsigned int) ((lba / sectorsPerElement) % (arrayWidth - 1));
    	dataPos = dataLogToPhys[logDataPos];
		TRACE("Data disk is %d\n",dataPos);
	}
	pba = (lba / sectors_per_stripe) * sectorsPerElement;
	pba += lba % sectorsPerElement;
	pba += addressOffset[0];
	
	return;
}
void CRaid::calcRaid5Physical( const bool fixedParity )
{
	unsigned int elements_per_parity_stripe;
	unsigned int sectors_per_stripe;
	unsigned int sectors_per_parity_stripe;
	ULONGLONG parityStripeOffset;
	unsigned int physArrayPos;
    unsigned int parity_drives = ( raidType == RAID6 ) ? 2 : 1;
    
	ASSERT(arrayWidth != (unsigned int) -1);
	if (dataPos > arrayWidth - 1) { dataPos = arrayWidth - 1; }
	
	elements_per_parity_stripe = GetElementsPerParityStripe();

	sectors_per_stripe  = sectorsPerElement * (arrayWidth - 1);
	sectors_per_parity_stripe = sectors_per_stripe * elements_per_parity_stripe;
	
	parityStripeOffset = (lba / sectors_per_parity_stripe) * 
                                        elements_per_parity_stripe * sectorsPerElement;

	ULONGLONG parity_stripe_number;
	parity_stripe_number = ((pba - addressOffset[0]) / (sectorsPerElement * elements_per_parity_stripe));
		
	if (fixedParity && raidLayout != RAID_RIGHT_SYMMETRIC)
	{
        if ((raidProduct == RAID_LONGBOW_PHASE2) || 
			(raidProduct == RAID_CATAPULT )) {
			/* Parity is the first drive.
			 * Skip first drive when calculating data pos.
			 */
            parityPos = 0;
        }
        else {
			/* Parity is last drive.
			 * Skip last drive when calculating data pos.
			 */
            parityPos = (arrayWidth - 1);
        }
        TRACE("Parity disk is %d\n", parityPos);

        // On Cham 2 the data goes right to left, just like right symm

    	if ( twoGB )
        {           
            dataLogToPhys[arrayWidth - 1] = parityPos;
	
            TRACE("Parity disk is %d\n", parityPos);
			
            unsigned int i;
				
            physArrayPos = parityPos;
            for ( i = 0; i < arrayWidth - 1; i++)
            {
                // Move to the next physical position.
                // Wrap around if we are at position zero
                physArrayPos = (physArrayPos == 0) 
                    ? (arrayWidth - 1) 
                    : (physArrayPos - 1);

                dataLogToPhys[i] = physArrayPos;
                physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
                if (physArrayPos == dataPos)
                {
                    physArrayPos = i;
                    break;
                }
            } 
        }
        else
        {
            // On other systems the data just goes left to right.

			dataLogToPhys[arrayWidth - 1] = parityPos;

			TRACE("Parity disk is %d\n", parityPos);
	
			/* Get logical data position.
             */
			logDataPos = (parityPos == 0) ? 1 : 0;
		
			unsigned int i;

			physArrayPos = 0;

			for ( i = 0; i < arrayWidth - 1; i++)
			{
				if (i == parityPos) {
					// Skip over parity
					physArrayPos++;
				}

				if (physArrayPos == dataPos)
				{
					physArrayPos = i;
					break;
				}
				dataLogToPhys[i] = physArrayPos;
				physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
				physArrayPos++;
			}
        }
        TRACE("Data disk is %d\n",dataPos);
	}
    else if (raidType == RAID6)
    {
        /* RAID 6 is always Right Symmetric.
         */

        /* Setup parity position.  Since the parity position
         * rotates two at a time, there are half as many possible
         * positions for parity.
         */
        parityPos = (unsigned int) ((parity_stripe_number * 2) % arrayWidth);
        parityPos2 = (unsigned int) (((parity_stripe_number * 2) + 1) % arrayWidth);
        dataLogToPhys[arrayWidth - 2] = parityPos;
        dataLogToPhys[arrayWidth - 1] = parityPos2;
	
        TRACE("Parity disk is %d\n", parityPos);
			
        unsigned int index;
				
        physArrayPos = parityPos;
        for ( index = 0; index < arrayWidth - 1; index++)
        {
            // Move to the next physical position.
            // Wrap around if we are at position zero
            physArrayPos = (physArrayPos == 0) 
                ? (arrayWidth - 1) 
                : (physArrayPos - 1);

            dataLogToPhys[index] = physArrayPos;
            physicalParityStart[index] = addressOffset[physArrayPos] + parityStripeOffset;
            if (physArrayPos == dataPos)
            {
                physArrayPos = index;
                break;
            }
        }
    }
    else
	{
		if (twoGB) {	
			if (raidLayout == RAID_RIGHT_SYMMETRIC) {
				/* Rt Symmetric */
                /* Setup parity position.
                 */
                if (fixedParity) {
                    parityPos = 0;
                }
                else {
                    parityPos = (unsigned int)(parity_stripe_number % arrayWidth);
                }
                dataLogToPhys[arrayWidth - 1] = parityPos;
	
                TRACE("Parity disk is %d\n", parityPos);
			
                unsigned int i;
				
                physArrayPos = parityPos;
                for ( i = 0; i < arrayWidth - 1; i++)
                {
                    // Move to the next physical position.
                    // Wrap around if we are at position zero
                    physArrayPos = (physArrayPos == 0) 
                        ? (arrayWidth - 1) 
                        : (physArrayPos - 1);

                    dataLogToPhys[i] = physArrayPos;
                    physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
					if (physArrayPos == dataPos)
					{
						physArrayPos = i;
						break;
					}
                }
            } /* End Right Symmetric */
            else {
                
				/* Left Symmetric */
                /* Setup parity position.
                 */
                parityPos = (unsigned int)((arrayWidth - 1) - (parity_stripe_number % arrayWidth));
                dataLogToPhys[arrayWidth - 1] = parityPos;
	
                TRACE("Parity disk is %d\n", parityPos);
		
                unsigned int i;

                physArrayPos = parityPos;
                for ( i = 0; i < arrayWidth - 1; i++)
                {
                    physArrayPos = (physArrayPos + 1) % arrayWidth;


					if (physArrayPos == dataPos)
					{
						physArrayPos = i;
						break;
					}
                    dataLogToPhys[i] = physArrayPos;
                    physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
                }
            } /* End Left Symmetric */
        }
		else
		{
	        /* For Right asymmetric, the parity rotates "left" to "right"
		     */
			parityPos = (unsigned int)(parity_stripe_number % arrayWidth);

			dataLogToPhys[arrayWidth - 1] = parityPos;

			TRACE("Parity disk is %d\n", parityPos);
	
			/* Get logical data position.
             */
			logDataPos = (parityPos == 0) ? 1 : 0;
		
			unsigned int i;

			physArrayPos = 0;

			for ( i = 0; i < arrayWidth - 1; i++)
			{
				if (i == parityPos) {
					// Skip over parity
					physArrayPos++;
				}

				if (physArrayPos == dataPos)
				{
					physArrayPos = i;
					break;
				}
				dataLogToPhys[i] = physArrayPos;
				physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
				physArrayPos++;
			}
		}
	}
	
	if (parityPos == dataPos || (raidType == RAID6 && parityPos2 == dataPos) )
	{
		dataPos = dataLogToPhys[0];
		physArrayPos = 0;
	}
	ASSERT(physArrayPos < (arrayWidth - 1));
	// calc lba of beginning of this stripe.
	lba = ((pba - addressOffset[0]) / sectorsPerElement) * ((arrayWidth - parity_drives) * sectorsPerElement);
	// add on the leftover blocks in element
	lba += (pba - addressOffset[0]) % sectorsPerElement;
	// add on blocks to this data position
	lba += (sectorsPerElement * physArrayPos);
	return;
}
void CRaid::calcHi5( const bool fixedParity )
{
	unsigned int elements_per_parity_stripe;
	unsigned int sectors_per_stripe;
	unsigned int sectors_per_parity_stripe;
	unsigned int parityStripeOffset;
	unsigned int element_num;

#define MAX_SEGMENT_NUM 32767
#define SEGMENT_DB_SIZE 2
#define MIN_LOG_SECTORS_PER_ELEMENT 3
		
    elements_per_parity_stripe = GetElementsPerParityStripe();

	sectors_per_stripe  = sectorsPerElement * (arrayWidth - 1);
	sectors_per_parity_stripe = sectors_per_stripe * elements_per_parity_stripe;
	
	ASSERT(lba <= MAX_SEGMENT_NUM);
	
	
	ASSERT(segmentOffset <= (sectorsPerElement * (arrayWidth - 1)));
	parityStripeOffset = (unsigned int)((lba / sectors_per_parity_stripe) * 
		elements_per_parity_stripe * sectorsPerElement);
	
	element_num = segmentOffset / (sectorsPerElement - 2);

    ULONGLONG segLba = lba;
    
	segLba = (lba * (arrayWidth - 1) * sectorsPerElement);
	segLba += (element_num * sectorsPerElement);

	segDbLba = segLba;
	segDbLba += 2;
	segDbLba += (segmentOffset % (sectorsPerElement - 2));
	
	if (!calcSegDb)
	{
		segLba = segDbLba;
	}

	if (twoGB) {	

        /* Setup parity position for Left Symmetric, 
		 * parity position rotates "right" to "left"
         */
		parityPos = (unsigned int) 
            ((arrayWidth - 1) - ((segLba / sectors_per_parity_stripe) % arrayWidth));

		dataLogToPhys[arrayWidth - 1] = parityPos;

		TRACE("Parity disk is %d\n", parityPos);
	
        /* Get logical data position.
         */
        logDataPos = (unsigned int)((segLba / sectorsPerElement) % (arrayWidth - 1));
		
        unsigned int i, physArrayPos;

        physArrayPos = parityPos;
        for ( i = 0; i < arrayWidth - 1; i++)
        {
            physArrayPos = (physArrayPos + 1) % arrayWidth;

            dataLogToPhys[i] = physArrayPos;
            physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
        }
    }
    else
    {
        /* For Right asymmetric, the parity rotates "left" to "right"
         */
        parityPos = (unsigned int)((segLba / sectors_per_parity_stripe) % arrayWidth);

        dataLogToPhys[arrayWidth - 1] = parityPos;

        TRACE("Parity disk is %d\n", parityPos);
	
        /* Get logical data position.
         */
        logDataPos = (parityPos == 0) ? 1 : 0;
		
        unsigned int i, physArrayPos;

        physArrayPos = 0;

        for ( i = 0; i < arrayWidth - 1; i++)
        {
            if (i == parityPos) {
                // Skip over parity
                physArrayPos++;
            }

            dataLogToPhys[i] = physArrayPos;
            physicalParityStart[i] = addressOffset[physArrayPos] + parityStripeOffset;
            physArrayPos++;
        }
    }
    logDataPos = (unsigned int) ((segLba / sectorsPerElement) % (arrayWidth - 1));
    dataPos = dataLogToPhys[logDataPos];
    TRACE("Data disk is %d\n",dataPos);

	pba = ((segLba/ sectors_per_stripe) * sectorsPerElement) +
        (segLba % sectorsPerElement) + (addressOffset[0]);
	return;
}

const CRaid &CRaid::operator=(const RAID_CALC_STRUCT &CalcStruct)
{
	// First make sure that the Layout in the RaidObject is setup
	// according to the PreCham2 field in the CalcStruct

	if (CalcStruct.platform != RAID_CALC_LONGBOW) {
		raidLayout = RAID_RIGHT_SYMMETRIC;
	}
	else {
		raidLayout = RAID_RIGHT_ASYMMETRIC;
	}

    if(CalcStruct.elementsPerParityStripe) {
		defaultSectorsPerElement = CalcStruct.elementsPerParityStripe
			                     * CalcStruct.sectorsPerElement;
	}
	else if ( raidType == RAID5 && CalcStruct.platform == RAID_CALC_PIRANHA) {
        // Piranha has a very large parity element, but only for R5.
        // Treat all other Raid Types normally (like CX).
        defaultSectorsPerElement = RAID_DEFAULT_ELEMENTS_PER_PARITY_STRIPE_PIRANHA;
    }
	else {
        defaultSectorsPerElement = 128;
    }

	// For the next set of fields, copy them directly from the
	// CalcStruct. 

	raidType = (RAID_TYPE) CalcStruct.raidType;
	
	sectorsPerElement = CalcStruct.sectorsPerElement;
	
	addressOffset[0] = CalcStruct.AddressOffset;
	SetArrayWidth(CalcStruct.ArrayWidth);
	
	// Calculate the Bind Offset given an AlignmentLba
	
	if ( CalcStruct.AlignmentLba == 0 )
	{
		bindOffset = 0;
	}
	else
	{
		ULONG PartialStripe = CalcStruct.AlignmentLba % GetDataStripeSize();
		bindOffset = GetDataStripeSize() - PartialStripe;
	}
	// Since there may be a bind offset, also include the bind
	// offset in our calculations.
	
	lba = CalcStruct.Lba + bindOffset;
	
	pba = CalcStruct.Pba;
	segmentOffset = CalcStruct.Hi5SegmentOffset;
	twoGB = CalcStruct.platform != RAID_CALC_LONGBOW;
	calcSegDb = CalcStruct.Hi5SegmentDb;
	dataPos = CalcStruct.dataPos;
	parityPos = CalcStruct.parityPos;
	
	return *this;
}
