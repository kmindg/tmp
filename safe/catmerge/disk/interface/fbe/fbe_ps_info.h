/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_PS_INFO_H
#define FBE_PS_INFO_H

/*!**************************************************************************
 * @file fbe_ps_info.h
 ***************************************************************************
 *
 * @brief
 *  This file PS defines & data structures.
 * 
 * @ingroup 
 * 
 * @revision
 *   04/15/2010:  Created. Joe Perry
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_pe_types.h"


#define FBE_ESP_PS_MAX_COUNT_PER_ENCL       4            // max per xPE, DPE, DAE


/*!****************************************************************************
 *    
 * @struct fbe_ps_info_s
 *  
 * @brief 
 *   This is the definition of the PS info. This structure
 *   stores information about the PS
 ******************************************************************************/
typedef struct fbe_ps_info_s {
    // data available from Board & Enclosure Objects (don't duplicate, try to remove)
    fbe_u32_t                           psFfid;
    fbe_bool_t                          psInserted;
    fbe_bool_t                          psFault;
    fbe_bool_t                          psAcFail;
    fbe_u16_t                           psInputPower;
    fbe_u16_t                           psMarginTestResults;    
    SP_ID                               associatedSp;
    SPS_ID                              associatedSps;
}fbe_ps_info_t;



#endif /* FBE_PS_INFO_H */

/*******************************
 * end fbe_ps_info.h
 *******************************/
