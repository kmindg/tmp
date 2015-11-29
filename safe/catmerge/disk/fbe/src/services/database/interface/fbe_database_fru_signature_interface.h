/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
*                               fbe_database_fru_signature.h
***************************************************************************
*
* DESCRIPTION: Interfaces of processing fru signature in database
*
* FUNCTIONS:
*
* NOTES:
*
* HISTORY:
*    7/18/2012  Gao Jian
*
**************************************************************************/

#ifndef FBE_DATABASE_FRU_SIGNATURE_PRIVATE_H
#define FBE_DATABASE_FRU_SIGNATURE_PRIVATE_H

#include "fbe_database_private.h"
#include "fbe_fru_signature.h"

typedef enum
{
    FBE_DATABASE_UNINITIALIZED_FRU_SIGNATURE_IO_OPERATION_STATUS,
    FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK,
    FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_UNINITIALIZED,
    FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE
}fbe_database_fru_signature_IO_operation_status_t;

typedef struct fbe_homewrecker_signature_report_s{
    fbe_database_fru_signature_IO_operation_status_t io_status; /*the status from read operation*/
    fbe_status_t call_status;   /*the return status after calling read signature*/
}fbe_homewrecker_signature_report_t;

typedef enum
{
    FBE_DATABASE_UNINITIALIZED_FRU_SIGNATURE_VALIDATION_STATUS,
    FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_OK, 
    FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_ERROR, 
    /* The fru signature version is less or larger than the version defined in fbe_fru_signature.h */
    FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_VERSION_ERROR,
    FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_AND_VERSION_ERROR
}fbe_database_fru_signature_validation_status_t;


/* The following functions are related to read/write and modify fru signature in Database
* *******************************************************************************
*/

/* write fru signature to disk */
fbe_status_t fbe_database_write_fru_signature_to_disk
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
fbe_fru_signature_t* in_fru_signature, fbe_database_fru_signature_IO_operation_status_t* operation_status);

/* clear fru signature */
fbe_status_t fbe_database_clear_disk_fru_signature
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, fbe_database_fru_signature_IO_operation_status_t* operation_status);

/* read fru signature */
fbe_status_t fbe_database_read_fru_signature_from_disk
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot,
fbe_fru_signature_t* out_fru_signature, fbe_database_fru_signature_IO_operation_status_t* operation_status);

/* validate fru signature */
fbe_status_t fbe_database_validate_fru_signature
(fbe_fru_signature_t* fru_sig, fbe_database_fru_signature_validation_status_t* signature_status);

/* write fru signature to all system disks */
fbe_status_t fbe_database_stamp_fru_signature_to_system_drives(void);

fbe_status_t  
fbe_database_get_homewrecker_fru_signature(fbe_fru_signature_t* fru_sig, 
                                                            fbe_homewrecker_signature_report_t *info,
                                                            int bus, int enclosure, int slot);


fbe_status_t 
fbe_database_set_homewrecker_fru_signature(fbe_fru_signature_t* fru_sig, 
                                                            fbe_homewrecker_signature_report_t *info,
                                                            int bus, int enclosure, int slot);

#endif



