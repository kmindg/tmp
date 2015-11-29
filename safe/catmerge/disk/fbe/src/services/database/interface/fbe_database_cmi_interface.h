#ifndef FBE_DB_CMI_INTERFACE_H
#define FBE_DB_CMI_INTERFACE_H

#include "fbe/fbe_database_packed_struct.h"
#include "fbe_database_private.h"
#include "fbe_database.h"


typedef enum fbe_cmi_msg_type_e{
    FBE_DATABE_CMI_MSG_TYPE_INVALID,
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO,/*active side sends a power save setup to passive*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO_CONFIRM,/*passive side tells active side power save setup is completed*/
    FBE_DATABE_CMI_MSG_TYPE_GET_CONFIG,/*the apssive side wants a copy of the config tables*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER, /*the active side is sending the passive the system db header*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER_CONFIRM, /*passive side confirms its updating system db header*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG,/*the active side is sending the passive a table entry*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE,/*the active side has no more tables to send*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE,/*the passive side is done creating the toplogy*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START,/*active side started a transaction and passive should start as well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM,/*passive side confirms it was able to start the transaction well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT,/*active side commited a transaction and passive should commit as well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM,/*passive side confirms it was able to commit the transaction well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT,/*active side abort a transaction and passive should start as well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM,/*passive side confirms it was able to abort the transaction well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE, /*active side invalidates a transaction and passive should invalidate as well*/
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE_CONFIRM,/*passive side confirms it was able to invalidate the transaction well*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD,/*active side sends a PVD update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_CONFIRM,/*passive side tells active side pvd update is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD,/*active side sends a PVD create to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD_CONFIRM,/*passive side tells active side pvd create is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD,/*active side sends a PVD destroy to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD_CONFIRM,/*passive side tells active side pvd destroy is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_VD,/*active side sends a VD create to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_VD_CONFIRM,/*passive side tells active side vd create is completed*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD,/*active side sends a VD update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD_CONFIRM,/*passive side tells active side vd update is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD,/*active side sends a VD destroy to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD_CONFIRM,/*passive side tells active side vd destroy is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID,/*active side sends a RG create to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID_CONFIRM,/*passive side tells active side RG create is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID,/*active side sends a RG destroy to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID_CONFIRM,/*passive side tells active side RG destroy is completed*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID,/*active side sends a RG update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID_CONFIRM,/*passive side tells active side RG update is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN,/*active side sends a LUN create to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN_CONFIRM,/*passive side tells active side LUN create is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN,/*active side sends a LUN destroy to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN_CONFIRM,/*passive side tells active side LUN destroy is completed*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN,/*active side sends a LUN update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN_CONFIRM,/*passive side tells active side LUN update is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT,/*active side sends a clone object to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT_CONFIRM,/*passive side tells active side clone object is completed*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE,/*active side sends a edge create to passive*/
    FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE_CONFIRM,/*passive side tells active side edge create is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE,/*active side sends a edge destroy to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE_CONFIRM,/*passive side tells active side edge destroy is completed*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG,/*active side sends a spare update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG_CONFIRM,/*passive side tells active side spare update is completed*/
    FBE_DATABE_CMI_MSG_TYPE_DB_SERVICE_MODE, /* Tell peer SP that the current SP database enter service mode */
    
    FBE_DATABE_CMI_MSG_TYPE_SET_OBJECT_STATE_COND,    /*not used anymore. Leave it here just to avoid disruptive upgrade*/
    FBE_DATABE_CMI_MSG_TYPE_SET_OBJECT_STATE_COND_CONFIRM,    /*not used anymore. Leave it here just to avoid disruptive upgrade*/

    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO,/*active side sends a time threshold setup to passive*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO_CONFIRM,/*passive side tells active side time threshold setup is completed*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG,/*active side sends a system background service flag to passive*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG_CONFIRM,/*passive side tells active side system background service flag set is completed*/
    FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_TYPE,/* Old version SP tells New version SP that the message type can't be recognized. */
    FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_SIZE,/* Old version SP tells New version SP that the message size is too big to recognize*/
    FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE,
    FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE_CONFIRM,
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_SHARED_EMV_INFO, /*update peer's shared expected memory info*/
    FBE_DATABE_CMI_MSG_TYPE_MAILBOMB, /* Used to send a mailbomb to panic peer. */
    FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE,  /*connect drives*/
    FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE_CONFIRM,
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR, /* the active side is sending fru descriptor to passive side */
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR_CONFIRM, /*passive side confirms its updateing in-memory fru descriptor*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE,/*active side sends the encryption setup to passive*/
    FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE_CONFIRM,/*active side sends the encryption setup to passive*/
    FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE,/*passive need to update its local table the same way active does*/
    FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE_CONFIRM,
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE,/*active side sends a PVD update to passive*/
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE_CONFIRM,/*passive side tells active side pvd update is completed*/
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE,/*the active side is sending the passive a table via DMA */
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE_CONFIRM,/*the passive side tells active side table update is completed */
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS,/*active side sends encryption keys to passive*/
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS_CONFIRM,/*passive side tells active side key update is completed*/
    FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS,/*active side sends rekey encryption keys to passive*/
    FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS_CONFIRM,/*passive side tells active side rekey is completed*/
    FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG,/*active side sends the pvd setup to passive*/
    FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG_CONFIRM,/*passive side sends the confirm to active*/
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS,/*active side sends rekey encryption keys to passive*/
    FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS_CONFIRM,/*passive side tells active side rekey is completed*/
    FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO, /* Active side wants information about the BE ports */
    FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO_CONFIRM, /* Passive side has not provided the information */
    FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE, /* push encryption backup state to peer */
    FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE_CONFIRM, /* confirmation for new encryption backup state */
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK, /* Active side wants to setup the KEK on the ports */
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_CONFIRM, /* Passive now responds with the handle if okay*/
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK, /* Active side wants to destroy the KEK on the ports */
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_CONFIRM, /* Passive destroys and responds with status */
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK, /* Active side wants to setup the KEK of KEKs on the ports */
    FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK_CONFIRM, /* Passive now responds with the handle */
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK, /* Active side wants to destroy the KEK of KEKs on the ports */
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK_CONFIRM, /* Passive destroys and responds with status */
    FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK, /* Active side wants to destroy the KEK on the ports */
    FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK_CONFIRM, /* Passive destroys and responds with status */
    FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE, /* Active side wants to set the port to a encrytion mode  */
    FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE_CONFIRM, /* Passive side responds with the status  */
    FBE_DATABASE_CMI_MSG_TYPE_KILL_VAULT_DRIVE, /* push request to shoot vault drive to peer */
    FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE,
    FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE_CONFIRM,
    FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL,
    FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_CONFIRM,
    FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN,
    FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN_CONFIRM,

    FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE,/*active side sends a PVD update to passive*/
    FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE_CONFIRM,/*passive side tells active side pvd update is completed*/

    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL,
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_CONFIRM,
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN,
    FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN_CONFIRM,
    FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_DMA,/*active side commited a transaction and passive should commit as well*/
    FBE_DATABE_CMI_MSG_TYPE_CLEAR_USER_MODIFIED_WWN_SEED, /* clear the peer's user modified wwn seed flag */

    /*******************************/

    /* 1. When adding new type THERE ^^^^ make sure to update fbe_database_cmi_get_msg_size as well.
     * 2. When adding new type, make sure to update fbe_database_process_received_cmi_message function
     *    During the NDU, if this new type of message can't be recogonized by the old version SP, 
     *    the old version SP should return a warning CMI message.
     */

	/*******************************/
    FBE_DATABE_CMI_MSG_TYPE_LAST
}fbe_cmi_msg_type_t;

/* Define a msg type with max value of u32 for test.
   It expects peer SP send back an unknown type of cmi message */
#define FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_FOR_TEST  FBE_U32_MAX

/*FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE*/
typedef struct fbe_cmi_msg_update_config_done_s{
    fbe_u64_t		entries_sent;
}fbe_cmi_msg_update_config_done_t;


typedef enum fbe_cmi_msg_err_type_e {
    FBE_CMI_MSG_ERR_TYPE_INVALID,
    FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE,
    FBE_CMI_MSG_ERR_TYPE_UNKNOWN_MSG_TYPE,
    FBE_CMI_MSG_ERR_TYPE_DISMATCH_TRANSACTION,
    FBE_CMI_MSG_ERR_TYPE_LAST,
}fbe_cmi_msg_err_type_t;

/*FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM*/
typedef struct fbe_cmi_msg_transaction_confirm_s{
    fbe_status_t		status;
    fbe_cmi_msg_err_type_t err_type;
    fbe_cmi_msg_type_t  received_msg_type;
    fbe_u64_t           sep_version;
}fbe_cmi_msg_transaction_confirm_t;

typedef struct fbe_cmi_msg_update_peer_table_s{
    database_config_table_type_t	table_type;/*which one are we updating*/
    database_table_size_t			table_entry;/*where in the table to stick it*/

    /*the data*/
    union{
        database_object_entry_t				object_entry;
        database_edge_entry_t				edge_entry;
        database_user_entry_t				user_entry;
        database_global_info_entry_t		global_info_entry;
        database_system_spare_entry_t       system_spare_entry;
    }update_peer_table_type;
}fbe_cmi_msg_update_peer_table_t;

typedef struct fbe_cmi_msg_update_config_table_s{
    database_config_table_type_t	table_type;/*which one are we updating*/
}fbe_cmi_msg_update_config_table_t;

typedef struct fbe_cmi_msg_update_peer_fru_descriptor_s{
    fbe_homewrecker_fru_descriptor_t fru_descriptor;
}fbe_cmi_msg_update_peer_fru_descriptor_t;

typedef struct fbe_cmi_msg_update_fru_descriptor_confirm_s{
    fbe_status_t status;
}fbe_cmi_msg_update_fru_descriptor_confirm_t;

typedef struct fbe_cmi_msg_update_peer_system_db_header_s{
    fbe_u32_t   system_db_header_size; /*the actual size of the system db header*/
    fbe_u64_t   current_sep_version;  /*current sep version number. equal to SEP_PACKAGE_VERSION*/
    database_system_db_header_t system_db_header; /*copy of system db header in this side*/
}fbe_cmi_msg_update_peer_system_db_header_t;

typedef struct fbe_cmi_msg_update_system_db_header_confirm_s{
    fbe_u64_t        current_sep_version; /*current sep version of our side*/
    fbe_status_t    status;
}fbe_cmi_msg_update_system_db_header_confirm_t;

typedef struct fbe_database_online_planned_drive_confirm_s
{
    fbe_status_t    status;
}fbe_database_online_planned_drive_confirm_t;

typedef struct fbe_database_peer_synch_context_s{
    fbe_semaphore_t	semaphore;
    fbe_u64_t        peer_sep_version; /*current sep version number of peer*/
    fbe_status_t	status;
}fbe_database_peer_synch_context_t;

typedef struct fbe_database_cmi_sync_context_s
{
    fbe_semaphore_t	semaphore;
    fbe_u64_t        peer_sep_version; /*current sep version number of peer*/
    fbe_status_t	status;
    void *context_buffer;
}fbe_database_cmi_sync_context_t;

typedef struct fbe_cmi_msg_database_get_be_port_info_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_get_be_port_info_t;

typedef struct fbe_cmi_msg_database_set_encryption_bakcup_state_s{
    fbe_database_peer_synch_context_t * context;
    fbe_encryption_backup_state_t new_state;
}fbe_cmi_msg_database_set_encryption_bakcup_state_t;

typedef struct fbe_cmi_msg_database_setup_kek_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_setup_kek_t kek_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_setup_kek_t;

typedef struct fbe_cmi_msg_database_destroy_kek_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_destroy_kek_t kek_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_destroy_kek_t;

typedef struct fbe_cmi_msg_database_setup_kek_kek_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_setup_kek_kek_t kek_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_setup_kek_kek_t;

typedef struct fbe_cmi_msg_database_destroy_kek_kek_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_destroy_kek_kek_t kek_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_destroy_kek_kek_t;

typedef struct fbe_cmi_msg_database_reestablish_kek_kek_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_control_reestablish_kek_kek_t kek_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_reestablish_kek_kek_t;

typedef struct fbe_cmi_msg_database_set_port_encryption_mode_s{
    fbe_database_cmi_sync_context_t * context;
    fbe_database_port_encryption_op_t op;
    fbe_database_control_port_encryption_mode_t mode_info;
    fbe_status_t return_status;
}fbe_cmi_msg_database_port_encryption_mode_t;

typedef struct ffbe_cmi_msg_database_kill_vault_drive_s{
    fbe_database_peer_synch_context_t * context;
    fbe_object_id_t pdo_id;
    fbe_status_t return_status;
}fbe_cmi_msg_database_kill_vault_drive_t; 

/******************************************************* 
 * This structure is used for repling the sender when 
 * receiving an unknown CMI message from Peer 
 ******************************************************/
typedef struct fbe_cmi_msg_unknown_msg_response_s {
    fbe_cmi_msg_err_type_t err_type;
    fbe_cmi_msg_type_t  received_msg_type;
    fbe_u32_t           received_msg_size;
    fbe_u64_t           sep_version;
    fbe_status_t        status;
}fbe_cmi_msg_unknown_msg_response_t;

struct fbe_database_cmi_msg_s;
typedef fbe_status_t (*fbe_database_cmi_callback_t)(struct fbe_database_cmi_msg_s *returned_msg, fbe_status_t completion_status, void *context);

/* For transfer database transaction to peer by CMI message*/
#pragma pack(1)
typedef struct database_cmi_transaction_s {
    fbe_database_transaction_id_t        transaction_id;
    fbe_database_transaction_type_t      transaction_type;
    fbe_u64_t   job_number; /*the job number of the job that starts this transaction*/
    database_transaction_state_t state;

    fbe_u32_t   max_user_entries;
    fbe_u32_t   user_entry_size;
    fbe_u32_t   user_entry_array_offset;
    fbe_u32_t   max_object_entries;
    fbe_u32_t   object_entry_size;
    fbe_u32_t   object_entry_array_offset;
    fbe_u32_t   max_edge_entries;
    fbe_u32_t   edge_entry_size;
    fbe_u32_t   edge_entry_array_offset;
    fbe_u32_t   max_global_info_entries;
    fbe_u32_t   global_info_entry_size;
    fbe_u32_t   global_info_entry_array_offset;
    database_user_entry_t user_entries[DATABASE_TRANSACTION_MAX_USER_ENTRY];
    database_object_entry_t object_entries[DATABASE_TRANSACTION_MAX_OBJECTS];
    database_edge_entry_t edge_entries[DATABASE_TRANSACTION_MAX_EDGES];
    /* Holds system global info entries */
    database_global_info_entry_t global_info_entries[DATABASE_TRANSACTION_MAX_GLOBAL_INFO];
    /**
     * New elements are appended here!
     */

}database_cmi_transaction_t;
#pragma pack()

/* For transfer database transaction to peer by CMI message*/
#pragma pack(1)
typedef struct database_cmi_transaction_header_s {
    fbe_database_transaction_id_t        transaction_id;
    fbe_database_transaction_type_t      transaction_type;
    fbe_u64_t   job_number; /*the job number of the job that starts this transaction*/
    database_transaction_state_t state;
#if 0
    fbe_u32_t   max_user_entries;
    fbe_u32_t   user_entry_size;
    fbe_u32_t   user_entry_array_offset;
    fbe_u32_t   max_object_entries;
    fbe_u32_t   object_entry_size;
    fbe_u32_t   object_entry_array_offset;
    fbe_u32_t   max_edge_entries;
    fbe_u32_t   edge_entry_size;
    fbe_u32_t   edge_entry_array_offset;
    fbe_u32_t   max_global_info_entries;
    fbe_u32_t   global_info_entry_size;
    fbe_u32_t   global_info_entry_array_offset;
    /**
     * New elements are appended here!
     */
#endif
}database_cmi_transaction_header_t;
#pragma pack()

typedef struct fbe_cmi_msg_get_config_s {
    fbe_u64_t   user_table_physical_address;
    fbe_u64_t   object_table_physical_address;
    fbe_u64_t   edge_table_physical_address;
    fbe_u64_t   system_spare_table_physical_address;
    fbe_u64_t   global_info_table_physical_address;
    fbe_u32_t   user_table_alloc_size;
    fbe_u32_t   object_table_alloc_size;
    fbe_u32_t   edge_table_alloc_size;
    fbe_u32_t   system_spare_table_alloc_size;
    fbe_u32_t   global_info_table_alloc_size;
    fbe_u64_t   encryption_key_table_physical_address;
    fbe_u32_t   encryption_key_table_alloc_size;
    fbe_u32_t   transaction_alloc_size;
    fbe_u64_t   transaction_peer_physical_address;
}fbe_cmi_msg_get_config_t;

/*************************************************************** 
 * this is the generic data structure used by DB service to send
 * data to the peer SP 
 * New elements are only allowed to append at the structure 
 * in the union.
 * *************************************************************/ 
typedef struct fbe_database_cmi_msg_s{
    fbe_version_header_t version_header;
    fbe_cmi_msg_type_t	msg_type;
    fbe_database_cmi_callback_t completion_callback;
    
     union{
         /******************************************************************
          * After supporting database versioning, 
          * 1. if the size of the following structure grows, 
          *    the newer version SP will receive warning message from old version SP. 
          *    The newer version SP can take action in function 
          *    fbe_database_cmi_unknown_cmi_msg_size_by_peer()
          *    if needed. 
          * 2. When grows the following structure, only appened at the end.
          * 3. the new added element in the following structure is set with zero
          *    If need other default value, please update the function
          *    fbe_db_cmi_msg_init()
          ******************************************************************/
         fbe_database_power_save_t 							power_save_info;
         fbe_cmi_msg_update_peer_table_t					db_cmi_update_table;
         fbe_cmi_msg_update_config_done_t					update_done_msg;
         fbe_cmi_msg_transaction_confirm_t					transaction_confirm;
         fbe_database_control_update_pvd_t					update_pvd;
         fbe_database_control_update_pvd_block_size_t   	update_pvd_block_size;
         fbe_database_control_pvd_t							create_pvd;
         fbe_database_control_vd_t							create_vd;
         fbe_database_control_update_vd_t					update_vd;
         fbe_database_control_destroy_object_t				destroy_object;
         fbe_database_control_raid_t						create_raid;
         fbe_database_control_update_raid_t					update_raid;
         fbe_database_control_lun_t							create_lun;
         fbe_database_control_update_lun_t					update_lun;
         fbe_database_control_clone_object_t                clone_object;
         fbe_database_control_create_edge_t					create_edge;
         fbe_database_control_destroy_edge_t				destroy_edge;
         fbe_database_control_update_system_spare_config_t	update_spare_config;
		 database_cmi_transaction_t							db_transaction;
		 database_cmi_transaction_header_t					db_transaction_dma;
         fbe_database_time_threshold_t                      time_threshold_info;
         fbe_database_control_update_peer_system_bg_service_t system_bg_service;
         fbe_cmi_msg_update_peer_fru_descriptor_t       fru_descriptor;
         fbe_cmi_msg_update_fru_descriptor_confirm_t fru_descriptor_confirm;
         fbe_cmi_msg_update_peer_system_db_header_t             system_db_header;
         fbe_cmi_msg_update_system_db_header_confirm_t         system_db_header_confirm;
         fbe_database_control_online_planned_drive_t      make_drive_online;
         fbe_database_online_planned_drive_confirm_t     make_drive_online_confirm;
         fbe_cmi_msg_unknown_msg_response_t                unknown_msg_res;
         fbe_database_emv_t						shared_emv_info;
         fbe_database_control_drive_connection_t          drive_connection;
         fbe_database_encryption_t 							encryption_info;
         fbe_database_control_update_encryption_mode_t      encryption_mode;
         fbe_cmi_msg_get_config_t                           get_config;
         fbe_cmi_msg_update_config_table_t                  update_table;
         fbe_database_control_setup_encryption_key_t        setup_encryption_key;
         fbe_global_pvd_config_t 							pvd_config;
         fbe_database_control_update_drive_key_t            update_drive_key;
         fbe_cmi_msg_database_get_be_port_info_t            get_be_port_info;
         fbe_cmi_msg_database_set_encryption_bakcup_state_t update_encryption_backup_state;
         fbe_cmi_msg_database_setup_kek_t                   setup_kek;
         fbe_cmi_msg_database_destroy_kek_t                 destroy_kek;
         fbe_cmi_msg_database_setup_kek_kek_t               setup_kek_kek;
         fbe_cmi_msg_database_destroy_kek_kek_t             destroy_kek_kek;
         fbe_cmi_msg_database_reestablish_kek_kek_t         reestablish_kek_kek;
         fbe_cmi_msg_database_port_encryption_mode_t        mode_info;
         fbe_cmi_msg_database_kill_vault_drive_t            kill_drive_info;
         fbe_database_control_create_ext_pool_t             create_ext_pool;
         fbe_database_control_create_ext_pool_lun_t         create_ext_pool_lun;
         /******************************************************************
          * After supporting database versioning, 
          * If the new type of CMI message added, 
          * the newer version SP will receive warning message from old version SP. 
          * The newer version SP can take action in function 
          * fbe_database_cmi_unknown_cmi_msg_by_peer()
          * if needed. 
          ******************************************************************/
         } payload;

}fbe_database_cmi_msg_t;


/*initialize the CMI portion of DB*/
fbe_status_t fbe_database_cmi_init(void);

/*destroy the CMI portion of DB*/
fbe_status_t fbe_database_cmi_destroy(void);

/*get a pre allocated contiguous memory to be used to send to peer*/
fbe_database_cmi_msg_t * fbe_database_cmi_get_msg_memory(void);

/*get the outstanding DB CMI msg count*/
fbe_s32_t fbe_database_cmi_get_outstanding_msg_count(void);

/*get the total DB CMI msg count allocated*/
fbe_s32_t fbe_database_cmi_get_msg_count_allocated(void);


/*return the pre allocated memory we got using fbe_database_cmi_get_msg_memory*/
void fbe_database_cmi_return_msg_memory(fbe_database_cmi_msg_t *cmi_msg_memory);

/*send a message to the peer (this is an asynch function and should handle everything in completion, no status is returned)*/
void fbe_database_cmi_send_message(fbe_database_cmi_msg_t * db_cmi_msg, void *context);


/*to be implemented by the database code to process a received CMI message*/
fbe_status_t fbe_database_process_received_cmi_message(fbe_database_cmi_msg_t * db_cmi_msg);

/*to be implemented by the database code to process a lost peer*/
fbe_status_t fbe_database_process_lost_peer(void);

fbe_u32_t fbe_database_cmi_get_msg_size(fbe_cmi_msg_type_t type);

fbe_status_t fbe_database_ask_for_peer_db_update(void);
void fbe_database_cmi_process_get_config(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_cmi_update_peer_system_db_header(database_system_db_header_t *system_db_header, fbe_u64_t *peer_sep_version);
fbe_status_t fbe_database_cmi_update_peer_tables(void);
fbe_status_t fbe_database_cmi_update_peer_tables_dma(void);
void fbe_database_cmi_process_update_config(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_update_config_done(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
void fbe_database_cmi_process_update_config_table(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_update_config_table_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_update_config_passive_init_done(fbe_database_service_t *db_service);
void fbe_database_cmi_process_update_drive_keys_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_setup_encryption_keys_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);


void fbe_database_cmi_process_invalid_vault_drive(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_cmi_send_general_command_to_peer_sync(fbe_database_service_t * fbe_database_service,
                                                                void  *config_data,
                                                                fbe_cmi_msg_type_t config_cmd);
fbe_status_t fbe_database_transaction_start_request_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_start_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_commit_request_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_commit_dma_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_commit_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_invalidate_request_from_peer(fbe_database_service_t* db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_invalidate_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);
void fbe_database_cmi_process_get_be_port_info(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_get_be_port_info_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_setup_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_setup_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_destroy_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_destroy_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_setup_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_setup_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_destroy_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_destroy_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_reestablish_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_reestablish_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_port_encryption_mode(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_port_encryption_mode_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_cmi_send_update_config_to_peer(fbe_database_service_t * fbe_database_service,
                                                         void  *config_data,
                                                         fbe_cmi_msg_type_t config_cmd);
void fbe_database_cmi_process_set_encryption_backup_state(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
void fbe_database_cmi_process_set_encryption_backup_state_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_config_change_confirmed_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_config_change_request_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_abort_request_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg);
fbe_status_t fbe_database_transaction_abort_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);


/* update peer in-memory fru descriptor */
fbe_status_t fbe_database_cmi_update_peer_fru_descriptor(fbe_homewrecker_fru_descriptor_t*fru_descriptor_sent_to_peer);
void fbe_database_cmi_process_update_fru_descriptor(fbe_database_service_t* fbe_database_service, fbe_database_cmi_msg_t* cmi_msg);
fbe_status_t fbe_database_cmi_update_fru_descriptor_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);


/* notify peer SP that current database service becomes service mode */
fbe_status_t fbe_database_cmi_send_db_service_mode_to_peer(void);
fbe_status_t fbe_database_cmi_update_system_db_header_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);
/* Function for testing unknown cmi messsage by Peer SP */
fbe_status_t database_check_unknown_cmi_msg_handling_test(void);

void fbe_database_db_service_mode_state_from_peer(fbe_database_service_t *fbe_database_service);

void fbe_database_cmi_process_update_system_db_header(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_cmi_sync_online_drive_request_to_peer(fbe_database_control_online_planned_drive_t* online_drive_request);

fbe_status_t fbe_database_cmi_online_drive_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg);

void fbe_database_cmi_process_online_drive_request(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

/*********************************************************************** 
  In oldver version SEP, CMI message from Peer may can't be recognized .
 **********************************************************************/
/* 1. When type of CMI message is not supported by current SP, handle it by this function */
void fbe_database_handle_unknown_cmi_msg_type_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
/* 2. When the size of CMI message is bigger than the one in current SP, handle it by this function*/
void fbe_database_handle_unknown_cmi_msg_size_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

/*********************************************************************** 
  In newer version SEP, our CMI message may can't be recognized by Peer SP.
 **********************************************************************/
/* 1. Send a type of message to Peer SP, but the message can't be recognized by Peer.
   Peer will send back a message to notify the sender. The sender use this function to handle it*/
fbe_status_t fbe_database_cmi_unknown_cmi_msg_by_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);
/* 2. Send a type of message to Peer SP, but the message size can't be recognized by Peer.
   Peer will send back a message to notify the sender. The sender use this function to handle it*/
fbe_status_t fbe_database_cmi_unknown_cmi_msg_size_by_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_cmi_update_peer_shared_emv_info(fbe_database_emv_t* shared_emv_info);

void fbe_database_cmi_process_update_shared_emv_info(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg);

fbe_status_t fbe_database_config_change_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context);
fbe_status_t fbe_database_cmi_process_send_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                     fbe_status_t completion_status,
                                                                     void *context);

void fbe_database_cmi_process_connect_drives(fbe_database_cmi_msg_t *cmi_msg);

void fbe_database_cmi_disable_service_when_lost_peer(void);

fbe_status_t fbe_database_cmi_set_peer_version(fbe_u64_t peer_sep_version);

fbe_status_t fbe_database_cmi_clear_user_modified_wwn_seed(void);
#endif /*FBE_DB_CMI_INTERFACE_H*/
