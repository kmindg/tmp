/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
*                               fbe_database_homewrecker_fru_descriptor.h
***************************************************************************
*
* DESCRIPTION: Interfaces of processing homewrecker fru descriptors
*
* FUNCTIONS:
*
* NOTES:
*
* HISTORY:
*    11/30/2011  zphu
*
**************************************************************************/

#ifndef FBE_DATABASE_HOMEWRECKER_FRU_DESCRIPTOR_H
#define FBE_DATABASE_HOMEWRECKER_FRU_DESCRIPTOR_H

#include "fbe/fbe_xor_api.h"
#include "fbe_fru_descriptor_structure.h"


typedef struct fbe_homewrecker_get_raw_mirror_info_s{
    fbe_homewrecker_get_raw_mirror_status_t status;     /*the status from read content*/
    fbe_u64_t seq_number;
    fbe_u32_t error_disk_bitmap;
    fbe_status_t call_status;       /*the return status after calling read raw mirror*/
}fbe_homewrecker_get_raw_mirror_info_t;

/* The following functions are related to generation and modification of homewrecker fru descriptors
* *******************************************************************************
*/

/* invalidate the in-memory fru descriptors and the private space regions on the system drives.
* return the count of successfully invalidated fru descriptors
* if input parameter success_flag_array is not NULL, it would record which fru_descriptors are successfully invalidated
*/
fbe_homewrecker_get_raw_mirror_status_t
database_homewrecker_raw_mirror_error(fbe_raid_verify_raw_mirror_errors_t* raw_mirror_error_report);

void database_initialize_fru_descriptor_sequence_number(void);

fbe_status_t database_set_fru_descriptor_sequence_number(fbe_u32_t seq_num);

fbe_status_t database_get_next_fru_descriptor_sequence_number(fbe_u32_t* seq_num);

#endif


