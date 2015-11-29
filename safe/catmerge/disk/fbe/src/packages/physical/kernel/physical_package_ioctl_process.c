#include "ntddk.h"
#include "ktrace.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "physical_package_kernel_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_topology.h"
//#include "srb.h"

//#include "storport.h"
#include "scsiclass.h"
//#include "ntddscsi.h"
#include "cpd_interface.h" /* CPD_CONFIG definition, it also includes icdmini.h for NTBE_SRB and NTBE_SGL */

#include "fbe_base_package.h"

#include "fbe_fibre_cpd_shim.h"

/***************************************************************************
 *  physical_package_notification.c
 ***************************************************************************
 *
 *  Description
 *      manage notifications for user space
 *      
 *
 *  History:
 *      10/16/07	sharel	Created
 *    
 ***************************************************************************/

typedef struct io_irp_context_s {
	PEMCPAL_IRP 				PIrp;
	ULONG 				outLength;
	fbe_packet_t *		user_packet;
	fbe_bool_t			io_irp;
	CPD_EXT_SRB *		ext_srb;
}io_irp_context_t;


static fbe_status_t physical_package_io_ioctl_completion_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t populate_ioctl_irp (PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer);
static fbe_status_t populate_io_irp (PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer, fbe_packet_t *io_packet);
static fbe_bool_t is_scsi_miniport_ioctl (PEMCPAL_IRP pIrp);
static fbe_status_t populate_ioctl_via_srb(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer);
static fbe_status_t populate_io_srb(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer);
static fbe_status_t fix_sg_list_pointers(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer);

#if 0
EMCPAL_STATUS physical_package_process_io_packet_ioctl(PEMCPAL_IRP PIrp)
{
	PEMCPAL_IO_STACK_LOCATION  	irpStack;
	PEMCPAL_IO_STACK_LOCATION  	user_irpStack;
	fbe_u8_t * 				ioBuffer = NULL;
    ULONG 					outLength = 0;
	fbe_packet_t * 			user_packet = NULL;
	fbe_u8_t * 				user_buffer = NULL;
    fbe_packet_t * 			io_packet = NULL;
	io_irp_context_t *		irp_context = NULL;
	fbe_io_block_t *		user_io_block = NULL;
	fbe_io_block_t *		new_io_block = NULL;
	PEMCPAL_IRP					new_irp = NULL;
	fbe_status_t			status;
	fbe_u32_t 				irp_size;
	PEMCPAL_IO_STACK_LOCATION		newIrpStack = NULL;
    
	/*allocate conext for completing the call later*/
	irp_context = (io_irp_context_t *) EmcpalAllocateUtilityPoolWithTag (EmcpalNonPagedPool, sizeof (io_irp_context_t),'pyhp');

	if (irp_context == NULL) {
		ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
		user_packet = (fbe_packet_t *)ioBuffer;
		fbe_transport_set_status(user_packet, FBE_STATUS_GENERIC_FAILURE, 0);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;

	}
    
	irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);
	ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
	outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);
	
    /*keep the context for later*/
	irp_context->outLength = outLength;
	irp_context->PIrp = PIrp;

	/*make sure we did not get bogus stuff*/
	if (ioBuffer == NULL) {
		fbe_release_nonpaged_pool_with_tag(irp_context, 'pyhp');
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(irp_context->PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;
	}
	
    
	/*the first thing in the buffer was the io packet*/
	user_packet = (fbe_packet_t *)ioBuffer;
	irp_context->user_packet = user_packet;

	/*we now allocate one of our own and copy the data to it*/
    io_packet = fbe_transport_allocate_packet();

	fbe_transport_initialize_packet(io_packet);

    fbe_transport_set_completion_function(io_packet,
										 physical_package_io_ioctl_completion_callback,
										 (fbe_packet_completion_context_t)irp_context);

    fbe_transport_set_address(io_packet, FBE_PACKAGE_ID_PHYSICAL, user_packet->service_id, user_packet->class_id, user_packet->object_id);

    /*get some basic irpstack information to make deceisions*/
	user_irpStack = (PEMCPAL_IO_STACK_LOCATION)(ioBuffer + FBE_MEMORY_CHUNK_SIZE);
    
    /*continue to copy the data, based on what transaction we have*/
	if (EmcpalExtIrpStackMajorFunctionGet(user_irpStack) == EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL ||
		(EmcpalExtIrpStackMajorFunctionGet(user_irpStack) == EMCPAL_IRP_TYPE_CODE_SCSI && EmcpalExtIrpStackParamScsiSrbGetFunction(user_irpStack) == SRB_FUNCTION_IO_CONTROL)){
		/*build IRP here with all the information we got from the user*/
		irp_context->io_irp = FBE_FALSE;
		new_irp = EmcpalIrpAllocate(3);
		EmcpalExtIrpThreadSet(new_irp, PsGetCurrentThread());
		status = populate_ioctl_irp (new_irp, (fbe_u8_t *)(ioBuffer + FBE_MEMORY_CHUNK_SIZE));
	} else {
		irp_context->io_irp = FBE_TRUE;
		irp_size =  EmcpalIrpCalculateSize(2);/* FIBRE_CPD_IRP_STACK_SIZE = 2*/
		new_irp = EmcpalAllocateUtilityPoolWithTag (EmcpalNonPagedPool, irp_size, 'pyhp');/*yes we need to check null :), later...*/
		EmcpalIrpInitialize(new_irp, irp_size, 2);/* FIBRE_CPD_IRP_STACK_SIZE = 2*/
		EmcpalIrpSetNextStackLocation(new_irp);
		status = populate_io_irp (new_irp, (fbe_u8_t *)(ioBuffer + FBE_MEMORY_CHUNK_SIZE), io_packet);
		/*after we populate the irp we want to remember the ext_srb because it would have a pointer to the sg list we would need to delete*/
		newIrpStack = EmcpalIrpGetNextStackLocation(new_irp);
		irp_context->ext_srb = (CPD_EXT_SRB *)EmcpalExtIrpStackParamScsiSrbGet(newIrpStack);
	}

	/*did everything worked well ?*/
	if (status != FBE_STATUS_OK) {
		fbe_release_nonpaged_pool_with_tag(irp_context, 'pyhp');
		fbe_transport_set_status(user_packet, FBE_STATUS_GENERIC_FAILURE, 0);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;
	}

   /*get the io block from the original packet, we need it for the port number*/
	user_io_block = fbe_transport_get_io_block (user_packet);

	/*set our new irp in the io block of the io packet*/
	new_io_block = fbe_transport_get_io_block(io_packet);
	io_block_build_block_execute_irp (new_io_block, new_irp, user_io_block->operation.execute_irp.port_number);

	/*the order is important here, no not change it
	1. Mark pending
	2. forward the ioctl
	3. return pending
	*/

    EmcpalIrpMarkPending (PIrp);
	fbe_topology_send_io_packet(io_packet);
    return EMCPAL_STATUS_PENDING;
	 
}
#endif

#if 0
static fbe_status_t
physical_package_io_ioctl_completion_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	io_irp_context_t *	 irp_context = (io_irp_context_t *)context;
	fbe_status_t		 status = FBE_STATUS_GENERIC_FAILURE;
	fbe_packet_t * 		 user_packet = NULL;
	fbe_io_block_t *	 io_block = fbe_transport_get_io_block (packet);

    /*get the status and set it in the user packet*/
	if (io_block->operation.execute_irp.io_block_request_status == IO_BLOCK_REQUEST_STATUS_SUCCESS) {
		status = FBE_STATUS_OK;
	} else {
		status = FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_transport_set_status(irp_context->user_packet, status, 0);

    /*set the flags in the IRP*/
	if (status == FBE_STATUS_OK) {
		EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_SUCCESS);
	} else {
        EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
	}

	EmcpalExtIrpInformationFieldSet(irp_context->PIrp, irp_context->outLength);

	/*delete memory we don't need anymore (we delete only the io IRPs, the IOCTL ones were deleted already when completed)*/
	if (irp_context->io_irp) {
		fbe_release_nonpaged_pool_with_tag(io_block->operation.execute_irp.irp, 'pfbe');
		if (irp_context->ext_srb->sgl != NULL) {
			fbe_release_nonpaged_pool_with_tag(irp_context->ext_srb->sgl, 'pfbe');
		}
	}
	
    /*complete to user space, this in turn would set the event the caller waits for*/
    EmcpalIrpCompleteRequest(irp_context->PIrp);
	
	fbe_transport_release_packet (packet);
	fbe_release_nonpaged_pool_with_tag(irp_context, 'pyhp');

	return EMCPAL_STATUS_SUCCESS;
}
#endif

static fbe_status_t populate_ioctl_irp (PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer)
{
    PEMCPAL_IO_STACK_LOCATION		newIrpStack = EmcpalIrpGetNextStackLocation(new_irp);
	PEMCPAL_IO_STACK_LOCATION		userIrpStack = (PEMCPAL_IO_STACK_LOCATION)ioBuffer;
	fbe_status_t			status = FBE_STATUS_OK;
    
	/*start by mass copy the stack locations, we would hand change the rest later*/
	fbe_copy_memory (newIrpStack, userIrpStack, sizeof (EMCPAL_IO_STACK_LOCATION));
	 /*set up some flags for ths irp*/
	EmcpalIrpSetFlags2(new_irp, IRP_BUFFERED_IO);
    
	/*it could be an ioctl which is populated in the iocontrol of the parameters or in the srb filed*/
	if (is_scsi_miniport_ioctl(new_irp)) {
		/*in this case we based things off the srb field*/
		status = populate_ioctl_via_srb (new_irp, ioBuffer);
		if (status != FBE_STATUS_OK) {
			return status;
		}
		/*and update the irp with the data we just updated*/
		EmcpalExtIrpUserBufferSet(new_irp, EmcpalExtIrpStackParamScsiSrbGetDataBuffer(newIrpStack));
		EmcpalExtIrpSystemBufferSet(new_irp, EmcpalExtIrpStackParamScsiSrbGetDataBuffer(newIrpStack));
	} else {
		/*this is an ioctl via the DeviceIoControl*/
       
		/*copy the buffers address*/
		EmcpalExtIrpStackParamType3InputBufferSet(newIrpStack, (fbe_u8_t *)(ioBuffer + sizeof(EMCPAL_IO_STACK_LOCATION)));
		
		/*and update the irp with the data we just updated*/
		EmcpalExtIrpUserBufferSet(new_irp, EmcpalExtIrpStackParamType3InputBufferGet(newIrpStack));
		EmcpalExtIrpSystemBufferSet(new_irp, EmcpalExtIrpStackParamType3InputBufferGet(newIrpStack));

	}

	return status;

}

static fbe_status_t populate_io_irp (PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer, fbe_packet_t *io_packet)
{
	PEMCPAL_IO_STACK_LOCATION		newIrpStack = EmcpalIrpGetNextStackLocation(new_irp);
	PEMCPAL_IO_STACK_LOCATION		userIrpStack = (PEMCPAL_IO_STACK_LOCATION)ioBuffer;
	fbe_status_t			status = FBE_STATUS_OK;
	CPD_EXT_SRB *			ext_srb = NULL;
    
	/*start by mass copy the stack locations, we would hand change the rest later*/
	fbe_copy_memory (newIrpStack, userIrpStack, sizeof (EMCPAL_IO_STACK_LOCATION));
	 /*set up some flags for ths irp*/
	/*new_irp->Flags = IRP_BUFFERED_IO;*/
    
	status = populate_io_srb (new_irp, ioBuffer);

	if (status != FBE_STATUS_OK) {
		return status;
	}

    /*now we fix all the pointers of the sg to point to the correct location in our big buffer
	we first check we actually have so mething to send and rely on the fact the populate_io_srb function
	makde sure the newIrpStack->Parameters.Scsi.Srb points nicely to the ext_srb*/
	ext_srb = (CPD_EXT_SRB *)EmcpalExtIrpStackParamScsiSrbGet(newIrpStack);
	if (ext_srb->sgl != NULL) {
		status  = fix_sg_list_pointers (new_irp, ioBuffer);
		if (status != FBE_STATUS_OK) {
			return status;
		}
	}

	return FBE_STATUS_OK;
}

static fbe_bool_t is_scsi_miniport_ioctl (PEMCPAL_IRP pIrp)
{
    PEMCPAL_IO_STACK_LOCATION  	irpStack = EmcpalIrpGetNextStackLocation(pIrp);

	/*first let's check the ioctl via srb*/
	if (EmcpalExtIrpStackMajorFunctionGet(irpStack) == EMCPAL_IRP_TYPE_CODE_SCSI && EmcpalExtIrpStackParamScsiSrbGetFunction(irpStack) == SRB_FUNCTION_IO_CONTROL ) {
		return FBE_TRUE;
	}

    switch (EmcpalExtIrpStackParamIoctlCodeGet(irpStack)) {
	case IOCTL_SCSI_MINIPORT:
	case IOCTL_SCSI_EXECUTE_NONE:
		return FBE_TRUE;
		break;
	case IOCTL_SCSI_GET_INQUIRY_DATA: 
	case IOCTL_SCSI_GET_CAPABILITIES:
		return FBE_FALSE;
		break;
	default:
		fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: illegal ioctl code: %X\n",
								__FUNCTION__,
								EmcpalExtIrpStackParamIoctlCodeGet(irpStack));
		
		break;
	}

	return FBE_FALSE;
}

static fbe_status_t populate_ioctl_via_srb(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer)
{
    PEMCPAL_IO_STACK_LOCATION		newIrpStack = EmcpalIrpGetNextStackLocation(new_irp);
	PEMCPAL_IO_STACK_LOCATION  	irpStack = NULL;
	PCPD_SCSI_REQ_BLK		userpSrb = NULL;
    S_CPD_CONFIG * 			userCpdIoCtl = NULL;
	S_CPD_CONFIG * 			newCpdIoCtl = NULL;
	fbe_u8_t *				start_of_srb_memory = (fbe_u8_t *)(ioBuffer + sizeof(EMCPAL_IO_STACK_LOCATION));
    
	/*copy the data of the srb*/
    userpSrb = (PCPD_SCSI_REQ_BLK)start_of_srb_memory;
	fbe_copy_memory (EmcpalExtIrpStackParamScsiSrbGet(newIrpStack), userpSrb, sizeof (CPD_SCSI_REQ_BLK));
	
    /*the ioctl srb carries a buffer with it (hopefully) so let's allocate and copy it too*/
	if (cpd_os_io_get_data_buf(userpSrb) != NULL) {
        EmcpalExtIrpStackParamScsiSrbSetDataBuffer(newIrpStack, (fbe_u8_t *)(start_of_srb_memory + sizeof (CPD_SCSI_REQ_BLK)));
	} else {
		/*nothing else to do*/
		return FBE_STATUS_OK;
	}

	if (EmcpalExtIrpStackMajorFunctionGet(newIrpStack) == EMCPAL_IRP_TYPE_CODE_SCSI && 
		cpd_os_io_get_command(userpSrb) == SRB_FUNCTION_IO_CONTROL &&
		cpd_os_io_get_flags(userpSrb) == SRB_FLAGS_FLARE_CMD) {
		return FBE_STATUS_OK;/*no extra buffer to copy here*/
	}

	/*now copy any associated buffer the ioctl may carry*/
    userCpdIoCtl = (S_CPD_CONFIG *)cpd_os_io_get_data_buf(userpSrb);
	newCpdIoCtl = (S_CPD_CONFIG *)EmcpalExtIrpStackParamScsiSrbGetDataBuffer(newIrpStack);

	switch(CPD_IOCTL_HEADER_GET_OPCODE(userCpdIoCtl->ioctl_hdr)){
	case CPD_IOCTL_GET_CONFIG:
		newCpdIoCtl->param.pass_thru_buf = (fbe_u8_t *)(start_of_srb_memory + sizeof (CPD_SCSI_REQ_BLK) + cpd_os_io_get_xfer_len(userpSrb));
		break;
	case CPD_IOCTL_RESET_DEVICE:
	case CPD_IOCTL_QUERY_TARGET_DEVICE_DATA:
		/*nothing to do here*/
		break;
	default:
		fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: illegal ioctl code: %X\n",
								__FUNCTION__,
								CPD_IOCTL_HEADER_GET_OPCODE(userCpdIoCtl->ioctl_hdr);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	

	return FBE_STATUS_OK;
}

static fbe_status_t populate_io_srb(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer)
{
	PEMCPAL_IO_STACK_LOCATION		newIrpStack = EmcpalIrpGetNextStackLocation(new_irp);
	fbe_u8_t *				start_of_srb_memory = (fbe_u8_t *)(ioBuffer + sizeof(EMCPAL_IO_STACK_LOCATION));
	CPD_EXT_SRB *			ext_srb = NULL;
    
	/*set the address of the srb*/
	ext_srb = (CPD_EXT_SRB *)start_of_srb_memory;
	EmcpalExtIrpStackParamScsiSrbSet(newIrpStack, &ext_srb->srb);
	EmcpalExtIrpStackMajorFunctionSet(newIrpStack, EMCPAL_IRP_TYPE_CODE_SCSI);
    
	/*fix the address of the irp to be our irp*/
	EmcpalExtIrpStackParamScsiSrbSetOriginalRequest(newIrpStack, new_irp);
    
    /*the io srb should not carry any buffer with it*/
	if (EmcpalExtIrpStackParamScsiSrbGetDataBuffer(newIrpStack) != NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
	}

	/*point to the sense info buffer area which is behind the extended srb*/
	EmcpalExtIrpStackParamScsiSrbSetSenseInfoBuffer(newIrpStack, (fbe_u8_t *)(start_of_srb_memory + sizeof (CPD_EXT_SRB)));

	/*when we run IO thse should be 0*/
	EmcpalExtIrpUserBufferSet(new_irp, (void *)0x0);
	EmcpalExtIrpSystemBufferSet(new_irp, (void *)0x0);

	return FBE_STATUS_OK;

}

static fbe_status_t fix_sg_list_pointers(PEMCPAL_IRP new_irp, fbe_u8_t * ioBuffer)
{
	/*point to the first sg element in the list*/
	NTBE_SGL *				sg_element = (NTBE_SGL *)(ioBuffer + sizeof(EMCPAL_IO_STACK_LOCATION) + sizeof (CPD_EXT_SRB) + FBE_IO_BLOCK_SENSE_INFO_BUFFER_SIZE);
    NTBE_SGL *				tmp_sg_element = sg_element;
    CPD_EXT_SRB *			ext_srb = (CPD_EXT_SRB *)(ioBuffer+ sizeof(EMCPAL_IO_STACK_LOCATION));/*io buffer now point to the beginning of this structure*/
	NTBE_SGL *				ntbe_sgl = NULL;
	NTBE_SGL *				temp_ntbe_sgl = NULL;
	void * 					virt_address = NULL;
	PHYSICAL_ADDRESS 		phys_address;
	fbe_u32_t				i = 0;
	NTBE_SGL *				user_sg_list = sg_element;
    
    /*we first have to skip over the sg list to the beginning of the data area and point the io buffer there*/

	/*move to start of sg list*/
	ioBuffer += sizeof(EMCPAL_IO_STACK_LOCATION) + sizeof (CPD_EXT_SRB) + FBE_IO_BLOCK_SENSE_INFO_BUFFER_SIZE;

	/*skip all sg elements*/
	while (tmp_sg_element->count !=0) {
		ioBuffer += sizeof (NTBE_SGL);
		tmp_sg_element ++;
	}

	/*and skip over the 0 one*/
	ioBuffer += sizeof (NTBE_SGL);

    /*go over the list and fix the pointers. We would need them to point correctly because we would use the address later
	to convert it to a physical address*/
    while (sg_element->count !=0) {
		sg_element->address = (PHYS_ADDR)(ioBuffer);
		ioBuffer += sg_element->count;
		sg_element ++;
	}

	/*the sg list we got now has the virtual addresses in it.
	we need to create a new sg list with the physical addresses and give it to the driver
	this would fill in our kernel addresses in the buffer*/

    ntbe_sgl = (NTBE_SGL *)fbe_allocate_nonpaged_pool_with_tag(FBE_IO_BLOCK_SGL_SIZE * sizeof (NTBE_SGL), 'pfbe');
    if (ntbe_sgl == NULL) {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

	/*now let's do some conversion*/
    temp_ntbe_sgl = ntbe_sgl;  /* just iterator */
    
     /* Walk every element in the input sg_element and translate to the outbound ntbe_sgl */
	for(i = 0; i < FBE_IO_BLOCK_SGL_SIZE; i++) {
        if (user_sg_list->count == 0) { /* the effective end of SG list*/
			temp_ntbe_sgl->count = 0;
            temp_ntbe_sgl->address = 0;  
            break;
		} else {
			temp_ntbe_sgl->count = user_sg_list->count;
			virt_address = (void *)user_sg_list->address;
			phys_address.QuadPart = fbe_get_contigmem_physical_address(virt_address);
			temp_ntbe_sgl->address = (PHYS_ADDR)(phys_address.QuadPart);

		}

        temp_ntbe_sgl++;
        user_sg_list++;
    }

	/*teach the ext srb where the physical sgl really is*/
	ext_srb->sgl = (PSGL)ntbe_sgl;

    
	return FBE_STATUS_OK;
}
