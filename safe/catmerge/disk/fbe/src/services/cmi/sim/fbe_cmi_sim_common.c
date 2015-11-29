/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_sim_client.c
 ***************************************************************************
 *
 *  Description
 *      client implementation for CMI simulation service
 *      
 *    
 ***************************************************************************/
#include "fbe_cmi.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_terminator_api.h"

typedef struct conduit_to_port_s{
	fbe_cmi_conduit_id_t conduit;
	fbe_u8_t port_str[16];
}conduit_to_port_t;

conduit_to_port_t		conduit_to_port_map_spa[] = 
{
	{FBE_CMI_CONDUIT_ID_TOPLOGY, "20050"},
    {FBE_CMI_CONDUIT_ID_ESP,	"20052"},
    {FBE_CMI_CONDUIT_ID_NEIT,	"20054"},
	#if 0
	{FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE,	"20056"},
	{FBE_CMI_CONDUIT_ID_CMS_TAGS,	"20058"},
	#endif
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE0,	"20060"},
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE1,	"20062"},
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE2,	"20064"},
	{FBE_CMI_CONDUIT_ID_LAST ,""}
	
};

conduit_to_port_t		conduit_to_port_map_spb[] = 
{
	{FBE_CMI_CONDUIT_ID_TOPLOGY, "20051"},
	{FBE_CMI_CONDUIT_ID_ESP,	"20053"},
	{FBE_CMI_CONDUIT_ID_NEIT,	"20055"},
	#if 0
	{FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE,	"20057"},
	{FBE_CMI_CONDUIT_ID_CMS_TAGS,	"20059"},
	#endif
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE0,	"20061"},
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE1,	"20063"},
	{FBE_CMI_CONDUIT_ID_SEP_IO_CORE2,	"20065"},
	{FBE_CMI_CONDUIT_ID_LAST ,""}
	
};

static fbe_terminator_api_get_cmi_port_base_function_t terminator_api_get_cmi_port_base_function = NULL;

fbe_status_t fbe_cmi_sim_map_conduit_to_client_port(fbe_cmi_conduit_id_t conduit_id, const fbe_u8_t **port_string, fbe_cmi_sp_id_t sp_id)
{
	conduit_to_port_t *	table_ptr = NULL;

	table_ptr = ((sp_id == FBE_CMI_SP_ID_A) ? conduit_to_port_map_spa : conduit_to_port_map_spb);

	while (table_ptr->conduit != FBE_CMI_CONDUIT_ID_LAST) {
		if (conduit_id == table_ptr->conduit) {
			*port_string = table_ptr->port_str;
			return FBE_STATUS_OK;
		}

		table_ptr ++;
	}

	return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_cmi_sim_map_conduit_to_server_port(fbe_cmi_conduit_id_t conduit_id, const fbe_u8_t **port_string, fbe_cmi_sp_id_t sp_id)
{
	conduit_to_port_t *	table_ptr = NULL;

	table_ptr = ((sp_id == FBE_CMI_SP_ID_B) ? conduit_to_port_map_spa : conduit_to_port_map_spb);

	while (table_ptr->conduit != FBE_CMI_CONDUIT_ID_LAST) {
		if (conduit_id == table_ptr->conduit) {
			*port_string = table_ptr->port_str;
			return FBE_STATUS_OK;
		}

		table_ptr ++;
	}

	return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_cmi_sim_set_get_cmi_port_base_func(fbe_terminator_api_get_cmi_port_base_function_t function)
{
    terminator_api_get_cmi_port_base_function = function;
    return FBE_STATUS_OK;
}

// If Terminator has changed the cmi port base, update all ports. Always return OK.
fbe_status_t fbe_cmi_sim_update_cmi_port(void)
{
    fbe_u16_t port_base = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    size_t cnt = 0;

    if(terminator_api_get_cmi_port_base_function != NULL)
    {
        status = terminator_api_get_cmi_port_base_function(&port_base);
        if(status != FBE_STATUS_OK)
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s fail to update cmi port from Terminator.\n", __FUNCTION__);
            return FBE_STATUS_OK;
        }
        /* port_base is 0 means user does not set */
        if(port_base > 0)
        {
            for(cnt = 0; cnt < sizeof(conduit_to_port_map_spa) / sizeof(conduit_to_port_t)
                            && cnt < sizeof(conduit_to_port_map_spb) / sizeof(conduit_to_port_t); ++cnt)
            {
                if(conduit_to_port_map_spa[cnt].conduit > FBE_CMI_CONDUIT_ID_INVALID
                        && conduit_to_port_map_spa[cnt].conduit < FBE_CMI_CONDUIT_ID_LAST)
                {
                    itoa(port_base++, conduit_to_port_map_spa[cnt].port_str, 10);
                    itoa(port_base++, conduit_to_port_map_spb[cnt].port_str, 10);
                }
            }
        }
    }
    return FBE_STATUS_OK;
}
