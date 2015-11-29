#ifndef FBE_DRIVE_CONFIGURATION_SERVICE_PRIVATE_H
#define FBE_DRIVE_CONFIGURATION_SERVICE_PRIVATE_H

#include "fbe/fbe_time.h"
#include "fbe_base_service.h"
#include "fbe/fbe_drive_configuration_interface.h"



#define NUMBER_OF_DRIVE_TYPES   FBE_DRIVE_TYPE_LAST-1
#define NUMBER_OF_PORT_TYPES    FBE_PORT_TYPE_LAST-1
#define DCS_THRESHOLD_TABLES 2
#define DCS_TABLE_INDEX_SHIFT 24  /*high byte is table index*/

/* This is the default service time we assign to all physical drives. 
 * The service time should get set once we know the type of drive. 
 */
#define FBE_DCS_DEFAULT_SERVICE_TIME_MS 18000
#define FBE_DCS_DEFAULT_REMAP_SERVICE_TIME_MS 18000
#define FBE_DCS_DEFAULT_EMEH_SERVICE_TIME_MS 27000

/* Size to break up FW image.  Note, there is no max on number of
   SG elements per IO, but there is per port.  The smaller the chunk,
   the more elements are needed, which can take away from the port resources,
   causing performance issues.  As long as number of elements is < 100 it
   shouldn't cause any issues.  Today (7/24/13) max fw image is 3MB, so a
   128K sg element size would be 24 elements.   */   
#define FBE_DCS_DEFAULT_FW_IMAGE_CHUNK_SIZE (128*1024)  /* 128K. */


/*************** TYPES *******************/

typedef struct drive_configuration_table_drive_entry_s{
    fbe_drive_configuration_record_t    threshold_record;
    fbe_u32_t                           ref_count;
    fbe_bool_t                          updated;
}drive_configuration_table_drive_entry_t;

typedef struct drive_configuration_table_port_entry_s{
    fbe_drive_configuration_port_record_t   threshold_record;
    fbe_u32_t                               ref_count;
    fbe_bool_t                              updated;
}drive_configuration_table_port_entry_t;


typedef struct fbe_drive_configuration_service_s{
    fbe_base_service_t                      base_service;

    /* DIEH data members */
    fbe_u32_t                               active_table_index;  /* index for active threshold_record_table */
    drive_configuration_table_drive_entry_t threshold_record_tables[DCS_THRESHOLD_TABLES][MAX_THRESHOLD_TABLE_RECORDS];  /* two tables - one for active and one for updating */
    drive_configuration_table_port_entry_t  threshold_port_record_table[MAX_PORT_ENTRIES];
    fbe_spinlock_t                          threshold_table_lock;
    fbe_bool_t                              drive_type_update_sent[NUMBER_OF_DRIVE_TYPES];/*used to mark this drive type already got updates notification*/
    fbe_bool_t                              port_type_update_sent[NUMBER_OF_PORT_TYPES];/*used to mark this drive type already got updates notification*/
    fbe_bool_t                              update_started;

    /* Download data members */
    /* TODO: currently they are global, but a better design would encapulate them here */

    /* Enhanced queuing timer config */
    fbe_drive_configuration_queuing_record_t   queuing_table[MAX_QUEUING_TABLE_ENTRIES];
    fbe_spinlock_t                             queuing_table_lock;

    /* PDO Tunables */
    fbe_dcs_tunable_params_t                pdo_params;

}fbe_drive_configuration_service_t;


/*************** EXTERN ******************/

extern fbe_drive_configuration_service_t    drive_configuration_service;


/************* FUNCTIONS *****************/

/* Utility */
void            drive_configuration_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...);

fbe_status_t    fbe_drive_configuration_get_physical_drive_objects (fbe_topology_control_get_physical_drive_objects_t * get_total_p);
fbe_u32_t       dcs_get_active_table_index(void);
fbe_u32_t       dcs_get_updating_table_index(void);
void            dcs_activate_new_table(void);
fbe_u32_t       dcs_handle_to_table_index(fbe_drive_configuration_handle_t handle);
fbe_u32_t       dcs_handle_to_entry_index(fbe_drive_configuration_handle_t handle);
fbe_bool_t      dcs_is_all_unregistered_from_old_table(void);

/* Paramters */
void            fbe_drive_configuration_init_paramters(void);
fbe_status_t    fbe_drive_configuration_control_get_parameters(fbe_packet_t * packet);
fbe_status_t    fbe_drive_configuration_control_set_parameters(fbe_packet_t * packet);
fbe_status_t    fbe_drive_configuration_control_get_mode_page_overrides(fbe_packet_t * packet);
fbe_status_t    fbe_drive_configuration_control_set_mode_page_byte(fbe_packet_t * packet);
fbe_status_t    fbe_drive_configuration_control_mp_addl_override_clear(fbe_packet_t * packet);


#endif /*FBE_DRIVE_CONFIGURATION_SERVICE_PRIVATE_H*/
