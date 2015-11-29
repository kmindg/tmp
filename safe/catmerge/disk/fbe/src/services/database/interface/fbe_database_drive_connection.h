#ifndef FBE_DATABASE_DRIVE_CONNECTION_H
#define FBE_DATABASE_DRIVE_CONNECTION_H

#include "fbe_database_private.h"

typedef enum database_pvd_discover_flag_e
{
    /*just take normal drive process procedure*/
    DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS = 0x0,
    /*homewrecker logic should not prevent the drive becomes online*/
    DATABASE_PVD_DISCOVER_FLAG_HOMEWRECKER_FORECE_ONLINE = 0x1
}database_pvd_discover_flag_t;

/*!*******************************************************************
 * @struct fbe_database_physical_drive_info_t
 *********************************************************************
 * @brief
 *  records pdo information.
 *
 *********************************************************************/
typedef struct fbe_database_physical_drive_info_s
{
    fbe_object_id_t                     drive_object_id;
    fbe_u32_t                           bus;
    fbe_u32_t                           enclosure;
    fbe_u32_t                           slot;
    fbe_block_edge_geometry_t           block_geometry;
    fbe_lba_t                           exported_capacity;   // exclude fru signature space
    fbe_pdo_maintenance_mode_flags_t    maintenance_mode_bitmap;
    fbe_u8_t                            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} fbe_database_physical_drive_info_t;

/*!*******************************************************************
 * @struct database_pvd_discover_object_t
 *********************************************************************
 * @brief
 *  records pdo object ids for more processing.
 *
 *********************************************************************/
typedef struct database_pvd_discover_object_s {
    fbe_queue_element_t         queue_element;
    fbe_object_id_t             object_id; /* PDO object id*/
    fbe_lifecycle_state_t       lifecycle_state;
    /*added flag to indicate how to process a drive.
    * (1) If it is DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS, just take normal process procedure.
    * (2) If the DATABASE_PVD_DISCOVER_FLAG_HOMEWRECKER_FORECE_ONLINE bit is set, 
    * homewrecker should not prevent this drive becomes online.*/
    fbe_u8_t                            drive_process_flag;
    fbe_object_id_t              pvd_obj_id;  /* used for connecting to pdo */
    fbe_time_t                   process_timestamp;  /* The timestamp when processing this drive */
}database_pvd_discover_object_t;

#define SECONDS_TO_RESTART_REINIT_SYSTEM_PVD_JOB   120   // The time to restart the reinit_system_pvd job

/*!******************************************************************
 * @enum database_drive_movement_type
 ********************************************************************
 * @brief
 * define each type of drive movement.
 *
 * The drive movement type is determined by drive original type and its location.
 *
 * The drive original type determined by FRU Signature. 
 * If a drive inserted to array and no FRU Signature on it. Then this is a new drive. 
 * Otherwise, this drive belonged to some array originally.
 * If it was in the current array, we can find the wwn seed in FRU Signature match with chassis wwn seed. 
 * We can determined the drive original type by FRU Signature bus, enc, slot. Bus, enc, slot from 0_0_0 to 0_0_4 
 * means this drive was DB drive originally.Other FRU Signature bus, enc, slot combination shows this drive was 
 * a USER drive originally.
 *
 * The drive location, is determined by location information in LDO only.
 * We can know the LDO whether is in a DB slot or a USER slot clearly by LDO information.
 */
 typedef enum _database_drive_movement_type
{
    INVALID_DRIVE_MOVEMENT = 0,
    INSERT_A_NEW_DRIVE_TO_A_USER_SLOT = 1,
    INSERT_A_NEW_DRIVE_TO_A_DB_SLOT = 2,
    INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_USER_SLOT = 3,
    INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_DB_SLOT = 4,
    INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT =5,
    INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ANOTHER_USER_SLOT = 6,
    INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_USER_SLOT = 7,
    INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_DB_SLOT = 8,
    INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT = 9,
    INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ANOTHER_USER_SLOT = 10,
    INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_USER_SLOT = 11,
    INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_DB_SLOT =12,
    INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_USER_SLOT = 13,
    INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_DB_SLOT = 14,
    INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ITS_ORIGINAL_SLOT = 15,
    INSERT_CURRENT_ARRAY_DB_DRIVE_TO_A_USER_SLOT = 16,
    INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ANOTHER_DB_SLOT = 17,
    
    REINSERT_A_USER_DRIVE_TO_ITS_ORIGINAL_SLOT  = 18,
    REINSERT_A_DB_DRIVE_TO_ITS_ORIGINAL_SLOT    = 19,
    MOVE_A_USER_DRIVE_TO_ANOTHER_USER_SLOT      = 20,
    MOVE_A_USER_DRIVE_TO_A_DB_SLOT              = 21,
    MOVE_A_DB_DRIVE_TO_ANOTHER_DB_SLOT          = 22,
    MOVE_A_DB_DRIVE_TO_A_USER_SLOT              = 23,
    INSERT_A_NEW_DRIVE_TO_USER_SLOT             = 24,
    INSERT_A_NEW_DRIVE_TO_DB_SLOT               = 25
} database_drive_movement_type;

 typedef enum _fbe_database_drive_type
{
    FBE_DATABASE_INVALID_DRIVE_TYPE = 0,
    FBE_DATABASE_NEW_DRIVE = 1,
    FBE_DATABASE_OTHER_ARRAY_USER_DRIVE = 2, 
    FBE_DATABASE_OTHER_ARRAY_DB_DRIVE = 3,
    /* type #4 == type (#5 + #6) */
    /* we can not differ the #5 and #6 in
      * fbe_database_determine_drive_type_by_fru_signature()
      * so if it returns type #4, then we check the drive is BIND or UNBIND
      * out of the funtion, so type #4 is needed.
      */
    FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE = 4,
    FBE_DATABASE_CURRENT_ARRAY_UNBIND_USER_DRIVE = 5,
    FBE_DATABASE_CURRENT_ARRAY_BIND_USER_DRIVE = 6,
    FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE = 7 
} fbe_database_drive_type;

 typedef enum _fbe_database_slot_type
{
    FBE_DATABASE_INVALID_SLOT_TYPE = 0,
    FBE_DATABASE_ITS_ORIGINAL_SLOT = 1,
    FBE_DATABASE_USER_SLOT = 2,
    FBE_DATABASE_ANOTHER_USER_SLOT = 3,
    FBE_DATABASE_DB_SLOT = 4,
    FBE_DATABASE_ANOTHER_DB_SLOT = 5
} fbe_database_slot_type;

fbe_status_t fbe_database_add_drive_to_be_processed(database_pvd_operation_t *connect_context, 
                                                           fbe_object_id_t pdo,
                                                           fbe_lifecycle_state_t lifecycle_state,
                                                           database_pvd_discover_flag_t  pvd_discover_flag);

fbe_status_t fbe_database_connect_to_pdo_by_location(fbe_object_id_t pvd_object_id, 
                                                     fbe_u32_t bus,
                                                     fbe_u32_t enclosure,
                                                     fbe_u32_t slot,
                                                     fbe_database_physical_drive_info_t *drive_info);

fbe_status_t fbe_database_connect_to_pdo_by_serial_number(fbe_object_id_t pvd_object_id, 
                                                          fbe_u8_t *SN,
                                                          fbe_database_physical_drive_info_t *drive_info);

fbe_status_t fbe_database_add_pvd_to_be_connected(database_pvd_operation_t *connect_context, fbe_object_id_t pvd_object_id);
fbe_bool_t fbe_database_is_pvd_to_be_connected(database_pvd_operation_t *connect_context, fbe_object_id_t pvd_object_id);

fbe_status_t fbe_database_determine_drive_movement_type(database_pvd_operation_t* discover_context,
                                                        fbe_database_physical_drive_info_t* pdo_info,
                                                        database_drive_movement_type* drive_movement_type);

fbe_status_t fbe_database_determine_drive_type
    (database_pvd_operation_t* discover_context,
     fbe_database_physical_drive_info_t* pdo_info, 
     fbe_database_drive_type* drive_type);

fbe_status_t  fbe_database_determine_slot_type
    (fbe_database_physical_drive_info_t* pdo_info, fbe_database_slot_type* slot_type);


fbe_bool_t fbe_database_validate_pvd(database_object_entry_t* pvd_entry);

fbe_status_t fbe_database_drive_connection_start_drive_process(fbe_database_service_t *database_service);

fbe_status_t fbe_database_get_pdo_info(fbe_object_id_t pdo, fbe_database_physical_drive_info_t *pdo_info);
fbe_status_t fbe_database_notify_pdo_logically_offline_due_to_maintenance(fbe_object_id_t pdo, const fbe_database_physical_drive_info_t *pdo_info);
fbe_status_t fbe_database_notify_pdo_logically_offline(fbe_object_id_t pdo, fbe_block_transport_logical_state_t state);

#endif /*  FBE_DATABASE_DRIVE_CONNECTION_H */
