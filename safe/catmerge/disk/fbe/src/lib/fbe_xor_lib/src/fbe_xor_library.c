/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_library.c
 ***************************************************************************
 *
 * @brief
 *  This file contains .
 *
 * @version
 *   3/12/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_xor_private.h"
#include "xorlib_api.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * fbe_xor_library_init()
 ****************************************************************
 * @brief
 *  Initialize the xor library.
 *  This initializes all of the global structures that
 *  the xor library maintains.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_library_init(void)
{
    fbe_status_t status;

    /* We will run asm routines on hardware and 64-bit sim environment.
     */
    xorlib_select_asm();

    status = fbe_xor_init_raid6_globals();
    if (status != FBE_STATUS_OK) { return status; }

    return status;
}
/******************************************
 * end fbe_xor_library_init()
 ******************************************/

/*!**************************************************************
 * fbe_xor_library_destroy()
 ****************************************************************
 * @brief
 *  Initialize the xor library.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_library_destroy(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    return status;
}
/******************************************
 * end fbe_xor_library_destroy()
 ******************************************/

/*************************
 * end file fbe_xor_library.c
 *************************/
