/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef TERMINATOR_DRIVE_H
#define TERMINATOR_DRIVE_H

#include "terminator_base.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_types.h"
#include "fbe_sas.h"
#include "fbe_simulated_drive.h"
#include "fbe/fbe_api_terminator_drive_interface.h"

#define INVALID_SLOT_NUMBER 0x3C
#define ZERO_PATTERN_SIZE 520
#define ZERO_PATTERN_CHECKSUM 0x7fff5eed
#define FBE_TERMINATOR_DRIVE_HANDLE_INVALID 0xFFFFFFFFFFFFFFFF
#define FBE_TERMINATOR_DRIVE_IDENTITY_SIZE 20
#define TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_BUS    (0)
#define TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_ENCL   (0)
#define TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_SLOT   (3)
#define TERMINATOR_SIMULATED_DISK_DEFAULT_BIND_OFFSET       (0x10000)
#define TERMINATOR_SIMULATED_DISK_520_BPS_OPTIMAL_BLOCKS    (0x40)
#define TERMINATOR_SIMULATED_DISK_512_BPS_OPTIMAL_BLOCKS    (0x41)

typedef struct terminator_sas_drive_info_s{
    fbe_sas_drive_type_t drive_type;
    fbe_cpd_shim_callback_login_t login_data;
	fbe_u8_t drive_serial_number[TERMINATOR_SATA_SERIAL_NUMBER_SIZE];
    fbe_u32_t backend_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;
	terminator_sas_drive_state_t drive_state;
    fbe_u32_t                   miniport_sas_device_table_index;
    fbe_u8_t product_id[TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE];
    fbe_u8_t inquiry[TERMINATOR_SCSI_INQUIRY_DATA_SIZE];
    fbe_u8_t vpd_inquiry_f3[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xF3_SIZE];
    fbe_u8_t vpd_inquiry_b2[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xB2_SIZE];
    fbe_u8_t vpd_inquiry_c0[TERMINATOR_SCSI_VPD_INQUIRY_PAGE_0xC0_SIZE];    
    fbe_u8_t mode_page[TERMINATOR_SCSI_MODE_PAGE_10_BYTE_SIZE];  /* only for mode sense/select 10*/
    fbe_u8_t mode_page_19[TERMINATOR_SCSI_MODE_PAGE_0x19_SIZE];  /* only for mode sense/select 10*/
    fbe_u8_t log_page_31[TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE];  /* only for mode sense/select 10*/
}terminator_sas_drive_info_t;

typedef struct terminator_sata_drive_info_s{
    fbe_sata_drive_type_t drive_type;
    fbe_cpd_shim_callback_login_t login_data;
	fbe_u8_t drive_serial_number[TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];
    fbe_u32_t backend_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;
    /* fields used by new API */
    fbe_u32_t                   miniport_sas_device_table_index;
    fbe_u8_t product_id[TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE];
}terminator_sata_drive_info_t;

typedef enum terminator_journal_record_type_e{
    TERMINATOR_JOURNAL_RECORD_ZERO, /*0*/
    TERMINATOR_JOURNAL_RECORD_WRITE, /*1*/
    TERMINATOR_JOURNAL_RECORD_LAST
}terminator_journal_record_type_t;

typedef enum terminator_journal_record_flags_e {
    TERMINATOR_JOURNAL_RECORD_FLAG_UNMAPPED = 0x00000001,
}terminator_journal_record_flags_t;

typedef struct terminator_journal_record_s {
    fbe_queue_element_t queue_element;
    terminator_journal_record_type_t record_type;
    fbe_block_size_t	block_size;
    fbe_lba_t		lba;
    fbe_block_count_t	block_count;
    fbe_u8_t		* data_ptr;
    fbe_u32_t		data_size; /* Used if data is compressed */
    fbe_bool_t		is_compressed; /* FBE_TRUE if data is compressed */
    terminator_journal_record_flags_t flags;
}journal_record_t;

typedef struct terminator_drive_s{
    base_component_t base;
	fbe_u32_t reset_count;
    fbe_drive_type_t drive_protocol;

	fbe_u8_t drive_identity[FBE_TERMINATOR_DRIVE_IDENTITY_SIZE];
	fbe_u64_t drive_handle;

	/* Peter Puhov */
	/* The drive will use memory for storage */
	/* This lock will protect acsess to the drive */
	fbe_spinlock_t		journal_lock;
	fbe_queue_head_t	journal_queue_head;
    fbe_u32_t           journal_num_records;
    fbe_block_count_t   journal_blocks_allocated;

	fbe_lba_t			capacity;
	fbe_block_size_t	block_size;
    fbe_u32_t           backend_number;
    fbe_u32_t           encl_number;
    fbe_u32_t           slot_number;

    /* Debug flags */
    fbe_terminator_drive_debug_flags_t drive_debug_flags;

    /* These are counts of errors such as incorrect sg lists or being beyond the capacity 
     * of the device.  All these counts indicate the client sent something unexpected.
     */
    fbe_u32_t error_count; 

    /* Maximum number of bytes per request (simulates checking that is done in
     * the miniport)
     */
    fbe_u32_t   maximum_transfer_bytes;

}terminator_drive_t;

typedef struct terminator_drive_array_s{
    fbe_spinlock_t      drive_array_spinlock;
    fbe_u32_t           num_simulated_drives;
    fbe_bool_t          is_drive_pulled[FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES];
    terminator_drive_t *drive_array[FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES];

}terminator_drive_array_t;

#define TERMINATOR_MAX_PATTERNS 8
#pragma pack(1)
typedef struct fbe_terminator_compressed_block_data_pattern_s
{
    fbe_u8_t            count;
    fbe_u64_t           pattern;
}fbe_terminator_compressed_block_data_pattern_t;

typedef struct fbe_terminator_compressed_block_s
{
    /*! Header array
     */
    //fbe_u64_t           header_array[3];
    fbe_terminator_compressed_block_data_pattern_t data_pattern[TERMINATOR_MAX_PATTERNS];
    //fbe_u64_t           metadata;
}fbe_terminator_compressed_block_t;
#pragma pack()


typedef fbe_status_t (*terminator_drive_io_func) (terminator_drive_t * drive,
                                                  fbe_lba_t lba, 
                                                  fbe_block_count_t block_count, 
                                                  fbe_block_size_t block_size, 
                                                  fbe_u8_t * data_buffer,
												  void * context); /* Simulated drive will use async completion */
fbe_status_t terminator_simulated_drive_class_init(void);
fbe_status_t terminator_simulated_drive_class_destroy(void);
fbe_status_t terminator_simulated_drive_set_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                        fbe_u32_t first_term_drive_index,
                                                        fbe_u32_t last_term_drive_index,
                                                        fbe_u32_t backend_bus_number,
                                                        fbe_u32_t encl_number,
                                                        fbe_u32_t slot_number,
                                                        fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags);
terminator_drive_t * drive_new(void);
fbe_status_t drive_free(terminator_drive_t * drive);
fbe_status_t free_pulled_drive(void);
fbe_status_t terminator_simulated_drive_set_drive_pulled_flag_status(terminator_drive_t * drive, fbe_bool_t drive_pulled_flag_status);
fbe_status_t set_terminator_simulated_drive_flag_to_drive_pulled(base_component_t * self);
fbe_status_t set_terminator_simulated_drive_flag_to_drive_reinserted(base_component_t * self);
fbe_status_t drive_journal_destroy(terminator_drive_t * drive);

/* creates terminator_sas_drive_info_t structure, allocating memory for it and 
   setting all the fields in the new structure to deliberately invalid values */
terminator_sas_drive_info_t * allocate_sas_drive_info(void);

terminator_sas_drive_info_t * sas_drive_info_new(fbe_terminator_sas_drive_info_t *sas_drive_info);

fbe_drive_type_t drive_get_protocol(terminator_drive_t * self);
void drive_set_protocol(terminator_drive_t * self, fbe_drive_type_t protocol);

fbe_cpd_shim_callback_login_t * sas_drive_info_get_login_data(terminator_sas_drive_info_t * self);
void sas_drive_set_drive_type(terminator_drive_t * self, fbe_sas_drive_type_t type);
fbe_sas_drive_type_t sas_drive_get_drive_type(terminator_drive_t * self);
void sas_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number);
fbe_u32_t sas_drive_get_slot_number(terminator_drive_t * self);
fbe_sas_address_t sas_drive_get_sas_address(terminator_drive_t * self);
void sas_drive_set_sas_address(terminator_drive_t * self, fbe_sas_address_t address);
fbe_status_t sas_drive_call_io_api(terminator_drive_t * self, fbe_terminator_io_t * terminator_io);
fbe_status_t sas_drive_info_get_serial_number(terminator_sas_drive_info_t * self, fbe_u8_t *serial_number);
fbe_status_t sas_drive_info_set_serial_number(terminator_sas_drive_info_t * self, fbe_u8_t *serial_number);
fbe_status_t sas_drive_info_get_product_id(terminator_sas_drive_info_t * self, fbe_u8_t *product_id);
fbe_status_t sas_drive_info_set_product_id(terminator_sas_drive_info_t * self, fbe_u8_t *product_id);


fbe_u8_t * sas_drive_get_serial_number(terminator_drive_t * self);
void sas_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number);
fbe_u8_t * sas_drive_get_product_id(terminator_drive_t * self);
void sas_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id);
fbe_status_t sas_drive_verify_product_id(fbe_u8_t * product_id);
fbe_u8_t * sas_drive_get_inquiry(terminator_drive_t * self);
fbe_u8_t * sas_drive_get_vpd_inquiry_f3(terminator_drive_t * self, fbe_u32_t * size);
fbe_u8_t * sas_drive_get_vpd_inquiry_b2(terminator_drive_t * self, fbe_u32_t * size);
fbe_u8_t * sas_drive_get_mode_page(terminator_drive_t * self, fbe_u32_t * size);
fbe_u8_t * sas_drive_get_mode_page_19(terminator_drive_t * self, fbe_u32_t * size);
fbe_u8_t * sas_drive_get_vpd_inquiry_c0(terminator_drive_t * self, fbe_u32_t * size);
fbe_u8_t * sas_drive_get_log_page_31(terminator_drive_t * self, fbe_u32_t * size);
fbe_status_t sas_drive_set_log_page_31(terminator_drive_t * self, fbe_u8_t * log_page_31, fbe_u32_t page_size);
fbe_u32_t sas_drive_get_backend_number(terminator_drive_t * self);
void sas_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number);
fbe_u32_t sas_drive_get_enclosure_number(terminator_drive_t * self);
void sas_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number);

fbe_status_t sas_drive_get_state(terminator_drive_t * self, terminator_sas_drive_state_t * drive_state);
fbe_status_t sas_drive_set_state(terminator_drive_t * self, terminator_sas_drive_state_t drive_state);

fbe_status_t sas_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t sas_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t sas_drive_get_default_vpd_inq_f3_data(fbe_sas_drive_type_t drive_type, terminator_sas_drive_vpd_inquiry_page_0xf3_t **inq_data);
fbe_status_t sas_drive_get_default_vpd_inq_b2_data(fbe_sas_drive_type_t drive_type, terminator_sas_drive_vpd_inquiry_page_0xb2_t **inq_data);
fbe_status_t sas_drive_get_default_vpd_inq_c0_data(fbe_sas_drive_type_t drive_type, terminator_sas_drive_vpd_inquiry_page_0xc0_t **inq_data);
fbe_status_t sas_drive_get_default_inq_data (fbe_sas_drive_type_t drive_type, terminator_sas_drive_inq_data_t **inq_data);
fbe_status_t sas_drive_get_default_mode_page_0x19_data (fbe_sas_drive_type_t drive_type, terminator_sas_drive_mode_page_0x19_data_t **mode_page_0x19_data);
fbe_status_t sas_drive_set_default_field (fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, const fbe_u8_t *data, fbe_u32_t size);

void drive_clear_reset_count(terminator_drive_t * self);
void drive_increment_reset_count(terminator_drive_t * self);

void drive_clear_error_count(terminator_drive_t * self);
void drive_increment_error_count(terminator_drive_t * self);
fbe_u32_t drive_get_error_count(terminator_drive_t * self);

fbe_lba_t drive_get_capacity(terminator_drive_t * self);
void drive_set_capacity(terminator_drive_t * self, fbe_lba_t capacity);

fbe_block_size_t drive_get_block_size(terminator_drive_t * self);
void drive_set_block_size(terminator_drive_t * self, fbe_block_size_t block_size);

fbe_u32_t drive_get_maximum_transfer_bytes(terminator_drive_t * self);
void drive_set_maximum_transfer_bytes(terminator_drive_t * self, fbe_u32_t maximum_transfer_bytes);

fbe_status_t drive_activate(terminator_drive_t * self);

/* creates terminator_sas_drive_info_t structure, allocating memory for it and 
   setting all the fields in the new structure to deliberately invalid values */
terminator_sata_drive_info_t * allocate_sata_drive_info(void);

terminator_sata_drive_info_t * sata_drive_info_new(fbe_terminator_sata_drive_info_t *sata_drive_info);
fbe_cpd_shim_callback_login_t * sata_drive_info_get_login_data(terminator_sata_drive_info_t * self);
void sata_drive_set_drive_type(terminator_drive_t * self, fbe_sata_drive_type_t type);
fbe_sata_drive_type_t sata_drive_get_drive_type(terminator_drive_t * self);
void sata_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number);
fbe_u32_t sata_drive_get_slot_number(terminator_drive_t * self);
fbe_sas_address_t sata_drive_get_sas_address(terminator_drive_t * self);
void sata_drive_set_sas_address(terminator_drive_t * self, fbe_sas_address_t address);
fbe_status_t sata_drive_call_io_api(terminator_drive_t * self, fbe_terminator_io_t * terminator_io);
fbe_status_t sata_drive_info_get_serial_number(terminator_sata_drive_info_t * self, fbe_u8_t *serial_number);
fbe_status_t sata_drive_info_set_serial_number(terminator_sata_drive_info_t * self, fbe_u8_t *serial_number);
fbe_u8_t * sata_drive_get_serial_number(terminator_drive_t * self);
void sata_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number);

fbe_status_t sata_drive_info_get_product_id(terminator_sata_drive_info_t * self, fbe_u8_t *product_id);
fbe_status_t sata_drive_info_set_product_id(terminator_sata_drive_info_t * self, fbe_u8_t *product_id);
fbe_u8_t * sata_drive_get_product_id(terminator_drive_t * self);
void sata_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id);
fbe_status_t sata_drive_verify_product_id(fbe_u8_t * product_id);
void sata_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number);
void sata_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number);
fbe_u32_t sata_drive_get_backend_number(terminator_drive_t * self);
fbe_u32_t sata_drive_get_enclosure_number(terminator_drive_t * self);

/* allocates and initializes drive attributes structure (dependent on type of drive) 
   currently only SAS and SATA drives are supported
*/
fbe_status_t terminator_drive_initialize_protocol_specific_data(terminator_drive_t * drive_handle);

fbe_status_t terminator_drive_complete_forced_creation(terminator_drive_t * self);

/* Functions that will trace terminator requests if enabled.
 */
void terminator_drive_trace_read(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start);
void terminator_drive_trace_write(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start);
void terminator_drive_trace_write_zero_pattern(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start);

/*
 * Global io operation function points.
 */
extern terminator_drive_io_func terminator_drive_read;
extern terminator_drive_io_func terminator_drive_write;
extern terminator_drive_io_func terminator_drive_write_zero_pattern;

extern terminator_simulated_drive_type_t terminator_simulated_drive_type;

/*
 * IO operation wrapper for remote memory version simulated drive
 */
fbe_status_t terminator_simulated_drive_client_read(terminator_drive_t * drive,
                                                          fbe_lba_t lba, 
                                                          fbe_block_count_t block_count, 
                                                          fbe_block_size_t block_size, 
                                                    fbe_u8_t * data_buffer,
                                                    void * context);

fbe_status_t terminator_simulated_drive_client_write(terminator_drive_t * drive,
                                                     fbe_lba_t lba, 
                                                     fbe_block_count_t block_count, 
                                                     fbe_block_size_t block_size, 
                                                     fbe_u8_t * data_buffer,
                                                     void * context);

fbe_status_t terminator_simulated_drive_client_write_zero_pattern(terminator_drive_t * drive,
                                                                  fbe_lba_t lba, 
                                                                  fbe_block_count_t block_count, 
                                                                  fbe_block_size_t block_size, 
                                                                  fbe_u8_t * data_buffer,
                                                                  void * context);
fbe_status_t terminator_simulated_drive_destroy(terminator_drive_t * drive);

#endif /* TERMINATOR_DRIVE_H */
