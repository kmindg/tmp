#include "physical_package_tests.h"
#include "mut.h"
#include "fbe_test_common_utils.h"

mut_testsuite_t * fbe_test_create_physical_package_test_suite(mut_function_t startup, mut_function_t teardown) {
    mut_testsuite_t *physical_package_test_suite = NULL;

    if (fbe_test_get_default_extended_test_level() == 0)
    {
        physical_package_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("physical_package_test_suite", startup, teardown);
    }
    else
    {
        physical_package_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("extended_physical_package_test_suite", startup, teardown);
    }
    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, static_config_test, 
                                  NULL, NULL,
                                  static_config_short_desc, static_config_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, maui,
                                  NULL, NULL,
                                  maui_short_desc, maui_long_desc) /*SAFEBUG*//*RCA-NPTF*/
    MUT_ADD_TEST(physical_package_test_suite, naxos, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, lapa_rios, NULL, NULL)
//    MUT_ADD_TEST(physical_package_test_suite, sobo_4, NULL, fbe_test_physical_package_tests_config_unload)
    MUT_ADD_TEST(physical_package_test_suite, home, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, grylls, NULL, NULL)
    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, petes_coffee_and_tea,
                                  NULL, NULL,
                                  petes_coffee_and_tea_short_desc, petes_coffee_and_tea_long_desc)
    MUT_ADD_TEST(physical_package_test_suite, zhiqis_coffee_and_tea, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, tiruvalla, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, republica_dominicana, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, trichirapalli, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, chautauqua, NULL, NULL)
//    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, los_vilos, los_vilos_load_and_verify, los_vilos_unload,
//                                  los_vilos_short_desc, los_vilos_long_desc)

//    MUT_ADD_TEST(physical_package_test_suite, mont_tremblant, mont_tremblant_load_and_verify, los_vilos_unload)

    MUT_ADD_TEST(physical_package_test_suite, trivandrum, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, kamphaengphet, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, vai, NULL, NULL) 
//    MUT_ADD_TEST(physical_package_test_suite, cococay, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, cliffs_of_moher, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, amalfi_coast, NULL, NULL)
    //MUT_ADD_TEST(physical_package_test_suite, sobo_4_sata, cococay_load_and_verify, fbe_test_physical_package_tests_config_unload)
    MUT_ADD_TEST(physical_package_test_suite, mount_vesuvius, NULL, NULL)
//    MUT_ADD_TEST(physical_package_test_suite, damariscove, los_vilos_load_and_verify, los_vilos_unload)
    MUT_ADD_TEST(physical_package_test_suite, roanoke, NULL, NULL)

    //ARS-384577: ! @todo: convert fbe_test_io to rdgen:  
    //MUT_ADD_TEST(physical_package_test_suite, los_vilos_sata, los_vilos_sata_load_and_verify, fbe_test_physical_package_tests_config_unload)

    MUT_ADD_TEST(physical_package_test_suite, cape_comorin, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, andalusia, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, turks_and_caicos, NULL, NULL)

    MUT_ADD_TEST(physical_package_test_suite, key_west, NULL, NULL)

    MUT_ADD_TEST(physical_package_test_suite, wallis_pond, wallis_pond_setup, fbe_test_physical_package_tests_config_unload)

    MUT_ADD_TEST(physical_package_test_suite, seychelles, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, ring_of_kerry, NULL, NULL)
    MUT_ADD_TEST(physical_package_test_suite, sealink, NULL, fbe_test_physical_package_tests_config_unload) 
    MUT_ADD_TEST(physical_package_test_suite, aansi, NULL, fbe_test_physical_package_tests_config_unload)
    MUT_ADD_TEST(physical_package_test_suite, sas_inq_selection_timeout_test, NULL, fbe_test_physical_package_tests_config_unload)
    MUT_ADD_TEST(physical_package_test_suite, superman, yancey_load_and_verify, fbe_test_physical_package_tests_config_unload)
    MUT_ADD_TEST(physical_package_test_suite, almance, yancey_load_and_verify, fbe_test_physical_package_tests_config_unload)
//  MUT_ADD_TEST(physical_package_test_suite, bliskey, los_vilos_load_and_verify, los_vilos_unload)

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, bugs_bunny_test, 
        maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
        bugs_bunny_short_desc, bugs_bunny_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, the_phantom, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            the_phantom_short_desc, the_phantom_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, tom_and_jerry, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            tom_and_jerry_short_desc, tom_and_jerry_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, chulbuli, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            chulbuli_short_desc, chulbuli_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, moksha, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            moksha_short_desc, moksha_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, winnie_the_pooh, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            winnie_the_pooh_short_desc, winnie_the_pooh_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, agent_oso, 
            maui_load_single_configuration, fbe_test_physical_package_tests_config_unload,
            agent_oso_short_desc, agent_oso_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, sally_struthers_test,
                                  sally_struthers_load_and_verify, fbe_test_physical_package_tests_config_unload,
                                  sally_struthers_short_desc, sally_struthers_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, denali_dest_injection, denali_load_and_verify, denali_unload,
                                  denali_short_desc, denali_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, serengeti_test, serengeti_test_setup, serengeti_cleanup,
                                  serengeti_short_desc, serengeti_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(physical_package_test_suite, hand_of_vecna_test, hand_of_vecna_load_and_verify,  fbe_test_physical_package_tests_config_unload,
                                  hand_of_vecna_short_desc, hand_of_vecna_long_desc)

    return physical_package_test_suite;
}

mut_testsuite_t * fbe_test_create_physical_package_dualsp_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * physical_package_dualsp_test_suite = NULL;

    if (fbe_test_get_default_extended_test_level() == 0)
    {
        physical_package_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("physical_package_dualsp_test_suite", startup, teardown);
    }
    else
    {
        physical_package_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("extended_physical_package_dualsp_test_suite", startup, teardown);
    }

    return physical_package_dualsp_test_suite;
}
