#ifndef FBE_TEST_CONFIGURATIONS_H
#define FBE_TEST_CONFIGURATIONS_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_database.h"
#include "sep_utils.h"                             
#include "sep_test_io.h"
#include "fbe/fbe_board_types.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_terminator_api.h"                        
#include "fbe/fbe_limits.h"

#define FBE_TEST_ENCLOSURES_PER_BUS     FBE_ENCLOSURES_PER_BUS
#define FBE_TEST_PHYSICAL_BUS_COUNT     FBE_PHYSICAL_BUS_COUNT
#define FBE_TEST_ENCLOSURE_SLOTS        FBE_ENCLOSURE_SLOTS

#define SIMPLE_CONFIG_DRIVE_MAX     4
#define SIMPLE_CONFIG_MAX_PORTS     1
#define SIMPLE_CONFIG_MAX_ENCLS     1
#define SIMPLE_CONFIG_MAX_OBJECTS   ((SIMPLE_CONFIG_MAX_PORTS * SIMPLE_CONFIG_MAX_ENCLS * SIMPLE_CONFIG_DRIVE_MAX) + (SIMPLE_CONFIG_MAX_PORTS * SIMPLE_CONFIG_MAX_ENCLS) + SIMPLE_CONFIG_MAX_PORTS + 1)

#define NAXOS_MAX_OB_AND_VOY        2
#define NAXOS_MAX_ENCLS             3
#define NAXOS_MAX_DRIVES            12

#define LAPA_RIO_MAX_PORTS          2
#define LAPA_RIO_MAX_ENCLS          3
#define LAPA_RIO_MAX_DRIVES         15
#define LAPA_RIO_MAX_OBJECTS        ((LAPA_RIO_MAX_DRIVES * LAPA_RIO_MAX_ENCLS * LAPA_RIO_MAX_PORTS) + (LAPA_RIO_MAX_ENCLS * LAPA_RIO_MAX_PORTS) + LAPA_RIO_MAX_PORTS + 1 - 24)

#define CHAUTAUQUA_MAX_PORTS          1
#define CHAUTAUQUA_MAX_ENCLS          FBE_TEST_ENCLOSURES_PER_BUS
#define CHAUTAUQUA_MAX_DRIVES         15
#define CHAUTAUQUA_MAX_OBJECTS        ((CHAUTAUQUA_MAX_PORTS * CHAUTAUQUA_MAX_ENCLS * CHAUTAUQUA_MAX_DRIVES) +(CHAUTAUQUA_MAX_ENCLS * CHAUTAUQUA_MAX_PORTS) + CHAUTAUQUA_MAX_PORTS + 1)

#define COCOCAY_MAX_PORTS          1
#define COCOCAY_MAX_ENCLS          3
#define COCOCAY_MAX_DRIVES         15
#define COCOCAY_MAX_OBJECTS        ((COCOCAY_MAX_PORTS * COCOCAY_MAX_ENCLS * COCOCAY_MAX_DRIVES) + (COCOCAY_MAX_PORTS * COCOCAY_MAX_ENCLS) + COCOCAY_MAX_PORTS + 1)

#define FP_MAX_PORTS                1
#define FP_MAX_ENCLS                1
#define FP_MAX_DRIVES               15
#define FP_MAX_OBJECTS              ((FP_MAX_PORTS * FP_MAX_ENCLS * FP_MAX_DRIVES) + (FP_MAX_PORTS * FP_MAX_ENCLS) + FP_MAX_PORTS + 1)

#define LOS_VILOS_SATA_MAX_PORTS          1
#define LOS_VILOS_SATA_MAX_ENCLS          1
#define LOS_VILOS_SATA_MAX_DRIVES         12
#define LOS_VILOS_SATA_MAX_OBJECTS        ((LOS_VILOS_SATA_MAX_PORTS * LOS_VILOS_SATA_MAX_ENCLS * LOS_VILOS_SATA_MAX_DRIVES) + (LOS_VILOS_SATA_MAX_PORTS * LOS_VILOS_SATA_MAX_ENCLS) + LOS_VILOS_SATA_MAX_PORTS + 1)

#define LOS_VILOS_MAX_DRIVES 12

#define FBE_TEST_PORT_SAS_ADDRESS_BASE  0x50BB097A00000000
#define FBE_TEST_ENCL_SAS_ADDRESS_BASE  0x50EE097A00000000
#define FBE_TEST_LCC_SAS_ADDRESS_BASE   0x50EE097A00000EE0   // For edge expanders in Voyager
#define FBE_TEST_DRIVE_SAS_ADDRESS_BASE 0x50DD097A00000000

#define FBE_TEST_DRIVE_SERIAL_BASE      0x6000

#define BIT(_b) (1LL << (_b))

/*!*******************************************************************
 * @def BUILD_PORT_SAS_ADDRESS
 *********************************************************************
 * @brief Builds port SAS address.
 *********************************************************************/
#define FBE_TEST_BUILD_PORT_SAS_ADDRESS(port_num) \
            (FBE_TEST_PORT_SAS_ADDRESS_BASE + \
            ((port_num) <<24))

/*!*******************************************************************
 * @def BUILD_ENCL_SAS_ADDRESS
 *********************************************************************
 * @brief Builds enclosure SAS address.
 *********************************************************************/
//#define ZRT_BUILD_ENCL_SAS_ADDRESS(port_num, encl_num) \
//            (ZEUS_READY_TEST_ENCL_SAS_ADDRESS_BASE + \
//             (((port_num * FBE_API_ENCLOSURES_PER_BUS) + encl_num) * 0x100))
#define FBE_TEST_BUILD_ENCL_SAS_ADDRESS(port_num, encl_num) \
            (FBE_TEST_ENCL_SAS_ADDRESS_BASE + \
             ((port_num) <<24) + ((encl_num) << 16))

/*!*******************************************************************
 * @def BUILD_LCC_SAS_ADDRESS
 *********************************************************************
 * @brief Builds edge expander (LCC) SAS address. 
 *********************************************************************/
#define FBE_TEST_BUILD_LCC_SAS_ADDRESS(port_num, encl_num, eenum) \
            (FBE_TEST_LCC_SAS_ADDRESS_BASE + \
             ((port_num) <<24) + ((encl_num) << 16) + ((eenum) << 12) + (eenum))

/*!*******************************************************************
 * @def BUILD_DISK_SAS_ADDRESS
 *********************************************************************
 * @brief Builds SAS address for a disk.
 *********************************************************************/
#define FBE_TEST_BUILD_DISK_SAS_ADDRESS(port_num, encl_num, slot_num) \
            (FBE_TEST_DRIVE_SAS_ADDRESS_BASE + \
            (((fbe_u64_t)(port_num) << 24) +((fbe_u64_t)(encl_num) << 16) + (slot_num)))

/*!*******************************************************************
 * @def INVALID_DRIVE_NUMBER
 *********************************************************************
 * @brief an invalid drive number.
 *********************************************************************/
#define INVALID_DRIVE_NUMBER -1

/*!*******************************************************************
 * @def MAX_DRIVE_COUNT_VIPER
 *********************************************************************
 * @brief maximum number of drives that can be added to a viper
 *  enclosure.
 *********************************************************************/
#define MAX_DRIVE_COUNT_VIPER 15
#define MAX_DRIVE_COUNT_CITADEL 25 
#define MAX_DRIVE_COUNT_BUNKER     15
#define MAX_DRIVE_COUNT_DERRINGER 25
#define MAX_DRIVE_COUNT_VOYAGER 60
#define MAX_DRIVE_COUNT_VIKING 120
#define MAX_DRIVE_COUNT_FALLBACK 25
#define MAX_DRIVE_COUNT_BOXWOOD 12
#define MAX_DRIVE_COUNT_KNOT 25
#define MAX_DRIVE_COUNT_PINECONE 12
#define MAX_DRIVE_COUNT_STEELJAW 12
#define MAX_DRIVE_COUNT_RAMHORN 25
#define MAX_DRIVE_COUNT_RHEA    12
#define MAX_DRIVE_COUNT_MIRANDA 25
#define MAX_DRIVE_COUNT_CALYPSO 25
#define MAX_DRIVE_COUNT_TABASCO 25
#define MAX_DRIVE_COUNT_ANCHO 15
#define MAX_DRIVE_COUNT_CAYENNE 60
#define MAX_DRIVE_COUNT_NAGA 120

// 12 slots
#define PINECONE_LOW_DRVCNT_MASK        0x0813
#define PINECONE_MED_DRVCNT_MASK        0x0CAF
#define PINECONE_HIGH_DRVCNT_MASK       0x0A63

// 15 slots
#define VIPER_LOW_DRVCNT_MASK           0x4013
#define VIPER_MED_DRVCNT_MASK           0x44AF
#define VIPER_HIGH_DRVCNT_MASK          0x7A63

#define ANCHO_LOW_DRVCNT_MASK           0x4013
#define ANCHO_MED_DRVCNT_MASK           0x44AF
#define ANCHO_HIGH_DRVCNT_MASK          0x7A63

// 25 slots
#define DERRINGER_LOW_DRVCNT_MASK       0x1805101
#define DERRINGER_MED_DRVCNT_MASK       0x1A010C1
#define DERRINGER_HIGH_DRVCNT_MASK      0x1E0A6AF

#define TABASCO_LOW_DRVCNT_MASK         0x1805101
#define TABASCO_MED_DRVCNT_MASK         0x1A010C1
#define TABASCO_HIGH_DRVCNT_MASK        0x1E0A6AF

// 60 slots
#define VOYAGER_LOW_DRVCNT_MASK         0x805010401000013
#define VOYAGER_MED_DRVCNT_MASK         0x890A0503700F0A3
#define VOYAGER_HIGH_DRVCNT_MASK        0xF91A25037C4F2E3

#define CAYENNE_LOW_DRVCNT_MASK         0x805010401000013
#define CAYENNE_MED_DRVCNT_MASK         0x890A0503700F0AF
#define CAYENNE_HIGH_DRVCNT_MASK        0xF91A25037C4F2E3

// 120 slots, only using 64
#define VIKING_LOW_DRVCNT_MASK          0x0805010401000013
#define VIKING_MED_DRVCNT_MASK          0x8890A0503700F0A3
#define VIKING_HIGH_DRVCNT_MASK         0xFF91A25037C4F2E3

#define NAGA_LOW_DRVCNT_MASK            0x0805010401000013
#define NAGA_MED_DRVCNT_MASK            0x8890A0503700F0AF
#define NAGA_HIGH_DRVCNT_MASK           0xFF91A25037C4F2E3

// DPEs
#define BUNKER_LOW_DRVCNT_MASK          0x8013
#define BUNKER_MED_DRVCNT_MASK           0x84A3
#define BUNKER_HIGH_DRVCNT_MASK         0xFA63

#define CITADEL_LOW_DRVCNT_MASK         0x85101
#define CITADEL_MED_DRVCNT_MASK     0xA10C1
#define CITADEL_HIGH_DRVCNT_MASK        0xEA6AF

#define FALLBACK_LOW_DRVCNT_MASK       0x85101
#define FALLBACK_MED_DRVCNT_MASK       0xA10C1
#define FALLBACK_HIGH_DRVCNT_MASK      0xEA6AF

#define BOXWOOD_LOW_DRVCNT_MASK        0x8013
#define BOXWOOD_MED_DRVCNT_MASK        0x84A3
#define BOXWOOD_HIGH_DRVCNT_MASK       0xFA63

#define KNOT_LOW_DRVCNT_MASK           0x85101
#define KNOT_MED_DRVCNT_MASK           0xA10C1
#define KNOT_HIGH_DRVCNT_MASK          0xEA6AF

// Moons
#define CALYPSO_LOW_DRVCNT_MASK         0x85101
#define CALYPSO_MED_DRVCNT_MASK         0xA10C1
#define CALYPSO_HIGH_DRVCNT_MASK        0xEA6AF

#define RHEA_LOW_DRVCNT_MASK            0x8013
#define RHEA_MED_DRVCNT_MASK            0x84A3
#define RHEA_HIGH_DRVCNT_MASK           0xFA63

#define MIRANDA_LOW_DRVCNT_MASK         0x85101
#define MIRANDA_MED_DRVCNT_MASK         0xA10C1
#define MIRANDA_HIGH_DRVCNT_MASK        0xEA6AF

#define DRIVE_MASK_IS_NOT_USED 0
#define MAX_PORTS_IS_NOT_USED 0
#define MAX_ENCLS_IS_NOT_USED 0
#define MAX_DRIVES_IS_NOT_USED 0

/*!*******************************************************************
 * @struct fbe_test_params_s
 *********************************************************************
 * @brief Structure that comprise the different parameters for
 * different test configurations to be tested.
 *********************************************************************/
typedef struct fbe_test_params_s
{
    fbe_u32_t                   test_number;
    fbe_char_t                  *title; 
    fbe_board_type_t            board_type;
    fbe_port_type_t             port_type;
    fbe_u32_t                   io_port_number;
    fbe_u32_t                   portal_number;
    fbe_u32_t                   backend_number;
    fbe_sas_enclosure_type_t    encl_type;
    fbe_u32_t                   encl_number;
    fbe_sas_drive_type_t        drive_type;
    fbe_u32_t                   drive_number;
    fbe_u32_t                   max_drives;
    //optional fields follow. 
    fbe_u32_t                   max_ports;
    fbe_u32_t                   max_enclosures;
    fbe_u64_t                   drive_mask;
} fbe_test_params_t;

typedef enum mixed_encl_config_types_e
{
    ENCL_CONFIG_VIPER,
    ENCL_CONFIG_PINECONE,
    ENCL_CONFIG_DERRINGER,
    ENCL_CONFIG_VOYAGER,
    ENCL_CONFIG_BUNKER,
    ENCL_CONFIG_CITADEL,
    ENCL_CONFIG_FALLBACK,
    ENCL_CONFIG_BOXWOOD,
    ENCL_CONFIG_KNOT,
    ENCL_CONFIG_RHEA,
    ENCL_CONFIG_MIRANDA,
    ENCL_CONFIG_CALYPSO,
    ENCL_CONFIG_TABASCO,
    ENCL_CONFIG_ANCHO,
    ENCL_CONFIG_VIKING,
    ENCL_CONFIG_CAYENNE,
    ENCL_CONFIG_NAGA,
    ENCL_CONFIG_LIMIT
} mixed_encl_config_type_t;

#define FBE_TEST_PORT_SAS_ADDRESS_BASE  0x50BB097A00000000
#define FBE_TEST_ENCL_SAS_ADDRESS_BASE  0x50EE097A00000000
#define FBE_TEST_LCC_SAS_ADDRESS_BASE   0x50EE097A00000EE0   // For edge expanders in Voyager
#define FBE_TEST_DRIVE_SAS_ADDRESS_BASE 0x50DD097A00000000


/*!*******************************************************************
 * @def BUILD_PORT_SAS_ADDRESS
 *********************************************************************
 * @brief Builds port SAS address.
 *********************************************************************/
#define FBE_BUILD_PORT_SAS_ADDRESS(port_num) \
            (FBE_TEST_PORT_SAS_ADDRESS_BASE + \
            ((port_num) <<24))

/*!*******************************************************************
 * @def BUILD_ENCL_SAS_ADDRESS
 *********************************************************************
 * @brief Builds enclosure SAS address.
 *********************************************************************/
#define FBE_BUILD_ENCL_SAS_ADDRESS(port_num, encl_num) \
            (FBE_TEST_ENCL_SAS_ADDRESS_BASE + \
             ((port_num) <<24) + ((encl_num) << 16))

/*!*******************************************************************
 * @def BUILD_LCC_SAS_ADDRESS
 *********************************************************************
 * @brief Builds edge expander (LCC) SAS address. 
 *********************************************************************/
#define FBE_BUILD_LCC_SAS_ADDRESS(port_num, encl_num, eenum) \
            (FBE_TEST_LCC_SAS_ADDRESS_BASE + \
             ((port_num) <<24) + ((encl_num) << 16) + ((eenum) << 12) + (eenum))

/*!*******************************************************************
 * @def BUILD_DISK_SAS_ADDRESS
 *********************************************************************
 * @brief Builds SAS address for a disk.
 *********************************************************************/
#define FBE_BUILD_DISK_SAS_ADDRESS(port_num, encl_num, slot_num) \
            (FBE_TEST_DRIVE_SAS_ADDRESS_BASE+ \
            (((fbe_u64_t)(port_num) << 24) +((fbe_u64_t)(encl_num) << 16) + (slot_num)))

/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_TEST_WILD_CARD
 *********************************************************************
 * @brief A number which can be used as wild card
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_TEST_WILD_CARD (-1)


fbe_status_t fbe_test_load_maui_config(SPID_HW_TYPE platform_type, fbe_test_params_t *test);
fbe_status_t fbe_test_load_naxos_config(fbe_test_params_t *test);
fbe_status_t fbe_test_load_lapa_rios_config(void);
fbe_status_t fbe_test_load_los_vilos_config(void);
fbe_status_t fbe_test_load_los_vilos_sata_config(void);
fbe_status_t fbe_test_load_chautauqua_config(fbe_test_params_t *test, SPID_HW_TYPE platform_type);
fbe_status_t fbe_test_load_cococay_config(fbe_test_params_t *test);
fbe_status_t fbe_test_load_yancey_config(void);

fbe_status_t fbe_test_load_simple_config(void);
fbe_status_t fbe_test_load_mixed_config(void);
fbe_status_t fbe_test_load_simple_config_table_data(fbe_test_params_t *test_data);

static fbe_status_t fbe_test_internal_insert_voyager_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles);
static fbe_status_t fbe_test_internal_insert_voyager_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_terminator_api_device_handle_t *drive_handle);
fbe_status_t fbe_test_startPhyPkg(fbe_bool_t waitForObjectReady, fbe_u32_t num_objects);
fbe_status_t fbe_test_startEnvMgmtPkg(fbe_bool_t waitForObjectReady);
fbe_status_t fbe_test_wait_for_all_esp_objects_ready(void);
fbe_status_t fbe_test_startAutoFupEnvMgmtPkg(fbe_bool_t waitForObjectReady);

/*!@todo following drive related function definations are part of fbe_test_sep_drive_utils.c file so need to move in another new
               header file */
fbe_status_t fbe_test_sep_drive_verify_pvd_state(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_lifecycle_state_t expected_state,
                                                 fbe_u32_t timeout_ms);
void fbe_test_sep_drive_configure_extra_drives_as_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t raid_group_count);
void fbe_test_sep_drive_get_permanent_spare_edge_index(fbe_object_id_t   vd_object_id,
                                                       fbe_edge_index_t  *edge_index_p);
void fbe_test_sep_drive_wait_permanent_spare_swap_in(fbe_test_rg_configuration_t * rg_config_p,
                                                     fbe_u32_t position);
void fbe_test_sep_drive_wait_extra_drives_swap_in(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count);
fbe_status_t fbe_test_sep_drive_check_for_disk_zeroing_event(fbe_object_id_t pvd_object_id, 
                                                            fbe_u32_t message_code,  fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot);
fbe_status_t  fbe_test_sep_drive_disable_background_zeroing_for_all_pvds(void);
fbe_status_t  fbe_test_sep_drive_disable_sniff_verify_for_all_pvds(void);
fbe_status_t fbe_test_sep_drive_enable_background_zeroing_for_all_pvds(void);
fbe_status_t  fbe_test_sep_drive_enable_sniff_verify_for_all_pvds(void);
/*!@todo following drive related.. - end*/

void elmo_physical_config(void);
void elmo_create_physical_config(fbe_bool_t b_use_512);
void elmo_create_physical_config_for_rg(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t num_raid_groups);
void elmo_load_config(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_u32_t luns_per_rg,
                      fbe_u32_t chunks_per_lun);
void shaggy_physical_config(void);
void scoobert_physical_config (fbe_block_size_t block_size);
void zeroing_physical_config(void);
void bluemeanie_physical_config(void);
void job_control_physical_config(void);
void mougli_physical_config(void);

void elmo_load_config(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_u32_t luns_per_rg,
                      fbe_u32_t chunks_per_lun);
void elmo_create_config(fbe_test_rg_configuration_t *rg_config_p,
                        fbe_u32_t luns_per_rg,
                        fbe_u32_t chunks_per_lun);
void elmo_load_sep_and_neit(void);
void elmo_load_esp_sep_and_neit(void);
void elmo_create_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                             fbe_u32_t luns_per_rg,
                             fbe_u32_t chunks_per_lun);
void elmo_set_trace_level(fbe_trace_level_t level);

void rick_load_sep_and_neit(void);

void fozzie_slf_test_wait_pvd_slf_state(fbe_object_id_t pvd_id, fbe_bool_t is_slf);
void big_bird_write_background_pattern(void);
void big_bird_write_zero_background_pattern(void);
void big_bird_read_background_pattern(fbe_rdgen_pattern_t pattern);
void big_bird_remove_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                fbe_u32_t raid_group_count, 
                                fbe_u32_t drives_to_remove,
                                fbe_bool_t b_reinsert_same_drive,
                                fbe_u32_t msecs_between_removals,
                                fbe_test_drive_removal_mode_t removal_mode);
void big_bird_insert_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                fbe_u32_t raid_group_count, 
                                fbe_u32_t drives_to_insert,
                                fbe_bool_t b_reinsert_same_drive);
void big_bird_wait_for_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_u32_t raid_group_count,
                                fbe_u32_t drives_to_remove);
void big_bird_start_io(fbe_test_rg_configuration_t * const rg_config_p,
                       fbe_api_rdgen_context_t **context_p,
                       fbe_u32_t threads,
                       fbe_u32_t io_size_blocks,
                       fbe_rdgen_operation_t rdgen_operation,
                       fbe_test_random_abort_msec_t ran_abort_msecs,
                       fbe_api_rdgen_peer_options_t peer_options);
void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);

void fbe_test_sep_util_run_rdgen_for_pvd_ex(fbe_object_id_t pvd_object_id, 
                                            fbe_package_id_t package_id,
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks);
void fbe_test_sep_util_wait_for_rg_base_config_flags(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t raid_group_count,
                                                     fbe_u32_t wait_seconds,
                                                     fbe_base_config_clustered_flags_t base_config_clustered_flags,
                                                     fbe_bool_t b_cleared);
fbe_status_t big_bird_wait_for_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t num_rebuilds,
                                          fbe_u32_t target_wait_ms);

void big_bird_wait_for_rgs_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                      fbe_u32_t raid_group_count,
                                      fbe_u32_t num_rebuilds);
void fbe_test_io_wait_for_io_count(fbe_api_rdgen_context_t *context_p,
                                   fbe_u32_t num_tests,
                                   fbe_u32_t io_count,
                                   fbe_u32_t timeout_msecs);

fbe_status_t handy_manny_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                            fbe_object_id_t object_id,
                                                            fbe_class_id_t class_id,
                                                            fbe_rdgen_operation_t rdgen_operation,
                                                            fbe_lba_t max_lba,
                                                            fbe_u32_t io_size_blocks,
                                                            fbe_rdgen_pattern_t pattern,
                                                            fbe_u32_t num_passes,
                                                            fbe_u32_t num_ios);
fbe_status_t handy_manny_test_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                       fbe_object_id_t object_id,
                                                       fbe_class_id_t class_id,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_lba_t capacity,
                                                       fbe_u32_t max_passes,
                                                       fbe_u32_t threads,
                                                       fbe_u32_t io_size_blocks,
                                                       fbe_rdgen_pattern_t pattern);
fbe_status_t abby_cadabby_wait_for_raid_group_verify(fbe_object_id_t rg_object_id,
                                                     fbe_u32_t wait_seconds);
void abby_cadabby_set_debug_flags(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_u32_t raid_group_count);
fbe_u32_t abby_cadabby_get_config_count(fbe_test_logical_error_table_test_configuration_t *config_p);
void abby_cadabby_simulation_setup(fbe_test_logical_error_table_test_configuration_t *logical_error_config_p,
                                   fbe_bool_t b_randomize_block_size);
void abby_cadabby_load_physical_config_raid_type(fbe_test_logical_error_table_test_configuration_t *config_p,
                                                 fbe_raid_group_type_t raid_type);
void abby_cadabby_load_physical_config(fbe_test_logical_error_table_test_configuration_t *config_p);
void abby_cadabby_run_tests(fbe_test_logical_error_table_test_configuration_t *config_p, 
                            fbe_test_random_abort_msec_t rand_aborts,
                            fbe_raid_group_type_t raid_type);
fbe_status_t abby_cadabby_enable_error_injection_for_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count);
void abby_cadabby_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t abby_cadabby_wait_for_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t raid_group_count);
void abby_cadabby_validate_io_status(fbe_test_logical_error_table_info_t *test_params_p,
                                     fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_api_rdgen_context_t *context_p,
                                     fbe_u32_t num_contexts,
                                     fbe_u32_t drives_to_power_off,
                                     fbe_test_random_abort_msec_t ran_abort_msecs);
fbe_status_t abby_cadabby_validate_event_log_errors(void);
fbe_status_t abby_cadabby_get_max_correctable_errors(fbe_raid_group_type_t raid_group_type,
                                                     fbe_u32_t  width,
                                                     fbe_u32_t *max_correctable_errors_p);
fbe_status_t abby_cadabby_wait_timed_out(fbe_test_logical_error_table_info_t *test_params_p,
                                         fbe_u32_t elapsed_msec,
                                         fbe_u32_t raid_type_max_correctable_errors,
                                         fbe_api_logical_error_injection_get_stats_t *stats_p,
                                         fbe_api_rdgen_get_stats_t *rdgen_stats_p,
                                         fbe_u64_t min_object_validations,
                                         fbe_u64_t min_object_injected_errors,
                                         fbe_u64_t max_object_injected_errors,
                                         fbe_object_id_t min_validations_object_id,
                                         fbe_object_id_t min_count_object_id,
                                         fbe_object_id_t max_count_object_id);
fbe_status_t abby_cadabby_rg_get_error_totals_both_sps(fbe_object_id_t object_id,
                                                       fbe_api_logical_error_injection_get_object_stats_t *stats_p);
fbe_status_t abby_cadabby_get_table_info(fbe_u32_t table_number,
                                         fbe_test_logical_error_table_info_t **info_p);
void big_bird_run_shutdown_test(fbe_test_rg_configuration_t *rg_config_p, void * unused_context_p);
void big_bird_zero_and_abort_io_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p);
void big_bird_normal_degraded_io_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p);
void big_bird_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void big_bird_normal_degraded_encryption_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p);

/* These common setup/cleanup need to be included in all tests using mumford_the_magician_run_tests */
void mumford_the_magician_common_setup(void);
void mumford_the_magician_common_cleanup(void);
void mumford_the_magician_run_tests(fbe_test_event_log_test_configuration_t *config_array_p);
fbe_status_t mumford_the_magician_run_single_test_case(fbe_test_rg_configuration_t * raid_group_p,
                                                       fbe_test_event_log_test_case_t * test_case_p);
void mumford_simulation_setup(fbe_test_event_log_test_configuration_t *config_p_qual,
                              fbe_test_event_log_test_configuration_t *config_p_extended);
fbe_status_t mumford_the_magician_run_single_test_case_on_zeroed_lun(fbe_test_rg_configuration_t * raid_group_p,
                                                                     fbe_test_event_log_test_case_t * test_case_p);
fbe_status_t mumford_the_magician_validates_event_log_message(fbe_test_event_log_test_result_t * result_p,
                                                              fbe_object_id_t logged_object_id,
                                                              fbe_bool_t b_verify_present);
fbe_status_t edgar_the_bug_verify_write_log_flushed(fbe_object_id_t raid_group_id, fbe_bool_t b_chk_remap, fbe_u32_t slot_id, fbe_bool_t *b_flushed_p);
fbe_status_t edgar_the_bug_zero_write_log_slot(fbe_object_id_t rg_object_id, fbe_u32_t slot_id);
void fbe_test_disable_raid_group_background_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_base_config_background_operation_t background_op);
void fbe_test_enable_raid_group_background_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_base_config_background_operation_t background_op);
void kermit_set_rg_options(void);
void kermit_unset_rg_options(void);
static fbe_status_t kermit_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                              fbe_u32_t *position_p);
fbe_u32_t mixed_config_get_encl_config_list_size(void);
fbe_u32_t mixed_config_number_of_configured_drives(fbe_u32_t encl_index, fbe_u32_t num_slots);
fbe_u64_t mixed_config_get_drive_mask (mixed_encl_config_type_t config_item);
fbe_sas_enclosure_type_t mixed_config_get_enclosure_type(mixed_encl_config_type_t config_item);
char *fbe_sas_encl_type_to_string(fbe_sas_enclosure_type_t sas_encl_type);
void kalia_test_validate_kek_of_kek_handling(fbe_test_rg_configuration_t *rg_config_ptr);

/*!*******************************************************************
 * @struct fbe_test_raid_get_info_test_case_t
 *********************************************************************
 * @brief
 *   This is a single test case for testing the raid get class info
 *   usurper command.
 *********************************************************************/
typedef struct fbe_test_raid_get_info_test_case_s
{
    /***************************************** 
     * Expected input values.
     *****************************************/
    /*! Number of drives in the raid group.
     */
    fbe_u32_t width;
    /*! This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;
    fbe_class_id_t class_id; /*!< Class to issue this to. */

    /*! The overall capacity the raid group is exporting.
     * This is imported capacity - the metadata capacity. 
     */
    fbe_lba_t exported_capacity;

    /*! Output: The overall capacity the raid group is importing. 
     *          This is exported capacity + the metadata capacity. 
     */
    fbe_lba_t imported_capacity;

    /* This is the size imported of a single drive. 
     */
    fbe_lba_t single_drive_capacity;

    /*! Input: Determines if the user selected bandwidth or iops  
     */
    fbe_bool_t b_bandwidth;

    /*! True to input exported cap otherwise input single drive cap. 
     */
    fbe_bool_t b_input_exported_capacity; 

    /***************************************** 
     * Expected output values.
     *****************************************/

    /*! number of data disks (so on R5 for example this is width-1)
     */
    fbe_u16_t data_disks;

    /*! Size of each data element.
     */
    fbe_element_size_t          element_size;
    /*! Output: Number of elements (vertically) before parity rotates.
     */
    fbe_elements_per_parity_t  elements_per_parity;

    /* Status we expect this test case to receive.
     */
    fbe_status_t expected_status;
}
fbe_test_raid_get_info_test_case_t;
void fbe_test_raid_class_get_info_cases(fbe_test_raid_get_info_test_case_t *test_case_p);

void big_bird_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_u32_t raid_group_count,
                            fbe_bool_t b_transition_pvd);

void big_bird_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                            fbe_u32_t raid_group_count,
                            fbe_bool_t b_transition_pvd_fail,
                            fbe_bool_t b_pull_drive,
                            fbe_test_drive_removal_mode_t removal_mode);

void shaggy_remove_drive(fbe_test_rg_configuration_t * rg_config_p,
                         fbe_u32_t position,
                         fbe_api_terminator_device_handle_t * out_drive_info_p);

void shaggy_test_verify_raid_object_state_after_drive_removal(fbe_test_rg_configuration_t  * rg_config_p,
                                                              fbe_u32_t * position_p,
                                                              fbe_u32_t num_drives_removed);
                                                
void shaggy_test_wait_permanent_spare_swap_in(fbe_test_rg_configuration_t * rg_config_p,
                                              fbe_u32_t position);

void shaggy_test_check_event_log_msg(fbe_u32_t message_code,
                                     fbe_test_raid_group_disk_set_t *spare_drive_location,
                                     fbe_object_id_t desired_drive_pvd_object_id);

void shaggy_verify_sep_object_state(fbe_object_id_t object_id,
                                    fbe_lifecycle_state_t expected_state);

void shaggy_verify_pdo_state(fbe_u32_t port_number, 
                             fbe_u32_t enclosure_number,
                             fbe_u32_t slot_number,
                             fbe_lifecycle_state_t lifecycle_state,
                             fbe_bool_t destroyed);

void shaggy_test_get_pvd_object_id_by_position(fbe_test_rg_configuration_t * in_rg_config_p,
                                               fbe_u32_t position,
                                               fbe_object_id_t * pvd_object_id_p);

void diabolicdiscdemon_write_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p);

void clifford_write_background_pattern(void);
void clifford_write_error_pattern(fbe_object_id_t in_object_id, 
                                  fbe_lba_t       in_start_lba, 
                                  fbe_u32_t       in_size);
void clifford_initiate_verify_cmd(fbe_object_id_t            in_lun_object_id,
                                  fbe_lun_verify_type_t      in_verify_type,
                                  fbe_lun_verify_report_t*   in_verify_report_p,
                                  fbe_u32_t                  in_pass_count);

void clifford_initiate_group_verify_cmd(fbe_test_rg_configuration_t *in_current_rg_config_p,
                                        fbe_object_id_t              in_raid_object_id,
                                        fbe_lun_verify_type_t        in_verify_type,
                                        fbe_u32_t                    pass_count,
                                        fbe_u32_t                    injected_on_first,
                                        fbe_u32_t                    injected_on_second);
void clifford_disable_sending_pdo_usurper(fbe_test_rg_configuration_t * rg_config_p);

/*!*************************************************
 * @struct monitor_test_case_s
 ***************************************************
 * @brief darkwing duck test case structure
 *
 ***************************************************/
typedef struct fbe_monitor_test_case_s
{
    char * test_case_name;  /* The name of the test case */
    fbe_object_id_t rg_object_id; /* Object to insert events on */
    fbe_u32_t state; /* Monitor state for hook */
    fbe_u32_t substate; /* Monitor substate for hook */
    fbe_base_config_clustered_flags_t clustered_flag; /* the flag to check for */
    fbe_raid_group_local_state_t local_state; /* the local state to check for */
    fbe_u32_t num_to_remove; /* Number of drives to remove in the rg */
    fbe_u32_t drive_slots_to_remove[3]; /* The slot position to rebuild in the rg*/
    fbe_bool_t peer_reboot; /* Passive SP Shutdown during the required join state? */
    fbe_api_terminator_device_handle_t      drive_info[3]; // drive info needed for reinsertion
}
fbe_monitor_test_case_t;
/**************************************
 * end monitor_test_case_s
 **************************************/


fbe_status_t darkwing_duck_create_random_config(fbe_test_rg_configuration_t * rg_config_p, fbe_u32_t num_rgs); 
fbe_status_t darkwing_duck_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate);
fbe_status_t darkwing_duck_set_target_SP_hook(fbe_test_rg_configuration_t* rg_config_p, 
                                              fbe_monitor_test_case_t *test_case_p); 
fbe_status_t darkwing_duck_remove_target_SP_hook(fbe_test_rg_configuration_t* rg_config_p, 
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate);
void darkwing_duck_setup_passive_sp_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p); 
void darkwing_duck_shutdown_peer_sp(void);
void darkwing_duck_shutdown_local_sp(void);
void darkwing_duck_startup_peer_sp(fbe_test_rg_configuration_t * rg_config_p, 
                                   fbe_monitor_test_case_t * test_cases_p,
                                   fbe_test_rg_config_test func_p);
void darkwing_duck_duplicate_config(fbe_test_rg_configuration_t * from_rg_config_p,
                                    fbe_test_rg_configuration_t * to_rg_config_p);
void darkwing_duck_remove_failed_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                        fbe_monitor_test_case_t *test_cases_p);



void captain_planet_wait_for_local_state(fbe_test_rg_configuration_t * rg_config_p,
                                         fbe_monitor_test_case_t * test_case_p);
void captain_planet_set_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p, 
                                       fbe_monitor_test_case_t *test_case_p);
void captain_planet_remove_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p, 
                                       fbe_monitor_test_case_t *test_case_p); 
void captain_planet_wait_for_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_monitor_test_case_t * test_case_p);

void shredder_crash_both_sps(fbe_test_rg_configuration_t * rg_config_p, 
                             fbe_monitor_test_case_t *test_cases_p,
                             fbe_test_rg_config_test func_p);
void shredder_dual_sp_crash_test(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_monitor_test_case_t *test_cases_p);

#endif /*FBE_TEST_CONFIGURATIONS_H*/
