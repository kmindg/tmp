#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe_test_configurations.h"
/*************************
 *   MACRO DEFINITIONS
 *************************/
 
#define READY_STATE_WAIT_TIME       20000

#define MAUI_NUMBER_OF_OBJ          27
#define YANCEY_NUMBER_OF_OBJ        17
#define NAXOS_NUMBER_OF_OBJ         89
#define CHAUT_NUMBER_OF_OBJ         499
#define LAPA_RIOS_NUMBER_OF_OBJ     LAPA_RIO_MAX_OBJECTS

#define COCOCAY_NUMBER_OF_OBJ       89
#define COCOCAY_MAX_ENCLS           3
#define COCOCAY_MAX_DRIVES          15

#define PP_EXPECTED_ERROR           0
#define ZRT_ATTRIBUTE_BUFFER_LENGTH 64


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

// populates the physical package suite.  Add all new test creation to this function
mut_testsuite_t* fbe_test_create_physical_package_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_physical_package_dualsp_test_suite(mut_function_t startup, mut_function_t teardown);

extern fbe_service_control_entry_t sep_control_entry;
extern fbe_io_entry_function_t sep_io_entry;

extern fbe_service_control_entry_t  physical_package_control_entry;
extern fbe_io_entry_function_t		physical_package_io_entry;

extern char * static_config_short_desc;
extern char * static_config_long_desc;
void static_config_test(void);
void static_config_test_table_driven(fbe_test_params_t *test);

extern char * maui_short_desc;
extern char * maui_long_desc;
void maui(void);
fbe_status_t maui_verify(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t lifecycle_state, fbe_u32_t timeout);
fbe_status_t maui_load_and_verify(fbe_test_params_t *test);
fbe_status_t maui_load_and_verify_table_driven(fbe_test_params_t *test);
void maui_load_single_configuration(void);

extern char * yancey_short_desc;
extern char * yancey_long_desc;
fbe_status_t yancey_load_and_verify(void);
fbe_status_t yancey_verify_topology(void);

void naxos(void);
fbe_status_t naxos_verify(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout,fbe_u32_t max_drives, 
                            fbe_u64_t drive_mask, fbe_u32_t max_enclosures);

fbe_status_t naxos_load_and_verify_table_driven(fbe_test_params_t *test);
fbe_test_params_t  *fbe_test_get_naxos_test_table(fbe_u32_t index_num);
fbe_u32_t fbe_test_get_naxos_num_of_tests(void);

fbe_status_t cococay_load_and_verify(fbe_test_params_t *test);
fbe_status_t cococay_verify(fbe_u32_t port, fbe_u32_t encl, 
                            fbe_lifecycle_state_t state,
                            fbe_u32_t timeout,
                            fbe_u32_t max_enclosures,
                            fbe_u32_t max_drives);
//fbe_status_t cococay_load_and_verify(void);

void lapa_rios(void);
fbe_status_t lapa_rios_load_and_verify(fbe_test_params_t *test);

void chautauqua(void);
fbe_status_t chautauqua_load_and_verify(fbe_test_params_t *test);
fbe_status_t chautauqua_load_and_verify_with_platform(fbe_test_params_t *test, SPID_HW_TYPE platform_type);
fbe_test_params_t  *fbe_test_get_chautauqua_test_table(fbe_u32_t index_num);
fbe_u32_t fbe_test_get_chautauqua_num_of_tests(void);

extern char * los_vilos_short_desc;
extern char * los_vilos_long_desc;
void los_vilos(void);
fbe_status_t los_vilos_load_and_verify(void);
fbe_status_t los_vilos_unload(void);

void parinacota(void);
fbe_status_t parinacota_load_and_verify(void);

void los_vilos_sata(void);
fbe_status_t los_vilos_sata_load_and_verify(void);

void mont_tremblant(void);
fbe_status_t mont_tremblant_load_and_verify(void);

extern char *   denali_short_desc;
extern char *   denali_long_desc;
void            denali_dest_injection(void);
void            denali_load_and_verify(void);
fbe_status_t    denali_unload(void);

void vai(void);

void home(void);
void grylls(void);
extern char * petes_coffee_and_tea_short_desc;
extern char * petes_coffee_and_tea_long_desc;
void petes_coffee_and_tea(void);


void republica_dominicana(void);
void sobo_4(void);
void sobo_4_sata(void);
void tiruvalla(void);
void zhiqis_coffee_and_tea(void);
void trichirapalli(void);

void wallis_pond(void);
void wallis_pond_setup(void);

void bora_bora (void);
void trivandrum(void);
void kamphaengphet(void);
void cococay(void);
void cliffs_of_moher(void);
void amalfi_coast(void);
void mount_vesuvius(void);
void damariscove(void);
void roanoke(void);
void cape_comorin(void);
void andalusia(void);
void key_west(void);
void turks_and_caicos(void);
void seychelles(void);
void ring_of_kerry(void);
void sas_inq_selection_timeout_test(void);
void kerala(void);
void triana(void);
void activate_timer_test(void);
void bugs_bunny_test(void);
void the_phantom(void); 
void tom_and_jerry(void);
void superman(void);
void almance(void);
void sealink(void);
void aansi(void);
void chulbuli(void);
void moksha(void);
void winnie_the_pooh(void);
void agent_oso(void);
void bliskey(void);

extern char * superman_short_desc;
extern char * superman_long_desc;

extern char * almance_short_desc;
extern char * almance_long_desc;

extern char * bugs_bunny_short_desc;
extern char * bugs_bunny_long_desc;

extern char * the_phantom_short_desc;
extern char * the_phantom_long_desc;

extern char * tom_and_jerry_short_desc;
extern char * tom_and_jerry_long_desc;

extern char * chulbuli_short_desc;
extern char * chulbuli_long_desc;

extern char * moksha_short_desc;
extern char * moksha_long_desc;

extern char * winnie_the_pooh_short_desc;
extern char * winnie_the_pooh_long_desc;

extern char * agent_oso_short_desc;
extern char * agent_oso_long_desc;


void sally_struthers_test(void);
fbe_status_t sally_struthers_load_and_verify(void);
extern char * sally_struthers_short_desc;
extern char * sally_struthers_long_desc;

void hand_of_vecna_test(void);
fbe_status_t hand_of_vecna_load_and_verify(void);
extern char * hand_of_vecna_short_desc;
extern char * hand_of_vecna_long_desc;

extern char * serengeti_short_desc;
extern char * serengeti_long_desc;
void serengeti_test_setup(void);
void serengeti_test(void);
void serengeti_cleanup(void);

extern char * super_hv_short_desc;
extern char * super_hv_long_desc;
void super_hv_setup(void);
void super_hv_test(void);
void super_hv_cleanup(void);
/* Dual SP */
void super_hv_dualsp_test(void);
void super_hv_dualsp_setup(void);
void super_hv_dualsp_cleanup(void);

/* Common functions that are called across different configuration (naxos, maui) tests. */
fbe_status_t fbe_zrt_verify_edge_info_of_logged_out_enclosure(fbe_u32_t port, fbe_u32_t encl);
fbe_status_t fbe_zrt_wait_enclosure_status (fbe_u32_t port, fbe_u32_t encl, fbe_u32_t lifecycle_state, fbe_u32_t);


fbe_status_t fbe_test_physical_package_tests_config_unload(void);
void physical_package_test_load_physical_package_and_neit(void);
void physical_package_test_load_physical_package_and_neit_both_sps(void);
void physical_package_test_unload_physical_package_and_neit(void);
void physical_package_test_unload_physical_package_and_neit_both_sps(void);

void sp_static_config_test(void);
//fbe_status_t fbe_test_alloc_slot_map(fbe_u32_t encl_backend_number, fbe_u32_t encl_num,slot_map_t**slot_map_p,fbe_u32_t*max_num_drives);
//void fbe_test_get_drive_state(fbe_u32_t encl_backend_number, fbe_u32_t encl_num,fbe_u32_t max_num_drives,slot_map_t*slot_map_p);
//void fbe_test_get_drv_info(fbe_edal_block_handle_t enclosureControlInfo,slot_map_t*slot_map_p,fbe_u32_t obj_id,fbe_u8_t index);


