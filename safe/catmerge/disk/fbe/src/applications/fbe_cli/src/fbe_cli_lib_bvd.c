/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_bvd.c
 ***************************************************************************
 *
 * @brief This file is used to parse BVD related commands
 *
 * @ingroup fbe_cli
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe_cli_bvd.h"


/*local functions*/
static void fbe_cli_enable_perf_stat(void)
{
	fbe_status_t status = fbe_api_bvd_interface_enable_peformance_statistics();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed to enable stats\n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Stats gathering enabled\n", __FUNCTION__);
	}
}

static void fbe_cli_disable_perf_stat(void)
{
	fbe_status_t status = fbe_api_bvd_interface_disable_peformance_statistics();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed to disable stats", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Stats gathering disabled\n", __FUNCTION__);
	}

}

static void fbe_cli_clear_perf_stat(void)
{
	fbe_status_t status = fbe_api_bvd_interface_clear_peformance_statistics();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed clear stats\n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Stats cleared.\n", __FUNCTION__);
	}

}

static void fbe_cli_print_perf_stat(void)
{
	fbe_sep_shim_get_perf_stat_t	bvd_perf_stats;
	fbe_status_t					status;
	fbe_cpu_id_t					core_count = fbe_get_cpu_count();
	fbe_cpu_id_t					current_core;
	fbe_u32_t j;

	status = fbe_api_bvd_interface_get_peformance_statistics(&bvd_perf_stats);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed to get stats form BVD\n", __FUNCTION__);
		return;
	}

	for (current_core = 0; current_core < core_count; current_core++) {

		fbe_cli_printf("\nCore %d stats\n===================\n", current_core);

		if (bvd_perf_stats.core_stat[current_core].current_ios_in_progress == 0 &&
			bvd_perf_stats.core_stat[current_core].max_io_q_depth == 0 &&
			bvd_perf_stats.core_stat[current_core].total_ios == 0) {

			fbe_cli_printf("No IO activity on core %d\n", current_core);
			continue;
		}
		
		fbe_cli_printf("Current IOs in flight:0x%llX\n", (unsigned long long)bvd_perf_stats.core_stat[current_core].current_ios_in_progress);
		fbe_cli_printf("Max concurrent IOs:0x%llX\n", (unsigned long long)bvd_perf_stats.core_stat[current_core].max_io_q_depth);
		fbe_cli_printf("Total IOs since last stat reset:0x%llX\n", (unsigned long long)bvd_perf_stats.core_stat[current_core].total_ios);
	}

	fbe_cli_printf("\n");

	for (current_core = 0; current_core < core_count; current_core++) {
		fbe_cli_printf("RunQ Core %d: depth %d max %d \n",  current_core,
															bvd_perf_stats.core_stat[current_core].runq_stats.transport_run_queue_current_depth,
															bvd_perf_stats.core_stat[current_core].runq_stats.transport_run_queue_max_depth);
	}

	for (current_core = 0; current_core < core_count; current_core++) {
		fbe_cli_printf("\nRunQ HIST Core %d: ",  current_core);

		for(j = 0; j < FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE; j++){
			if(!(j%8)) {
				fbe_cli_printf("\n");
			}
			fbe_cli_printf(" %lld ", (long long)bvd_perf_stats.core_stat[current_core].runq_stats.transport_run_queue_hist_depth[j]);
		}
	}

	fbe_cli_printf("\n");
}

static void fbe_cli_enable_async_io(void)
{
	fbe_status_t status = fbe_api_bvd_interface_enable_async_io();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Async mode set\n", __FUNCTION__);
	}
}

static void fbe_cli_disable_async_io(void)
{
	fbe_status_t status = fbe_api_bvd_interface_disable_async_io();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Sync mode set\n", __FUNCTION__);
	}
}

static void fbe_cli_enable_async_io_compl(void)
{
	fbe_status_t status = fbe_api_bvd_interface_enable_async_io_compl();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Async completion mode set\n", __FUNCTION__);
	}
}

static void fbe_cli_disable_async_io_compl(void)
{
	fbe_status_t status = fbe_api_bvd_interface_disable_async_io_compl();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Sync completion mode set\n", __FUNCTION__);
	}
}

static void fbe_cli_set_rq_method(fbe_u32_t rq_method)
{
	fbe_status_t status = fbe_api_bvd_interface_set_rq_method(rq_method);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("rq_method set to %d\n",  rq_method);
	}
}

static void fbe_cli_set_alert_time(fbe_u32_t alert_time)
{
	fbe_status_t status = fbe_api_bvd_interface_set_alert_time(alert_time);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("alert_time set to %d\n",  alert_time);
	}
}

static void fbe_cli_enable_group_priority(void)
{
	fbe_status_t status = fbe_api_bvd_interface_enable_group_priority(FALSE);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Run queue group priority mode enabled\n", __FUNCTION__);
	}
}

static void fbe_cli_disable_group_priority(void)
{
	fbe_status_t status = fbe_api_bvd_interface_disable_group_priority(FALSE);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Run queue group priority mode disabled\n", __FUNCTION__);
	}
}

static void fbe_cli_enable_pp_group_priority(void)
{
	fbe_status_t status = fbe_api_bvd_interface_enable_group_priority(TRUE);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s:PP Run queue group priority mode enabled\n", __FUNCTION__);
	}
}

static void fbe_cli_disable_pp_group_priority(void)
{
	fbe_status_t status = fbe_api_bvd_interface_disable_group_priority(TRUE);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s:PP Run queue group priority mode disabled\n", __FUNCTION__);
	}
}

static void fbe_cli_shutdown(void)
{
	fbe_status_t status = fbe_api_bvd_interface_shutdown();
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s: Failed \n", __FUNCTION__);
	}else{
		fbe_cli_printf("%s: Shutdown command initiated successfully.\n", __FUNCTION__);
	}
}

void fbe_cli_bvd_commands(int argc, char** argv)
{
	if (argc < 1){
        /*If there are no arguments show usage*/
		fbe_cli_printf("%s", FBE_CLI_BVD_USAGE);
		return;
    }

    
	if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)){
		/*Print bvd usage*/
		fbe_cli_printf("%s", FBE_CLI_BVD_USAGE);
		return;
	}
    
	if ((strcmp(*argv, "-get_stat") == 0)){
		fbe_cli_print_perf_stat();
		return;
	}

	if ((strcmp(*argv, "-enable_stat") == 0)){
		fbe_cli_enable_perf_stat();
		return;
	}

	if ((strcmp(*argv, "-disable_stat") == 0)){
		fbe_cli_disable_perf_stat();
		return;
	}

	if ((strcmp(*argv, "-clear_stat") == 0)){
		fbe_cli_clear_perf_stat();
		return;
	}

	if ((strcmp(*argv, "-set_async_io") == 0)){
		fbe_cli_enable_async_io();
		return;
	}

	if ((strcmp(*argv, "-set_sync_io") == 0)){
		fbe_cli_disable_async_io();
		return;
	}

	if ((strcmp(*argv, "-set_async_compl") == 0)){
		fbe_cli_enable_async_io_compl();
		return;
	}

	if ((strcmp(*argv, "-set_sync_compl") == 0)){
		fbe_cli_disable_async_io_compl();
		return;
	}

	if ((strcmp(*argv, "-enable_gp") == 0)){
		fbe_cli_enable_group_priority();
		return;
	}

	if ((strcmp(*argv, "-disable_gp") == 0)){
		fbe_cli_disable_group_priority();
		return;
	}

	if ((strcmp(*argv, "-enable_pp_gp") == 0)){
		fbe_cli_enable_pp_group_priority();
		return;
	}

	if ((strcmp(*argv, "-disable_pp_gp") == 0)){
		fbe_cli_disable_pp_group_priority();
		return;
	}

	if ((strcmp(*argv, "-set_rq_method") == 0)){
		fbe_u32_t rq_method = 0;
		argv++;
		argc--;
		if(argc > 0){
			rq_method = atoi(*argv);
			fbe_cli_set_rq_method(rq_method);
			return;
		}
		fbe_cli_printf("%s", FBE_CLI_BVD_USAGE);
		return;
	}

	if ((strcmp(*argv, "-set_alert_time") == 0)){
		fbe_u32_t alert_time = 0;
		argv++;
		argc--;
		if(argc > 0){
			alert_time = atoi(*argv);
			fbe_cli_set_alert_time(alert_time);
			return;
		}
		fbe_cli_printf("%s", FBE_CLI_BVD_USAGE);
		return;
	}

    if ((strcmp(*argv, "-shutdown") == 0)) {
        fbe_cli_shutdown();
        return;
    }
	fbe_cli_printf("%s", FBE_CLI_BVD_USAGE);
	return;
}

