#include "terminator_test.h"
#include "terminator_port.h"

void port_new_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t * uut = NULL;

    uut = port_new();
    MUT_ASSERT_NOT_NULL(uut);
    MUT_ASSERT_NOT_NULL(&uut->base);
    MUT_ASSERT_INT_EQUAL(FBE_PORT_TYPE_INVALID, uut->port_type);
    MUT_ASSERT_INT_EQUAL(FBE_PORT_SPEED_INVALID, uut->port_speed);
    MUT_ASSERT_NOT_NULL(&uut->logout_queue_head);
    status = port_destroy(uut);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

void port_set_port_number_test(void)
{
    terminator_port_t * uut = NULL;
    terminator_sas_port_info_t *attributes = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_port_info_t port_info;
    fbe_u32_t miniport_port_index;
    
    uut = port_new();
    MUT_ASSERT_NOT_NULL(uut);
    MUT_ASSERT_NOT_NULL(&uut->base);
    MUT_ASSERT_TRUE(FBE_PORT_TYPE_INVALID == port_get_type(uut));
    port_info.sas_address = 0x1234567890abcdef;
    port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
    port_info.io_port_number = 12;
    port_info.portal_number = 123;
    port_info.backend_number = 11;

    port_set_type(uut, port_info.port_type);
    MUT_ASSERT_TRUE(FBE_PORT_TYPE_SAS_PMC == port_get_type(uut));

    /* Create a sas port info block and attach it to the attribute, 
     * when accessing the info, take it out from attribute */
    attributes = sas_port_info_new(&port_info);
    MUT_ASSERT_NOT_NULL(attributes);
    base_component_assign_attributes(&uut->base, attributes);
    MUT_ASSERT_TRUE(port_info.sas_address == sas_port_get_address(uut));
    MUT_ASSERT_TRUE(port_info.io_port_number == sas_port_get_io_port_number(uut));
    MUT_ASSERT_TRUE(port_info.portal_number == sas_port_get_portal_number(uut));
    MUT_ASSERT_TRUE(port_info.backend_number == sas_port_get_backend_number(uut));

    status = port_set_miniport_port_index(uut, 123);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_get_miniport_port_index(uut, &miniport_port_index);
    MUT_ASSERT_INT_EQUAL(123, miniport_port_index);
    status = port_destroy(uut);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

void get_sas_port_address_test(void)
{
    terminator_port_t * uut = NULL;
    terminator_sas_port_info_t *attributes = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_port_info_t port_info;
    
    uut = port_new();
    MUT_ASSERT_NOT_NULL(uut);
    MUT_ASSERT_NOT_NULL(&uut->base);
    MUT_ASSERT_TRUE(FBE_PORT_TYPE_INVALID == port_get_type(uut));

    port_info.sas_address = 0x1234567890abcdef;
    port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
    port_info.io_port_number = 12;
    port_info.portal_number = 123;
    port_info.backend_number = 11;

    port_set_type(uut, port_info.port_type);

    /* Create a sas port info block and attach it to the attribute, 
     * when accessing the info, take it out from attribute */
    attributes = sas_port_info_new(&port_info);
    MUT_ASSERT_NOT_NULL(attributes);
    base_component_assign_attributes(&uut->base, attributes);
    MUT_ASSERT_TRUE(port_info.sas_address == sas_port_get_address(uut));
    MUT_ASSERT_TRUE(port_info.port_type == port_get_type(uut));
    MUT_ASSERT_TRUE(port_info.io_port_number == sas_port_get_io_port_number(uut));
    MUT_ASSERT_TRUE(port_info.portal_number == sas_port_get_portal_number(uut));
    MUT_ASSERT_TRUE(port_info.backend_number == sas_port_get_backend_number(uut));
    status = port_destroy(uut);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

void port_destroy_test(void)
{
    terminator_port_t * uut = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    uut = port_new();
    MUT_ASSERT_NOT_NULL(uut);
    MUT_ASSERT_NOT_NULL(&uut->base);

    status = port_destroy(uut);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}
void sas_port_info_new_test(void)
{
	terminator_sas_port_info_t *uut = NULL;
    fbe_terminator_sas_port_info_t port_info;

    port_info.sas_address = 0x1234567890abcdef;
    port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
    port_info.io_port_number = 12;
    port_info.portal_number = 123;
    port_info.backend_number = 11;

	uut = sas_port_info_new(&port_info);
    MUT_ASSERT_NOT_NULL(uut);
	MUT_ASSERT_TRUE(port_info.sas_address == uut->sas_address);
    MUT_ASSERT_TRUE(port_info.io_port_number == uut->io_port_number);
    MUT_ASSERT_TRUE(port_info.portal_number == uut->portal_number);
    MUT_ASSERT_TRUE(port_info.backend_number == uut->backend_number);
    free(uut);
}
void port_get_last_enclosure_test(void)
{
	terminator_port_t * uut = NULL;
	terminator_enclosure_t * last_encl = NULL;
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	uut = port_new();
	MUT_ASSERT_NOT_NULL(uut);
	/* no enclosure has been added to the port yet. 
	 So a NULL should be returned.
	 No exception should occur.*/
	last_encl = port_get_last_enclosure(uut);
	MUT_ASSERT_NULL(last_encl);
    status = port_destroy(uut);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}
