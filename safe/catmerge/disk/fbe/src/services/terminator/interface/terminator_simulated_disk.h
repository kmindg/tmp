#ifndef TERMINATOR_SIMULATED_DISK_H
#define TERMINATOR_SIMULATED_DISK_H

fbe_status_t terminator_simulated_disk_memory_write(terminator_drive_t * drive,
												    fbe_lba_t lba, 
												    fbe_block_count_t block_count, 
												    fbe_block_size_t block_size, 
												    fbe_u8_t * data_buffer,
												   void * context);

fbe_status_t terminator_simulated_disk_memory_read(terminator_drive_t * drive,
												   fbe_lba_t lba, 
												   fbe_block_count_t block_count, 
												   fbe_block_size_t block_size, 
												   fbe_u8_t * data_buffer,
												   void * context);

fbe_status_t terminator_simulated_disk_memory_write_zero_pattern(terminator_drive_t * drive,
        														 fbe_lba_t lba,	
        														 fbe_block_count_t block_count,	
        														 fbe_block_size_t block_size, 
        														 fbe_u8_t * data_buffer,
																void * context);

fbe_status_t terminator_simulated_disk_local_file_write(terminator_drive_t * drive_handle,
                                                        fbe_lba_t lba, 
                                                        fbe_block_count_t block_count, 
                                                        fbe_block_size_t block_size, 
                                                        fbe_u8_t * data_buffer,
												   void * context);

fbe_status_t terminator_simulated_disk_local_file_read(terminator_drive_t * drive_handle,
                                                       fbe_lba_t lba, 
                                                       fbe_block_count_t block_count, 
                                                       fbe_block_size_t block_size, 
                                                       fbe_u8_t * data_buffer,
												   void * context);

fbe_status_t terminator_simulated_disk_local_file_write_zero_pattern(terminator_drive_t * drive_handle,
    														fbe_lba_t lba,	
    														fbe_block_count_t block_count,	
    														fbe_block_size_t block_size, 
    														fbe_u8_t * data_buffer,
															void * context);

fbe_status_t terminator_simulated_disk_local_file_init(void);
fbe_status_t terminator_simulated_disk_local_file_destroy(void);

void terminator_simulated_disk_remote_file_simple_init(csx_module_context_t context,
                                                       const char          *server_address,
                                                       csx_nuint_t          server_port,
                                                       csx_nuint_t          array_id);
void terminator_simulated_disk_remote_file_simple_destroy(void);

fbe_status_t terminator_simulated_disk_remote_file_simple_create(fbe_u32_t port, 
                                                                 fbe_u32_t enclosure, 
                                                                 fbe_u32_t slot, 
                                                                 fbe_block_size_t block_size, 
                                                                 fbe_lba_t max_lba);
fbe_status_t terminator_simulated_disk_remote_file_simple_write(terminator_drive_t * drive_handle,
                                                                fbe_lba_t lba, 
                                                                fbe_block_count_t block_count, 
                                                                fbe_block_size_t block_size, 
                                                                fbe_u8_t* data_buffer,
                                                                void *context);

fbe_status_t terminator_simulated_disk_remote_file_simple_write_zero_pattern(
                                                                terminator_drive_t * drive_handle,
                                                                fbe_lba_t lba, 
                                                                fbe_block_count_t block_count, 
                                                                fbe_block_size_t block_size, 
                                                                fbe_u8_t* data_buffer,
                                                                void *context);

fbe_status_t terminator_simulated_disk_remote_file_simple_read(terminator_drive_t * drive_handle,
                                                               fbe_lba_t lba, 
                                                               fbe_block_count_t block_count, 
                                                               fbe_block_size_t block_size, 
                                                               fbe_u8_t * data_buffer,
                                                                void *context);


void terminator_simulated_disk_memory_free_record(terminator_drive_t *drive, 
                                                  journal_record_t **record_pp);

fbe_status_t terminator_simulated_disk_encrypt_data(fbe_u8_t *data_buffer,
                                           fbe_u32_t buffer_size,
                                           fbe_u8_t *dek, 
                                           fbe_u32_t dek_size);
fbe_status_t terminator_simulated_disk_decrypt_data(fbe_u8_t *data_buffer,
                                           fbe_u32_t buffer_size,
                                           fbe_u8_t *dek, 
                                           fbe_u32_t dek_size);
fbe_status_t terminator_simulated_disk_crypto_process_data(fbe_u8_t *data_buffer,
                                                  fbe_u32_t buffer_size,
                                                  fbe_u8_t *dek, 
                                                  fbe_u32_t dek_size);

#endif /*TERMINATOR_SIMULATED_DISK_H*/

