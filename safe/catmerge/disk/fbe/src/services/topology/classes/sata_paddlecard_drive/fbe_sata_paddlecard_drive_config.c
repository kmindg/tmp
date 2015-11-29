/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_paddlecard_drive_config.c
 ***************************************************************************
 *
 * @brief
 *  This file defines the mode sense/select data structures.
 * 
 * HISTORY
 *   12/30/2010:  Created. Wayne Garrett
 *
 ***************************************************************************/

#include "sata_paddlecard_drive_config.h"


fbe_vendor_page_t fbe_def_sata_paddlecard_vendor_tbl[] =
{
/*
 *  Default Drive Table for SATA paddlecards
 * 
 *  Note, the table is only used for situation where the default mode pages
 *  are not correct.  The mode pages should be configured correctly by
 *  SBMT before hand, therefore initially this table should be empty.  But the
 *  implementation for doing the mode sense depends on the table having
 *  at least one entry. 
 * 
 *  mask indicates which bits are okay to clear at destination.  any bit on is
 * cleared at destination.
 *
 *     page            offset   mask   value
 */
        
    {FBE_MS_DISCO_PG02, 2, 0xFF, 0xFF},   // added to ensure table has at least one entry.

    {FBE_MS_FIBRE_PG19, 4, 0xFF, 0x07},  /* bytes 4,5 is IT Nexus timeout in msec. set to 2 sec*/
    {FBE_MS_FIBRE_PG19, 5, 0xFF, 0xd0},
    {FBE_MS_FIBRE_PG19, 6, 0xFF, 0x11},  /* bytes 6,7 is IRT timeout in msec. set to 4.5 sec */
    {FBE_MS_FIBRE_PG19, 7, 0xFF, 0x94},
};

const fbe_u16_t fbe_def_sata_paddlecard_vendor_tbl_entries = sizeof(fbe_def_sata_paddlecard_vendor_tbl)/sizeof(fbe_def_sata_paddlecard_vendor_tbl[0]);

