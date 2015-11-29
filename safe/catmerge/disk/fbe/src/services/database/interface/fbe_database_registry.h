#ifndef FBE_DATABASE_REGISTRY_H
#define FBE_DATABASE_REGISTRY_H 
/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
fbe_status_t fbe_database_registry_get_ndu_flag(fbe_bool_t *ndu_flag);
fbe_status_t fbe_database_registry_set_ndu_flag(fbe_bool_t *ndu_flag);

fbe_status_t fbe_database_registry_get_local_expected_memory_value(fbe_u32_t   *lemv);
fbe_status_t fbe_database_registry_set_shared_expected_memory_value(fbe_u32_t   semv);
fbe_status_t fbe_database_registry_get_addl_supported_drive_types(fbe_database_additional_drive_types_supported_t *supported);
fbe_status_t fbe_database_registry_addl_set_supported_drive_types(fbe_database_additional_drive_types_supported_t supported);

fbe_status_t fbe_database_registry_set_user_modified_wwn_seed_info(fbe_bool_t user_modified_wwn_seed_flag);
fbe_status_t fbe_database_registry_get_user_modified_wwn_seed_info(fbe_bool_t *user_modified_wwn_seed_flag);

fbe_status_t fbe_database_registry_get_c4_mirror_reinit_flag(fbe_bool_t *reinit);
fbe_status_t fbe_database_registry_set_c4_mirror_reinit_flag(fbe_bool_t reinit);
#endif 
