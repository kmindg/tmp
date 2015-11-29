#ifndef FBE_CMS_CDR_INTERFACE_H
#define FBE_CMS_CDR_INTERFACE_H

/***************************************************************************/
/** @file fbe_cms_cdr_interface.h
***************************************************************************
*
* @brief
*  This file contains definitions for the CDR and other data needed for managing the SM
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_cmi.h"


#pragma pack(1)
typedef struct fbe_cms_cdr_image_valid_info_s{
	fbe_u64_t	generation_number;
}fbe_cms_cdr_image_valid_info_t;
#pragma pack()

typedef enum image_valid_type_e{
	SPA_VALID_SECTOR,
	SPB_VALID_SECTOR,
	VAULT_VALID_SECTOR
}image_valid_type_t;

fbe_status_t fbe_cms_sm_read_image_valid_data(fbe_cmi_sp_id_t sp_id,
											  fbe_bool_t *this_sp_valid,
											  fbe_bool_t *peer_valid,
											  fbe_bool_t *vault_valid);

fbe_status_t fbe_cms_sm_set_image_invalid(image_valid_type_t sector);

fbe_status_t fbe_cms_sm_set_image_valid(image_valid_type_t sector,
										fbe_u64_t new_generation_number);

#endif /* FBE_CMS_CDR_INTERFACE_H */
