/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sep_env_limits_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main function for the executible that will display
 *  environmental limits for sep.
 *
 * @version
 *   2/4/2013 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "core_config_runtime.h"
#include "fbe/fbe_environment_limit.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_sep.h"
#include "fbe_scheduler.h"
#include "fbe_metadata.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void display_separator(void)
{
    printf("------------------------------------------------------------------------------\n");
}
/*!**************************************************************
 * fbe_sep_display_env_limits()
 ****************************************************************
 * @brief
 *  Display sep limits.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/4/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_sep_display_env_limits(SPID_HW_TYPE platform_type)
{
    fbe_bool_t status;
    fbe_environment_limits_t env_limits;
    fbe_u32_t platform_max_memory_mb;
    fbe_u32_t orig_platform_max_memory_mb;
    fbe_u32_t sep_number_of_chunks;
    fbe_memory_dps_init_parameters_t dps_params;
    fbe_u32_t metadata_memory_mb;
    fbe_u32_t scheduler_memory_mb;
    fbe_u32_t core_count;
    fbe_u32_t non_dps_mb;
    fbe_u32_t total_sep_mb;

    status = ccrInitCoreConfig(platform_type);
    if (status != FBE_TRUE)
    {
        printf("\nstatus is: %d\n", status);
    }
    printf("\n\n");
    display_separator();
    switch (platform_type)
    {
        case SPID_ENTERPRISE_HW_TYPE:
            printf("platform type is: SPID_ENTERPRISE_HW_TYPE (%d)\n", platform_type);
            core_count = 24;
            break;
        case SPID_PROMETHEUS_M1_HW_TYPE:
            printf("platform type is: SPID_PROMETHEUS_M1_HW_TYPE (%d)\n", platform_type);
            core_count = 16;
            break;
        case SPID_DEFIANT_M1_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_M1_HW_TYPE (%d)\n", platform_type);
            core_count = 8;
            break;
        case SPID_DEFIANT_M2_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_M2_HW_TYPE (%d)\n", platform_type);
            core_count = 6;
            break;
        case SPID_DEFIANT_M3_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_M3_HW_TYPE (%d)\n", platform_type);
            core_count = 4;
            break;
        case SPID_DEFIANT_M4_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_M4_HW_TYPE (%d)\n", platform_type);
            core_count = 4;
            break;
        case SPID_DEFIANT_M5_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_M5_HW_TYPE (%d)\n", platform_type);
            core_count = 4;
            break;
        case SPID_DEFIANT_K2_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_K2_HW_TYPE (%d)\n", platform_type);
            core_count = 8;
            break;
        case SPID_DEFIANT_K3_HW_TYPE:
            printf("platform type is: SPID_DEFIANT_K3_HW_TYPE (%d)\n", platform_type);
            core_count = 6;
            break;
// ENCL_CLEANUP - real core counts
        case SPID_OBERON_1_HW_TYPE:
            printf("platform type is: SPID_OBERON_1_HW_TYPE (%d)\n", platform_type);
            core_count = 6;
            break;
        case SPID_OBERON_2_HW_TYPE:
            printf("platform type is: SPID_OBERON_2_HW_TYPE (%d)\n", platform_type);
            core_count = 6;
            break;
        case SPID_OBERON_3_HW_TYPE:
            printf("platform type is: SPID_OBERON_3_HW_TYPE (%d)\n", platform_type);
            core_count = 8;
            break;
        case SPID_OBERON_4_HW_TYPE:
            printf("platform type is: SPID_OBERON_4_HW_TYPE (%d)\n", platform_type);
            core_count = 10;
            break;
        case SPID_HYPERION_1_HW_TYPE:
            printf("platform type is: SPID_HYPERION_1_HW_TYPE (%d)\n", platform_type);
            core_count = 12;
            break;
        case SPID_HYPERION_2_HW_TYPE:
            printf("platform type is: SPID_HYPERION_2_HW_TYPE (%d)\n", platform_type);
            core_count = 16;
            break;
        case SPID_HYPERION_3_HW_TYPE:
            printf("platform type is: SPID_HYPERION_3_HW_TYPE (%d)\n", platform_type);
            core_count = 24;
            break;
        case SPID_TRITON_1_HW_TYPE:
            printf("platform type is: SPID_TRITON_1_HW_TYPE (%d)\n", platform_type);
            core_count = 24;
            break;
        default:
            printf("platform unknown (%d)\n", platform_type);
            core_count = 8;
            break;
    };
    display_separator();
    fbe_sep_init_set_simulated_core_count(core_count);
    env_limits.platform_fru_count = pCoreConfigRuntime_Global->PlatformFruCount;
    env_limits.platform_max_rg = pCoreConfigRuntime_Global->RgUserCount;
    env_limits.platform_max_user_lun = pCoreConfigRuntime_Global->PlatformHostAccessibleFlareLuns;
    fbe_environment_limit_get_max_memory_mb(platform_type,
                                            &orig_platform_max_memory_mb);

    platform_max_memory_mb = orig_platform_max_memory_mb;
    sep_get_memory_parameters(&sep_number_of_chunks, &platform_max_memory_mb, &env_limits);
    sep_get_dps_memory_params(&dps_params);

    display_separator();
    printf("Core Count:                    %d\n", core_count);
    printf("Max FRUs:                      %d\n", env_limits.platform_fru_count);
    printf("Max RAID Groups:               %d\n", env_limits.platform_max_rg);
    printf("Max User LUNs:                 %d\n", env_limits.platform_max_user_lun);
    printf("Max Backends:                  %d\n", pCoreConfigRuntime_Global->PlatformMaxBeCount);
    printf("Front End Ports:               %d\n", pCoreConfigRuntime_Global->FEPortCountPerSP);
    printf("Max FCT FE Requests:           %d\n", pCoreConfigRuntime_Global->MaxPlatformFCTInternalBeRequests);
    printf("Max MCC FE Requests:           %d\n", pCoreConfigRuntime_Global->MaxPlatformPerCoreMCCBeRequests);
    printf("Max MCR FE Requests:           %d\n", pCoreConfigRuntime_Global->MaxPlatformPerCoreMCRFeRequests);
 #ifdef FBE_KH_MISC_DEPEND
    printf("Max Write Cache Size Tier A:   %d\n", pCoreConfigRuntime_Global->PlatformMaxWcSizeTierA);
    printf("Max Write Cache Size Tier B:   %d\n", pCoreConfigRuntime_Global->PlatformMaxWcSizeTierB);
    printf("Max Write Cache Size Tier C:   %d\n", pCoreConfigRuntime_Global->PlatformMaxWcSizeTierC);
#endif
    display_separator();
    printf("number_of_main_chunks (DPS):         %d\n", dps_params.number_of_main_chunks);
    display_separator();
    printf("packet_number_of_chunks:             %d\n", dps_params.packet_number_of_chunks);
    printf("block_1_number_of_chunks:            %d\n", dps_params.block_1_number_of_chunks);
    printf("block_64_number_of_chunks:           %d\n", dps_params.block_64_number_of_chunks);
    display_separator();
    printf("fast_packet_number_of_chunks:        %d\n", dps_params.fast_packet_number_of_chunks);
    printf("fast_block_1_number_of_chunks:       %d\n", dps_params.fast_block_1_number_of_chunks);
    printf("fast_block_64_number_of_chunks:      %d\n", dps_params.fast_block_64_number_of_chunks);
    display_separator();
    printf("reserved_packet_number_of_chunks:    %d\n", dps_params.reserved_packet_number_of_chunks);
    printf("reserved_block_1_number_of_chunks:   %d\n", dps_params.reserved_block_1_number_of_chunks);
    printf("reserved_block_64_number_of_chunks:  %d\n", dps_params.reserved_block_64_number_of_chunks);
    display_separator();
    printf("platform max memory mb:              %d\n", orig_platform_max_memory_mb);
    printf("platform max memory dps mb:          %d\n", platform_max_memory_mb);
    //printf("object memory mb:                    %d\n", orig_platform_max_memory_mb - platform_max_memory_mb + 1);
    display_separator();
    fbe_memory_get_mem_mb_for_chunk_count(sep_number_of_chunks, &non_dps_mb);
    printf("sep memory svc (NON-DPS)   mb:       %d\n", non_dps_mb);
    fbe_metadata_get_control_mem_use_mb(&metadata_memory_mb); 
    printf("metadata service memory mb:          %d\n", metadata_memory_mb);
    fbe_scheduler_get_control_mem_use_mb(&scheduler_memory_mb); 
    printf("scheduler service memory mb:         %d\n", scheduler_memory_mb);
    display_separator();
    printf("total non-dps use mb:                %d\n", non_dps_mb + metadata_memory_mb + scheduler_memory_mb);
    display_separator();
    total_sep_mb = dps_params.number_of_main_chunks + non_dps_mb + metadata_memory_mb + scheduler_memory_mb;
    printf("total sep control use mb:            %d\n", total_sep_mb);

    ccrUnInit();
}
/******************************************
 * end fbe_sep_display_env_limits()
 ******************************************/

/*!**************************************************************
 * main()
 ****************************************************************
 * @brief
 *  This is the main entry point of the sep env limits display.
 *
 * @param argc - argument count
 * @argv  argv - argument array
 *
 * @return int 0   
 *
 * @author
 *  2/1/2013 - Created. Rob Foley
 *
 ****************************************************************/

int __cdecl main (int argc , char ** argv)
{
#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    fbe_sep_display_env_limits(SPID_ENTERPRISE_HW_TYPE);

    fbe_sep_display_env_limits(SPID_PROMETHEUS_M1_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_M1_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_M2_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_M3_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_M4_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_M5_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_K2_HW_TYPE);

    fbe_sep_display_env_limits(SPID_DEFIANT_K3_HW_TYPE);

    fbe_sep_display_env_limits(SPID_OBERON_1_HW_TYPE);

    fbe_sep_display_env_limits(SPID_OBERON_2_HW_TYPE);
    
    fbe_sep_display_env_limits(SPID_OBERON_3_HW_TYPE);

    fbe_sep_display_env_limits(SPID_OBERON_4_HW_TYPE);

    fbe_sep_display_env_limits(SPID_HYPERION_1_HW_TYPE);

    fbe_sep_display_env_limits(SPID_HYPERION_2_HW_TYPE);
    
    fbe_sep_display_env_limits(SPID_HYPERION_3_HW_TYPE);

    fbe_sep_display_env_limits(SPID_TRITON_1_HW_TYPE);

    return 0;
}
/**************************************
 * end main()
 **************************************/

/*************************
 * end file fbe_sep_env_limits_main.c
 *************************/
