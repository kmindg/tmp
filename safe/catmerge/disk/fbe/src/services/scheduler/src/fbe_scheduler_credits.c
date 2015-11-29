#include "fbe/fbe_scheduler_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_scheduler_private.h"
#include "fbe_scheduler.h"

/*local structure*/
static fbe_u64_t fbe_schduler_credit_cpu_rate = 10000000000;


/*we start with some  number which will be enough to do some basic background activity.
we assume a system with 1000 drives all doing verify and a few RG doing zero or rebuild.
We then multiply everything by 4 because for now we will consume everything from core 0
*/
static fbe_u32_t MAXIMUM_MBPS = 0;

/*when the out of band schduler is controlling us and gives us less than 100 percent, 
we want to start with a smaller amount to fill the effects of the scheduler throtteling
the MINIMUM_MBPS will allow us to do at least one rebuild*/
static fbe_u32_t MINIMUM_MBPS = 0;

typedef struct schduler_credit_table_entry_s{
	fbe_scheduler_credit_t	current_credits;
	fbe_scheduler_credit_t	master_credits;
	/*TODO: add some stat here on usage or anything else*/
}schduler_credit_table_entry_t;

/*static variables*/
static fbe_u32_t						fbe_scheduler_number_of_cores = 1;
static schduler_credit_table_entry_t	core_credit_table[FBE_CPU_ID_MAX];
static fbe_atomic_t						scheduler_master_memory;
static fbe_atomic_t						current_scale = 100;/*we start with unlimited resources*/

/*forward declerations*/
static fbe_status_t scheduler_credit_get_number_of_cores(fbe_u32_t *cores);
static void scheduler_credit_init_tables(void);
static void scheduler_credit_destroy_tables(void);

/**************************************************************************************************************************************/
fbe_status_t fbe_scheduler_credit_init(void)
{

	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    
	/*first of all, let's see how many cores we have in the system,
	 we have support for a max of FBE_CPU_ID_MAX but we don't want to go ove all of them all the time
	 if we don't have them*/

	status = scheduler_credit_get_number_of_cores(&fbe_scheduler_number_of_cores);
	if (status != FBE_STATUS_OK) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, 
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s failed to get number of cores\n", __FUNCTION__);

		return status;
	}

	MINIMUM_MBPS = 16 * fbe_scheduler_number_of_cores * 10000; /* One rebuild per core */

	/* We will get the memory size available for sep from DPS services */
	MAXIMUM_MBPS = (240 / 4) * 10000; /* We will not use more that 25% of overall available memory */
	
	

	/*now let's initialize some internal data structures*/
	scheduler_credit_init_tables();
    
	return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_credit_destroy(void)
{
    
	return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_remove_credits_per_core(fbe_packet_t *packet)
{
	fbe_scheduler_change_cpu_credits_per_core_t *   change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
	fbe_atomic_t									reverse_value = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_cpu_credits_per_core_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    if (change_credits->core >= FBE_CPU_ID_MAX) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal core number:%d\n", __FUNCTION__, change_credits->core);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	reverse_value = change_credits->cpu_operations_per_second * -1;
    fbe_atomic_add(&core_credit_table[change_credits->core].master_credits.cpu_operations_per_second, reverse_value);

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: core %d credits reduced by: %lld\n",
			__FUNCTION__, change_credits->core,
			(long long)change_credits->cpu_operations_per_second);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_add_credits_per_core(fbe_packet_t *packet)
{
	fbe_scheduler_change_cpu_credits_per_core_t *   change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_cpu_credits_per_core_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    if (change_credits->core >= FBE_CPU_ID_MAX) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal core number:%d\n", __FUNCTION__, change_credits->core);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_atomic_add(&core_credit_table[change_credits->core].master_credits.cpu_operations_per_second,
				   change_credits->cpu_operations_per_second);

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: core %d credits increased by: %lld\n",
			__FUNCTION__, change_credits->core,
			(long long)change_credits->cpu_operations_per_second);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_remove_credits_all_core_cores(fbe_packet_t *packet)
{
	fbe_scheduler_change_cpu_credits_all_cores_t *  change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
	fbe_atomic_t									reverse_value = 0;
	fbe_u32_t										cores = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_cpu_credits_all_cores_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	reverse_value = change_credits->cpu_operations_per_second * -1;

	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
		fbe_atomic_add(&core_credit_table[cores].master_credits.cpu_operations_per_second, reverse_value);

	}

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: All corse credits reduced by: %lld\n",
			__FUNCTION__,
			(long long)change_credits->cpu_operations_per_second);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_add_credits_all_cores(fbe_packet_t *packet)
{
	fbe_scheduler_change_cpu_credits_all_cores_t *  change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
    fbe_u32_t										cores = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_cpu_credits_all_cores_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}
    
    for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
		fbe_atomic_add(&core_credit_table[cores].master_credits.cpu_operations_per_second, change_credits->cpu_operations_per_second);

	}

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: All corse credits increased by: %lld\n",
			__FUNCTION__,
			(long long)change_credits->cpu_operations_per_second);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_add_memory_credits(fbe_packet_t *packet)
{
	fbe_scheduler_change_memory_credits_t *  		change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
	fbe_atomic_t									divided_value = 0;
	fbe_u32_t										cores = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_memory_credits_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    if (change_credits->mega_bytes_consumption_change <= 0) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal memory value:%lld\n", __FUNCTION__, (long long)change_credits->mega_bytes_consumption_change);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/*change the master, and then update the table*/
	fbe_atomic_add(&scheduler_master_memory, change_credits->mega_bytes_consumption_change);

	divided_value = scheduler_master_memory / fbe_scheduler_number_of_cores;

	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
		fbe_atomic_exchange(&core_credit_table[cores].master_credits.mega_bytes_consumption, divided_value);
	}

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: memory increased by:%lld, now:%lld\n",
			__FUNCTION__,
			(long long)change_credits->mega_bytes_consumption_change,
			(long long)scheduler_master_memory);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_remove_memory_credits(fbe_packet_t *packet)
{
	fbe_scheduler_change_memory_credits_t *  		change_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         	payload = NULL;
	fbe_payload_control_operation_t *           	control_operation = NULL; 
	fbe_payload_control_buffer_length_t         	length = 0;
	fbe_atomic_t									divided_value = 0;
	fbe_atomic_t									reverse_value = 0;
	fbe_u32_t										cores = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &change_credits);
	if (change_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_change_memory_credits_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    if (change_credits->mega_bytes_consumption_change <= 0) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal memory value:%lld\n", __FUNCTION__, (long long)change_credits->mega_bytes_consumption_change);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/*change the master, and then update the table*/
	reverse_value = change_credits->mega_bytes_consumption_change * -1;
	fbe_atomic_add(&scheduler_master_memory, reverse_value);

	divided_value = scheduler_master_memory / fbe_scheduler_number_of_cores;

	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
		fbe_atomic_exchange(&core_credit_table[cores].master_credits.mega_bytes_consumption, divided_value);
	}

	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: memory decreased by:%lld, now:%lld\n",
			__FUNCTION__,
			(long long)change_credits->mega_bytes_consumption_change,
			(long long)scheduler_master_memory);
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_set_credits(fbe_packet_t *packet)
{
	fbe_scheduler_set_credits_t *   			set_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         payload = NULL;
	fbe_payload_control_operation_t *           control_operation = NULL; 
	fbe_payload_control_buffer_length_t         length = 0;  
	fbe_u32_t									cores = 0;
	fbe_atomic_t								even_memory = 0;

    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &set_credits);
	if (set_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_set_credits_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	if (set_credits->mega_bytes_consumption <= 0) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal memory size passed:%lld\n", __FUNCTION__, (long long)set_credits->mega_bytes_consumption);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/*we need to set the credits of the master in the table to match the ones we just got from the user*/
	fbe_atomic_exchange(&scheduler_master_memory, set_credits->mega_bytes_consumption);
	even_memory = set_credits->mega_bytes_consumption / fbe_scheduler_number_of_cores;

    for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {

		if (set_credits->cpu_operations_per_second[cores] <= 0) {
			scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Illegal number of operations passed:%lld\n", __FUNCTION__, (long long)set_credits->cpu_operations_per_second[cores]);
			fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
		}

		/*setting the master will assure we load these values next time (next second)*/
		fbe_atomic_exchange(&core_credit_table[cores].master_credits.cpu_operations_per_second, set_credits->cpu_operations_per_second[cores]);

		/*memory is currently allocated evenly among corese*/
		fbe_atomic_exchange(&core_credit_table[cores].master_credits.mega_bytes_consumption, even_memory);
	}


	scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: New credits. CPU:%p, Mem:%lld\n", __FUNCTION__,
			set_credits->cpu_operations_per_second,
			(long long)set_credits->mega_bytes_consumption);

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_get_credits(fbe_packet_t *packet)
{
	fbe_scheduler_set_credits_t *   			get_credits = NULL;    /* INPUT */
	fbe_payload_ex_t *                         payload = NULL;
	fbe_payload_control_operation_t *           control_operation = NULL; 
	fbe_payload_control_buffer_length_t         length = 0;  
	fbe_u32_t									cores = 0;
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &get_credits);
	if (get_credits == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_set_credits_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/*get total memory*/
	fbe_atomic_exchange(&get_credits->mega_bytes_consumption, scheduler_master_memory);

    for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
        fbe_atomic_exchange(&get_credits->cpu_operations_per_second[cores], core_credit_table[cores].master_credits.cpu_operations_per_second);
	}
	
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_scheduler_request_credits(fbe_scheduler_credit_t *credits_to_use, fbe_u32_t core_number, fbe_bool_t *grant_status)
{
	fbe_atomic_t	cpu_cycles_left;
	fbe_atomic_t	memory_left;
	fbe_atomic_t	user_cycles_request = credits_to_use->cpu_operations_per_second;
	fbe_atomic_t	user_memory_reqeust = credits_to_use->mega_bytes_consumption;
    
	if (core_number >= fbe_scheduler_number_of_cores) {
				scheduler_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s core #:%d bigger than # of corese it table:%d \n", __FUNCTION__, core_number, fbe_scheduler_number_of_cores);

				return FBE_STATUS_GENERIC_FAILURE;
	}

	/*in order to track how much we consume, we add the requested MBps to the total request, this will be our peak usage.
	We will use this peak usage to reload the tables on the next clock. The scale from the Background Activity Manager,
	will adjust what we load based on what it permits us.
	
	fbe_atomic_add(&scheduler_master_memory, user_memory_reqeust);

	
	scheduler_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
					FBE_TRACE_MESSAGE_ID_INFO,
					"%s: master memory changed to %d\n", __FUNCTION__, scheduler_master_memory);
	*/

	/*let's take a snapshot of what we have in the table right now*/
	fbe_atomic_exchange(&cpu_cycles_left, core_credit_table[core_number].current_credits.cpu_operations_per_second);
	fbe_atomic_exchange(&memory_left, core_credit_table[core_number].current_credits.mega_bytes_consumption);

	/*do we have enough ?*/
	if ((cpu_cycles_left >= user_cycles_request) && (memory_left >= user_memory_reqeust)){
		/*if so, we just negate the values from the user and use the fbe_atomic_add since there is not fbe_atomic_subtract*/
		user_cycles_request *= -1;
		user_memory_reqeust *= -1;
		fbe_atomic_add(&core_credit_table[core_number].current_credits.cpu_operations_per_second, user_cycles_request);
		fbe_atomic_add(&core_credit_table[core_number].current_credits.mega_bytes_consumption, user_memory_reqeust);

		*grant_status = FBE_TRUE;
/*
        scheduler_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s user_cycles_request:%d, user_memory_reqeust:%d \n", __FUNCTION__,
								user_cycles_request, user_memory_reqeust);
*/
		
	}else{
        scheduler_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Denied: user_cycles_request:%d, user_memory_reqeust:%d \n", __FUNCTION__,
								(int)user_cycles_request, (int)user_memory_reqeust);

		*grant_status = FBE_FALSE;/*too bad, come later*/
	}
	
	return FBE_STATUS_OK;
}

static fbe_status_t scheduler_credit_get_number_of_cores(fbe_u32_t *cores)
{
	*cores = fbe_get_cpu_count();
    if(*cores == 0)
    {
        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s number of cores(%d) is Zero\n", __FUNCTION__, *cores);

    }
	return FBE_STATUS_OK;
}

static void scheduler_credit_init_tables(void)
{
	fbe_u32_t	cores = 0;

	scheduler_master_memory = MAXIMUM_MBPS;/*we keep one pool of memory*/
    
	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
        core_credit_table[cores].master_credits.cpu_operations_per_second = fbe_schduler_credit_cpu_rate;

		/*memory is currently allocated evenly among corese*/
		core_credit_table[cores].master_credits.mega_bytes_consumption = (scheduler_master_memory / fbe_scheduler_number_of_cores);
        
		/*do initial load of current credits*/
		fbe_copy_memory(&core_credit_table[cores].current_credits,
						&core_credit_table[cores].master_credits,
						sizeof (fbe_scheduler_credit_t));
	}

	
}

static void scheduler_credit_destroy_tables(void)
{
	
}

void scheduler_credit_reload_table(void)
{
	fbe_u32_t		cores;

	/*reload the credits to the ones we are currently using from the master ones*/
	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
        fbe_atomic_exchange(&core_credit_table[cores].current_credits.cpu_operations_per_second,
							core_credit_table[cores].master_credits.cpu_operations_per_second);

		fbe_atomic_exchange(&core_credit_table[cores].current_credits.mega_bytes_consumption,
							core_credit_table[cores].master_credits.mega_bytes_consumption);

        if (cores == 0) {
			scheduler_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s: loaded %lld to table \n", __FUNCTION__, (long long)core_credit_table[cores].master_credits.mega_bytes_consumption);
		}
        
	}

}

void scheduler_credit_reset_master_memory(void)
{
	fbe_u32_t		cores;
	fbe_atomic_t	scale;
	fbe_atomic_t 	current_usage;

	fbe_atomic_exchange(&scale, current_scale);
	fbe_atomic_exchange(&current_usage, scheduler_master_memory);

	/*for the Background Activity Manager to be effective, we apply it's scale to the usage to see how much of this usage
	we will take into consideration*/
	current_usage = ((current_usage * scale) / 100);
	if (current_usage == 0) {
		current_usage = MINIMUM_MBPS;
	}

	current_usage  = current_usage / fbe_scheduler_number_of_cores;
	for (cores = 0; cores < fbe_scheduler_number_of_cores; cores++) {
		/*reload in the core table the number we updated based on the requests from the monitors*/
		fbe_atomic_exchange(&core_credit_table[cores].master_credits.mega_bytes_consumption, current_usage);
	}
}

fbe_status_t fbe_scheduler_set_scale_usurper(fbe_packet_t *packet)
{
	fbe_scheduler_set_scale_t *   				set_scale = NULL;    /* INPUT */
	fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *                         payload = NULL;
	fbe_payload_control_operation_t *           control_operation = NULL; 
	fbe_payload_control_buffer_length_t         length = 0;  
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &set_scale);
	if (set_scale == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_set_scale_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}
	
	status = fbe_scheduler_set_scale(set_scale->scale);
    
	fbe_payload_control_set_status(control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

fbe_status_t fbe_scheduler_set_scale(fbe_atomic_t credits_scale)
{
	fbe_u64_t	scale_print = credits_scale;/*in simulation atomic is 32 bits so formatiing will change and we want to prevent it*/

	if (credits_scale < 0 || credits_scale >100) {
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, 
				FBE_TRACE_MESSAGE_ID_INFO,
				"%s: Illegal scale(must be 0-100):0%llx\n",
				__FUNCTION__, (unsigned long long)credits_scale);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_atomic_exchange(&current_scale, credits_scale);

	scheduler_trace(FBE_TRACE_LEVEL_INFO, 
			FBE_TRACE_MESSAGE_ID_INFO,
			"%s: changed to:%llu\n", __FUNCTION__,
			(unsigned long long) scale_print);

	return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_get_scale_usurper(fbe_packet_t *packet)
{
	fbe_scheduler_set_scale_t *   				set_scale = NULL;    /* INPUT */
	fbe_payload_ex_t *                         payload = NULL;
	fbe_payload_control_operation_t *           control_operation = NULL; 
	fbe_payload_control_buffer_length_t         length = 0;  
    
    /* Get the payload to set the configuration information.*/
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &set_scale);
	if (set_scale == NULL){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* If length of the buffer does not match, return an error*/
	fbe_payload_control_get_buffer_length (control_operation, &length);
	if(length != sizeof(fbe_scheduler_set_scale_t)){
		scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}
	
	fbe_atomic_exchange(&set_scale->scale, current_scale);
    
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*return credits we asked for and did not use because other things did not allow us to proceed*/
fbe_status_t fbe_scheduler_return_credits(fbe_scheduler_credit_t *credits_to_return, fbe_u32_t core_number)
{
	fbe_atomic_add(&core_credit_table[core_number].current_credits.cpu_operations_per_second,  credits_to_return->cpu_operations_per_second);
	fbe_atomic_add(&core_credit_table[core_number].current_credits.mega_bytes_consumption, credits_to_return->mega_bytes_consumption);
	return FBE_STATUS_OK;
}


