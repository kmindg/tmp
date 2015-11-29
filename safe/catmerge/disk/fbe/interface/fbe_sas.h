#ifndef FBE_SAS_H
#define FBE_SAS_H

#include "fbe/fbe_types.h"

/*typedef fbe_u64_t fbe_miniport_device_id_t; <-- move to fbe_types.h*/

typedef enum fbe_sas_element_type_e {
	FBE_SAS_DEVICE_TYPE_UKNOWN,
	FBE_SAS_DEVICE_TYPE_SSP,
	FBE_SAS_DEVICE_TYPE_STP,
	FBE_SAS_DEVICE_TYPE_EXPANDER,
	FBE_SAS_DEVICE_TYPE_VIRTUAL,

	FBE_SAS_DEVICE_TYPE_EXPANSION_PORT, /* I think it should not be here */

	FBE_SAS_DEVICE_TYPE_LAST
}fbe_sas_element_type_t;

typedef struct fbe_sas_element_s {
	fbe_sas_element_type_t	element_type;
	fbe_sas_address_t		sas_address;
	fbe_u8_t				cpd_device_id; /*! temporary support for LSI miniport */

	fbe_u8_t				present; /*! If set to one indicates that device physically connected */
	fbe_u8_t				change_count;
	fbe_u8_t				swap;	/*! If set to one - device was removed and the same or other device was reinserted */

	/* New staff */
	fbe_sas_address_t		parent_sas_address;
	fbe_u8_t				parent_cpd_device_id; /*! temporary support for LSI miniport */
	fbe_u8_t				phy_number;
	fbe_u8_t				enclosure_chain_depth;
	fbe_u8_t				reserved2;

}fbe_sas_element_t;

#endif /* FBE_SAS_H */