#include <stdio.h>
#include "ntddk.h"
//#include "ntddscsi.h"
//#include "storport.h"
//#include "srb.h"
#include "cpd_interface.h"
#include "icdmini.h"
#include "fbe/fbe_types.h"
#include "fbe_cpd_shim_sim.h"




static fbe_status_t fbe_cpd_shim_process_ioctl_irp (PEMCPAL_IRP pIrp, fbe_u32_t port_number);
static fbe_status_t process_scsi_miniport_ioctl (void * buffer, fbe_u32_t port_number);
static fbe_status_t process_scsi_get_capabilities_ioctl (void * buffer, fbe_u32_t port_number);
static fbe_status_t process_scsi_execute_none_ioctl(PEMCPAL_IRP pIrp, fbe_u32_t port_number);
static fbe_status_t process_scsi_inquiry_ioctl (void * buffer, fbe_u32_t port_number);
static fbe_status_t process_get_config_ioctl(CPD_CONFIG *cpd_config, fbe_u32_t port_number);
static fbe_status_t fbe_cpd_shim_process_io_irp (PCPD_SCSI_REQ_BLK Srb,fbe_u32_t port_number);

//	Describe the pseudo-device Inquiry data
#define ProductIdSym8751 'P','r','o','d','u','c','t','I','d','S','y','m','8','5','7','1'
static	UCHAR	Inquiry[36] = 
{
		0x1e,0x00,0x02,0x02,0x0b,0x00,0x00,0x02,
		VendorIdCLARiiON,
		ProductIdSym8751,
		'1','.','0',' '
};

fbe_status_t 
fbe_cpd_shim_execute_irp(fbe_u32_t port_number, const void * irp)
{

	fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	PEMCPAL_IRP 						 		pIrp = NULL;
	PEMCPAL_IO_STACK_LOCATION  				irpStack = NULL;
	fbe_u8_t * 							ioBuffer = NULL;

	pIrp = (PEMCPAL_IRP)irp;

	/*if we used the real irp transport, we would check CUrrent but we did not move
	anything so we just look at Next*/

    irpStack = EmcpalIrpGetNextStackLocation(pIrp);
     
	switch (EmcpalExtIrpStackMajorFunctionGet(irpStack)) {
		case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
			status =  fbe_cpd_shim_process_ioctl_irp (pIrp, port_number);
			break;
	case EMCPAL_IRP_TYPE_CODE_SCSI:
			/*this IO might be carying an IOCTL from CM*/
			if (EmcpalExtIrpStackParamScsiSrbGetFunction(irpStack) == SRB_FUNCTION_IO_CONTROL &&
				EmcpalExtIrpStackParamScsiSrbGet(irpStack)->SrbFlags == SRB_FLAGS_FLARE_CMD) {
				status =  process_scsi_miniport_ioctl (EmcpalExtIrpStackParamScsiSrbGetDataBuffer(irpStack), port_number);
			} else {
				status = fbe_cpd_shim_process_io_irp (EmcpalExtIrpStackParamScsiSrbGet(irpStack), port_number);
			}
			break;
		default:
			status = FBE_STATUS_GENERIC_FAILURE;/*we don't know how to process that yet*/
	}

	if (status != FBE_STATUS_OK && EmcpalExtIrpUserStatusPtrGet(pIrp) != NULL) {
        EmcpalExtIrpUserStatusFieldSet(pIrp, 0xFFFFFFFF);/*taken from FBE_NTSTATUS_FAILURE                   ((DWORD   )0xFFFFFFFFL) */
	}

	return status;
}

static fbe_status_t fbe_cpd_shim_process_ioctl_irp (PEMCPAL_IRP pIrp, fbe_u32_t port_number)
{
	fbe_status_t			status;
	fbe_u32_t 				ioctl_code = 0;
    PEMCPAL_IO_STACK_LOCATION  	irpStack = NULL;

	irpStack = EmcpalIrpGetNextStackLocation(pIrp);

	ioctl_code = EmcpalExtIrpStackParamIoctlCodeGet(irpStack);

    switch (ioctl_code) {
	case IOCTL_SCSI_MINIPORT:
		status = process_scsi_miniport_ioctl (EmcpalExtIrpStackParamScsiSrbGetDataBuffer(irpStack), port_number);
		break;
	case IOCTL_SCSI_GET_CAPABILITIES:
		status = process_scsi_get_capabilities_ioctl (EmcpalExtIrpStackParamType3InputBufferGet(irpStack), port_number);
		break;
	case IOCTL_SCSI_GET_INQUIRY_DATA:
		status = process_scsi_inquiry_ioctl (EmcpalExtIrpStackParamType3InputBufferGet(irpStack), port_number);
		break;
	case IOCTL_SCSI_EXECUTE_NONE:
		status = process_scsi_execute_none_ioctl (pIrp, port_number);
		break;
	default:
		printf("%s: irp ioctl code not defined: %i\n", __FUNCTION__, ioctl_code);
		status = FBE_STATUS_GENERIC_FAILURE;
		break;
	}
	return status;
}

static fbe_status_t process_scsi_miniport_ioctl (void * buffer, fbe_u32_t port_number)
{
	fbe_status_t				status = FBE_STATUS_OK;
	CPD_REGISTER_CALLBACKS *	register_callback = NULL;
	CPD_CM_LOOP_COMMAND	*		cm_ioctl = NULL;
	CPD_IOCTL_HEADER *			ioctl_hdr = NULL;
	
    /*the buffer always have an SRB_IO_CONTROL header and under it the actual ioctl */
    ioctl_hdr = (CPD_IOCTL_HEADER *)buffer;

	switch(CPD_IOCTL_HEADER_GET_OPCODE(ioctl_hdr)){
	case CPD_IOCTL_GET_CONFIG:
		status  = process_get_config_ioctl((CPD_CONFIG *)((fbe_u8_t *)buffer + sizeof(CPD_IOCTL_HEADER)), port_number);
		break;
	case CPD_IOCTL_REGISTER_CALLBACKS:
		register_callback = (CPD_REGISTER_CALLBACKS *)((fbe_u8_t *)buffer + sizeof(CPD_IOCTL_HEADER));
		register_callback->callback_handle = (void *)0x12345678;
		break;
	case CPD_IOCTL_INITIATOR_LOOP_COMMAND:
		/*this is a bit tricky, the data structure we get here in the buffer is NTBE_CM_IOCTL
		 however it is defined deep in NTBE. So we have to cheat and get only the data we really want*/
		cm_ioctl = (CPD_CM_LOOP_COMMAND *)((fbe_u8_t *)buffer + sizeof(CPD_IOCTL_HEADER));
		switch (cm_ioctl->command_type) {
		case CPD_CM_DISCOVER_LOOP:
			cm_ioctl->discover_loop_status = CPD_LOOP_DISCOVERED;
			return FBE_STATUS_OK;
		default:
			printf("%s: cm_ioctl->command_type not defined: %i\n", __FUNCTION__, cm_ioctl->command_type);
			return FBE_STATUS_GENERIC_FAILURE;
		}
	case CPD_IOCTL_RESET_DEVICE:
		/*nothing to do, physical package would do that*/
		break;
	default:
		printf("%s: cpd ioctl code not defined: %i\n", __FUNCTION__, 
			   CPD_IOCTL_HEADER_GET_OPCODE(ioctl_hdr));
		return FBE_STATUS_GENERIC_FAILURE;
	}
	return status;
}

static fbe_status_t process_get_config_ioctl(CPD_CONFIG *cpd_config, fbe_u32_t port_number)
{
    char bus_str[10];
	char start_str[2] = {'B','('};
	char middle_str[3] = {')','I','('};
	char end_str[3] = {')','C',0};
	

	cpd_config->k10_drives = 15;/*just a made up number, this is for display only*/
	cpd_config->available_speeds.speed.fc = 0x1000;/*based on # FC_SPEED_4_GBPS                     0x1000*/

	/*we will set each port to be also the bus number*/
	memcpy (bus_str, start_str, 2);
	bus_str[2] = port_number + 0x30;/*0 in ASCII is 0x30*/
	memcpy (bus_str + 3, middle_str, 3);
	bus_str[6] = port_number + 0x30;/*0 in ASCII is 0x30*/
	memcpy (bus_str + 7, end_str, 3);

	memcpy (cpd_config->pass_thru_buf, bus_str, 10);
    
	return FBE_STATUS_OK;
}

static fbe_status_t process_scsi_get_capabilities_ioctl (void * buffer, fbe_u32_t port_number)
{
	PIO_SCSI_CAPABILITIES port_capabilities = (PIO_SCSI_CAPABILITIES)buffer;

	port_capabilities->TaggedQueuing = 0;


	return FBE_STATUS_OK;
}

static fbe_status_t process_scsi_inquiry_ioctl (void * buffer, fbe_u32_t port_number)
{
	fbe_u32_t			bus = 0;
	fbe_u32_t			target = 0;
	fbe_u32_t			path = 0;

	PSCSI_ADAPTER_BUS_INFO 		adapter_bus_info = (PSCSI_ADAPTER_BUS_INFO)buffer;
	PSCSI_INQUIRY_DATA			scsi_inq_data = NULL;
	PINQUIRYDATA				inq_data = NULL;


	adapter_bus_info->NumberOfBuses = 4;

	for (bus = 0; bus < adapter_bus_info->NumberOfBuses; bus ++) {
		adapter_bus_info->BusData[bus].NumberOfLogicalUnits = 15;
		/*calculation here taken from INQUIRY_DATA_SIZE in ntbe_class.c
		#define INQUIRY_DATA_BUFFER_SIZE is 40 there even though the inq_size is defined as 36 bytes
		someone decided to have some spare left

		The 4 is for 4 paths, each with 32 devices which in total give 128 devices per port
		The size of each device is the device data (SCSI_INQUIRY_DATA) + 40 bytes for inqury data for this device (PSCSI_INQUIRY_DATA)
		*/
		adapter_bus_info->BusData[bus].InquiryDataOffset = sizeof(SCSI_ADAPTER_BUS_INFO) + 
														   (79) * sizeof(SCSI_BUS_DATA) +
														   ((sizeof (SCSI_INQUIRY_DATA) + 40) * (bus * 4 * 32));

		for (path = 0; path < 4; path ++) {

			for (target = 0; target < 32; target ++) {

				/*we can have only 126 devices, not 127, so the last one we just drop
				there is code in ntbeshim that checks we are less than MAX_FIBRE_DISKS_PER_BUS*/
				if (target == 31 && path == 3) {
					break;
				}
			
				scsi_inq_data = (PSCSI_INQUIRY_DATA)((fbe_u8_t *)buffer + 
													  adapter_bus_info->BusData[bus].InquiryDataOffset +
													  ((target + (path * 32)) * (36 + sizeof(SCSI_INQUIRY_DATA))) );
		
				scsi_inq_data->PathId = path;
				scsi_inq_data->TargetId = target;
				scsi_inq_data->Lun = 0;
				scsi_inq_data->InquiryDataLength = 0; /*not used*/
				
				/*copy inqury data*/
				inq_data =(PINQUIRYDATA) &scsi_inq_data->InquiryData;
				memcpy (inq_data, Inquiry, 36);
		
				scsi_inq_data->NextInquiryDataOffset = (adapter_bus_info->BusData[bus].InquiryDataOffset +
													  ((target + 1 + (path * 32)) * (36 + sizeof(SCSI_INQUIRY_DATA))) );
			} 
		}

		scsi_inq_data->NextInquiryDataOffset = 0;/*overwrite for the last one to indicate we are done*/
	}


	return FBE_STATUS_OK;
}

static fbe_status_t fbe_cpd_shim_process_io_irp (PCPD_SCSI_REQ_BLK Srb,fbe_u32_t port_number)
{
	fbe_status_t				status;
	fbe_io_block_t	   			fbe_io_block;
	CPD_EXT_SRB *				ext_srb = (CPD_EXT_SRB *)Srb;
	PNTBE_SGL					sg_list = (PNTBE_SGL)ext_srb->sgl;
   
    /*extract the data from the srb into the io block*/
	fbe_io_block.sg_list = (fbe_sg_element_t *)sg_list;

	fbe_io_block.io_block_opcode = FBE_IO_BLOCK_OPCODE_EXECUTE_CDB;
	fbe_io_block.timeout = cpd_os_io_get_native_time_out(Srb);

	memcpy(fbe_io_block.operation.execute_cdb.cdb, cpd_os_io_get_cdb(Srb), FBE_IO_BLOCK_CDB_SIZE);
	fbe_io_block.operation.execute_cdb.cdb_length = cpd_os_io_get_cdb_length(Srb);
	fbe_io_block.operation.execute_cdb.port_number = port_number;
	fbe_io_block.operation.execute_cdb.io_block_task_attribute = cpd_os_io_get_queue_action(Srb);
	fbe_io_block.operation.execute_cdb.io_block_flags = cpd_os_io_get_flags(Srb);
	fbe_io_block.operation.execute_cdb.cpd_device_id = (cpd_os_io_get_target_id(Srb) + (cpd_os_io_get_path_id(Srb) * 32));
	fbe_io_block.operation.execute_cdb.cdb_length = cpd_os_io_get_cdb_length(Srb);
	fbe_io_block.operation.execute_cdb.block_size = 520;/*HACK for now*/
    
	status = fbe_cpd_shim_execute_cdb(port_number, &fbe_io_block);

	/*now we copy the appropriate data to the srb*/
	if (cpd_os_io_get_sense_buf(Srb) != NULL) {
		memcpy(cpd_os_io_get_sense_buf(Srb), fbe_io_block.operation.execute_cdb.sense_info_buffer, cpd_os_io_get_sense_len(Srb));
	}

	cpd_os_io_set_queue_tag(Srb, 0);
	cpd_os_io_set_scsi_status(Srb, fbe_io_block.operation.execute_cdb.io_block_scsi_status);
	cpd_os_io_set_status(Srb,  fbe_io_block.operation.execute_cdb.io_block_request_status);

	return status;
}


static fbe_status_t process_scsi_execute_none_ioctl(PEMCPAL_IRP pIrp, fbe_u32_t port_number)
{
	PEMCPAL_IO_STACK_LOCATION  		irpStack = NULL;

	irpStack = EmcpalIrpGetNextStackLocation (pIrp);
	if (EmcpalExtIrpStackParamScsiSrbGetFunction(irpStack) == SRB_FUNCTION_RELEASE_DEVICE ||
		EmcpalExtIrpStackParamScsiSrbGetFunction(irpStack) == SRB_FUNCTION_CLAIM_DEVICE){

		/*we do nothing in simulation....*/

		return FBE_STATUS_OK;
	}
	
    /*if it's not one of these we currently do not support that*/
	return FBE_STATUS_GENERIC_FAILURE;
}


