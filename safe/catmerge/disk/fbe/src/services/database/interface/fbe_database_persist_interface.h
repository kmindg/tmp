#ifndef FBE_DATABASE_PERSIST_INTERFACE_H
#define FBE_DATABASE_PERSIST_INTERFACE_H

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database_private.h"
#include "fbe/fbe_persist_interface.h"

fbe_status_t fbe_database_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info);

fbe_status_t fbe_database_persist_set_lun(fbe_object_id_t lu_object_id,
										  fbe_persist_completion_function_t completion_function,
										  fbe_persist_completion_context_t completion_context);

fbe_status_t fbe_database_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle);
fbe_status_t fbe_datbase_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle);
fbe_status_t fbe_database_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle,
													 fbe_persist_completion_function_t completion_function,
													 fbe_persist_completion_context_t completion_context);
fbe_status_t fbe_database_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle,
													 fbe_persist_completion_function_t completion_function,
													 fbe_persist_completion_context_t completion_context);
fbe_status_t fbe_database_persist_read_entry(fbe_persist_entry_id_t entry_id,
                                             fbe_persist_sector_type_t persist_sector_type,
                                             fbe_u8_t *read_buffer,
                                             fbe_u32_t data_length,/*MUST BE size of FBE_PERSIST_DATA_BYTES_PER_ENTRY*/
                                             fbe_persist_single_read_completion_function_t completion_function,
                                             fbe_persist_completion_context_t completion_context);
fbe_status_t fbe_database_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
											  fbe_persist_sector_type_t target_sector,
											  fbe_u8_t *data,
											  fbe_u32_t data_length,
											  fbe_persist_entry_id_t *entry_id);
fbe_status_t fbe_database_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
											   fbe_u8_t *data,
											   fbe_u32_t data_length,
											   fbe_persist_entry_id_t entry_id);
fbe_status_t fbe_database_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
											   fbe_persist_entry_id_t entry_id);

fbe_status_t fbe_database_persist_read_sector(fbe_persist_sector_type_t target_sector,
											  fbe_u8_t *read_buffer,/*must be of size FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE*/
											  fbe_u32_t data_length,
											  fbe_persist_entry_id_t start_entry_id,
											  fbe_persist_read_multiple_entries_completion_function_t completion_function,
											  fbe_persist_completion_context_t completion_context);
fbe_status_t fbe_database_persist_get_required_lun_size(fbe_lba_t *required_lun_size);

fbe_status_t fbe_database_clear_user_modified_wwn_seed_flag(void);
fbe_status_t fbe_database_get_user_modified_wwn_seed_flag(fbe_bool_t *user_modified_wwn_seed_flag);

fbe_status_t fbe_database_get_board_resume_prom_wwnseed(fbe_u32_t *wwn_seed);
fbe_status_t fbe_database_set_board_resume_prom_wwnseed(fbe_u32_t wwn_seed);
fbe_status_t fbe_database_persist_unset_lun(void);

fbe_status_t fbe_database_get_board_resume_prom_wwnseed_by_pp(fbe_u32_t *wwn_seed);
fbe_status_t fbe_database_set_board_resume_prom_wwnseed_by_pp(fbe_u32_t wwn_seed);

fbe_status_t fbe_database_get_user_modified_wwn_seed_flag_with_retry(fbe_bool_t *user_modified_wwn_seed_flag);
fbe_status_t fbe_database_get_board_resume_prom_wwnseed_with_retry(fbe_u32_t *wwn_seed);
fbe_status_t fbe_database_set_board_resume_prom_wwnseed_with_retry(fbe_u32_t wwn_seed);
fbe_status_t fbe_database_clear_user_modified_wwn_seed_flag_with_retry(void);

fbe_status_t fbe_database_persist_validate_entry(fbe_persist_transaction_handle_t transaction_handle,
                                                 fbe_persist_entry_id_t entry_id);
fbe_status_t fbe_database_populate_transaction_entry_ids_from_persist(database_config_table_type_t table_type,
                                                                      database_table_header_t *transaction_entry_header_p);

#endif /*  FBE_DATABASE_PERSIST_INTERFACE_H */

