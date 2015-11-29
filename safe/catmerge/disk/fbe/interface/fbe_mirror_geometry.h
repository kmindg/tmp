#ifndef _FBE_MIRROR_GEOMETRY_H_
#define _FBE_MIRROR_GEOMETRY_H_
/*******************************************************************
 * Copyright (C) EMC Corporation 2000, 2006-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*!***************************************************************************
 *          fbe_mirror_geometry.h
 *****************************************************************************
 *
 * @brief   The mirror geometry structure contains complete and sufficient
 *          information that is required by the mirror read optimization algorithm.
 *
 * @author
 *  04/05/2011  Swati Fursule
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_mirror.h"


/*!*******************************************************************
 * @def FBE_MIRROR_GEOMETRY_MAX_WIDTH 
 *********************************************************************
 *  @brief this is the max number of mirror components currently supported 
 *********************************************************************/
/*#define FBE_MIRROR_GEOMETRY_MAX_WIDTH   FBE_MIRROR_MAX_WIDTH
 */


/*!*******************************************************************
 * @struct fbe_mirror_optimization_t
 *********************************************************************
 * @brief
 *  This structure contains the mirror read optimization counters
 *
 *********************************************************************/
typedef struct fbe_mirror_optimization_s
{
    fbe_spinlock_t spinlock;

    /*! This is read io count used to optimize mirror read. 
     */
    fbe_u32_t num_reads_outstanding[FBE_MIRROR_MAX_WIDTH];

    /*! This is the LBA position where we left the disk head.
     */
    fbe_lba_t current_lba[FBE_MIRROR_MAX_WIDTH];

}fbe_mirror_optimization_t;

static __forceinline
void fbe_mirror_optimization_init(fbe_mirror_optimization_t *mirror_opt_p)
{
    fbe_u32_t i;
    fbe_spinlock_init(&mirror_opt_p->spinlock);
    fbe_spinlock_lock(&mirror_opt_p->spinlock);
    for(i = 0; i < FBE_MIRROR_MAX_WIDTH; i++)
    {
        mirror_opt_p->num_reads_outstanding[i] = 0;
        mirror_opt_p->current_lba[i] = 0;
    }
    fbe_spinlock_unlock(&mirror_opt_p->spinlock);
}

/*****************************************
 * end of fbe_mirror_geometry.h
 *****************************************/

#endif // _FBE_MIRROR_GEOMETRY_H_
