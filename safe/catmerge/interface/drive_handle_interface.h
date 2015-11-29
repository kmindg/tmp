#ifndef DRIVE_HANDLE_INTERFACE_H
#define DRIVE_HANDLE_INTERFACE_H

typedef unsigned int DRIVE_HANDLE;

typedef struct drive_phys_location_s
{
   unsigned int bus;
   unsigned int enclosure;   //This is not a position field. It's an encl_num (range 0 to ENCLOSURES_PER_BUS-1)
   unsigned int slot;
} DRIVE_PHYS_LOCATION;

/* Currently all the data in the drive handle table are non-persistent. */
typedef DRIVE_PHYS_LOCATION  DRIVE_HANDLE_SYNC_DATA;

void drive_handle_init_table(void);
int drive_handle_get_phys_location(DRIVE_HANDLE handle, DRIVE_PHYS_LOCATION *phy_location);
int drive_handle_get_drive_handle(DRIVE_PHYS_LOCATION *phy_location, DRIVE_HANDLE *handle);
unsigned int drive_handle_get_drive_handle_by_bes(unsigned int bus_num, unsigned int encl_num, unsigned int slot_num);
int drive_handle_init_phys_location(DRIVE_PHYS_LOCATION *phys_location);
int drive_handle_is_initialized(unsigned int fru);
int drive_handle_validate_in_sync(DRIVE_HANDLE handle, DRIVE_HANDLE_SYNC_DATA * sync_data_to_be_validated_p);
int drive_handle_retrieve_sync_data(unsigned char * buffer, unsigned int buffer_size);
int drive_handle_populate_sync_data(unsigned char * buffer, unsigned int buffer_size);
int drive_handle_check_sync_data(DRIVE_HANDLE drive_handle, 
                                          DRIVE_HANDLE_SYNC_DATA * drive_handle_sync_data_p);
unsigned int drive_handle_get_bus_from_fru(unsigned int m_fru);
unsigned int drive_handle_get_encl_from_fru(unsigned int m_fru);
unsigned int drive_handle_get_slot_from_fru(unsigned int m_fru);
unsigned int drive_handle_get_addr_from_fru(unsigned int m_fru);
BOOL drive_handle_get_bes_from_fru(UINT_32 fru,UINT_32 *bus,UINT_32 *encl,UINT_32 *slot);
void drive_handle_print_encl_addr(unsigned int ring_buffer);
BOOL drive_handle_check_for_free_handle(DRIVE_PHYS_LOCATION *phy_location, DRIVE_HANDLE *handle);
void drive_handle_set_drive_bank_by_fru_num(unsigned int fru_num, unsigned int  bank_width);



#endif
