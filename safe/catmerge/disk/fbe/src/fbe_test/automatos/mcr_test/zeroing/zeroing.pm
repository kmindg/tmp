package mcr_test::zeroing::zeroing;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_disk;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use strict;
use warnings;

sub main
{
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    test_util_general_test_start();
    
    platform_shim_device_check_error_trace_counters($self->{device});
	
    my $timeout = 60*60*3; # s to wait for lun zeroing to finish			
	my $device = $self->{device};
    
    my %disk_zeroing_hash = ();

	$self->platform_shim_test_case_log_info('Gathering disks...');  
	my @disks = @{test_util_configuration_get_fastest_disks($self, {num_disks => 3})};
	my @zero_disks = @{test_util_configuration_get_fastest_disks($self, {num_disks => 3, unzeroed_preferred => 1})};
	
	# Step 1. Insert new drives. Verify that zeroing automatically starts on new drive. 
    #test_util_disk_simulate_new_disk($self, \@disks);
    test_util_disk_simulate_new_disk($self, \@zero_disks);

    foreach my $disk (@zero_disks) 
    {
		my $zeroPercent = platform_shim_disk_get_property($disk, 'zero_percent');
		my $disk_id = platform_shim_disk_get_property($disk, 'id');
		$self->platform_shim_test_case_log_step("Verify Zeroing restarts on new disk $disk_id: $zeroPercent %");
		$self->platform_shim_test_case_assert_true_msg(($zeroPercent != 100), "Zeroing did not restart on disk $disk_id");
		
		$disk_zeroing_hash{$disk_id} = $zeroPercent; 
		
	}
	
	# Step 2. Powercycle drive. Verify zeroing continues on drive
	$self->platform_shim_test_case_log_step("Verify zeroing continues after disk powercycle");    
	my $diskFault = platform_shim_disk_remove($zero_disks[0]);
	my @tempdisks = ($zero_disks[0]);
	my $temp_disk_id = platform_shim_disk_get_property($zero_disks[0], 'id');
	platform_shim_disk_wait_for_disk_removed($device, \@tempdisks);
	platform_shim_disk_reinsert($diskFault);
	platform_shim_disk_wait_for_disk_ready($device, \@tempdisks);
	$disk_zeroing_hash{$temp_disk_id} = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $zero_disks[0], $disk_zeroing_hash{$temp_disk_id}, $timeout);
	
    # Step 3. Create a raid group on the new drive. Verify zeroing continues uninterrupted
    $self->platform_shim_test_case_log_step("Verify zeroing continues uninterrupted after raid group creation");
    my $rg = test_util_configuration_create_raid_group($self, {disks => \@zero_disks, raid_type => 'r5'});
	foreach my $disk (@zero_disks) 
	{
		my $disk_id = platform_shim_disk_get_property($disk, 'id');
		$disk_zeroing_hash{$disk_id} = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zeroing_hash{$disk_id}, $timeout);
	}
	
	# Step 4. Bind lun in nonzeroed area. Verify zeroing completes on lun
	$self->platform_shim_test_case_log_step("Verify zeroing starts and finishes on new lun");

	my ($luncap, $lun_unzeroed);
	if ($self->platform_shim_test_case_is_promotion())
	{
	    $lun_unzeroed = test_util_configuration_create_lun_size($self, $rg, '1GB');
	} 
	else
	{
	    my $disk_id = platform_shim_disk_get_property($zero_disks[0], 'id');
        my $disk_cap = platform_shim_disk_get_property($zero_disks[0], 'capacity');
        my $offset = int($disk_zeroing_hash{$disk_id} * $disk_cap * .01);
    	$luncap = ($offset * 3  > 20 ? $offset * 3 : 20);
    	$luncap = ($luncap < $disk_cap ? $luncap : $disk_cap);
    	$luncap = $luncap * scalar(@zero_disks);
    	$luncap = 10000;
    	
    	$luncap = test_util_configuration_get_data_disks($self, $rg)  * 2;
    	$lun_unzeroed = test_util_configuration_create_lun_size($self, $rg, $luncap."GB");
	}
	test_util_zeroing_wait_for_lun_zeroing_percent($self, $lun_unzeroed, 100, $timeout);


    # promotion test ends here
    if ($self->platform_shim_test_case_is_promotion())
    {
        test_util_general_log_test_time($self);
        return;
    }
    
	
	# Step 5a. Unbind lun, NDB lun, check lun
	sleep(5);
	$self->platform_shim_test_case_log_step("Write data pattern to lun");
	my %ndb_params = test_util_configuration_get_ndb_parameters($self, $lun_unzeroed);
	my %rdtParams = (lba_address => 0, 
					 lba_count => 10, 
					 set_pattern => 1, 
					 'write' => 1);
	platform_shim_lun_rdt($lun_unzeroed, \%rdtParams);
	%rdtParams = (lba_address => 0, 
			      lba_count => 10, 
				  check_pattern => 1, 
				  'read' => 1);
	my %res = platform_shim_lun_rdt($lun_unzeroed, \%rdtParams);
	
	
	# Step 6. Verify zeroing proceeds even after lun is unbound.
	$self->platform_shim_test_case_log_step("Verify zeroing continues uninterrupted on lun unbind");
	platform_shim_lun_unbind($lun_unzeroed);
	sleep(5);
	foreach my $disk (@zero_disks) 
	{
		my $disk_id = platform_shim_disk_get_property($disk, 'id');
		$disk_zeroing_hash{$disk_id} = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zeroing_hash{$disk_id}, $timeout);
	}

	# Step 5b: Verify ndb lun
	$self->platform_shim_test_case_log_step("Verify disk zeroing continues uninterrupted when ndb'ing lun");
	$lun_unzeroed = platform_shim_lun_ndb($rg, \%ndb_params);
	sleep(5);
	foreach my $disk (@zero_disks) 
	{
		my $disk_id = platform_shim_disk_get_property($disk, 'id');
		$disk_zeroing_hash{$disk_id} = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zeroing_hash{$disk_id}, $timeout);
	}
	
	$self->platform_shim_test_case_log_step("Verify data on ndb'd lun");
	my %ans = platform_shim_lun_rdt($lun_unzeroed, \%rdtParams);
	foreach my $key (keys %ans)
	{
		my @results = @{$res{$key}};
		my @check = @{$ans{$key}};
		foreach my $resword (@results) 
		{
			my $checkword = shift(@check);
			$self->platform_shim_test_case_assert_true_msg($resword eq $checkword, "$resword != $checkword");
		}	
	}
	
	# sp reboot and dual sp reboot during zero
	my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($skip_reboot != 1) {
	   $self->zeroing_with_sp_reboots_subtest(\@zero_disks);
	} 
	
	# Step 7. Verify disk zeroing completes on new drive 
	foreach my $disk (@disks)
	{
		my $disk_id = platform_shim_disk_get_property($disk, 'id');
		$self->platform_shim_test_case_log_step("Verify zeroing completes on disk $disk_id");
		test_util_zeroing_wait_for_disk_zeroing_percent($self, $disk, 100, $timeout);
		$disk_zeroing_hash{$disk_id} = 100;
	}
	
	# Step 8. Verify Zeroing checkpoint does not move backward on raid group creation on zeroed drive
	$self->platform_shim_test_case_log_step("Verify zero checkpoint does not move backward on raid group creation on zeroed disk");
    $rg = test_util_configuration_create_raid_group($self, {disks => \@disks, raid_type => 'r5'});
    foreach my $disk (@disks) 
    {
	    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    	my $zeroingPercent = platform_shim_disk_get_property($disk, 'zero_percent'); 
    	$self->platform_shim_test_case_assert_true_msg($zeroingPercent >=  $disk_zeroing_hash{$disk_id}, 
    					     "$disk_id zeroing percent ".$zeroingPercent." decreased from ".$disk_zeroing_hash{$disk_id});
	}
	
	# Step 9. Bind lun in zeroed area. Zeroing does not actually occur on drive
	$self->platform_shim_test_case_log_step("Verify lun bind on zeroed area does not cause zeroing to reoccur on that area");
	my $lun_zeroed = test_util_configuration_create_lun_size($self, $rg, '20GB');
	my $zeroed_timeout = 10 *60; # 10 min
	test_util_zeroing_wait_for_lun_zeroing_percent($self, $lun_zeroed, 100, $zeroed_timeout);
	
				  
	# Step 10. Validate Event log messages
	$self->platform_shim_test_case_log_step("Verify appropriate event log messages are present");
	platform_shim_event_verify_lun_zeroing_started({object => $self->{device},
                             	  lun => $lun_unzeroed});     
	platform_shim_event_verify_lun_zeroing_completed({object => $self->{device},
                                    lun => $lun_unzeroed});   
	platform_shim_event_verify_lun_zeroing_started({object => $self->{device},
                             	  lun => $lun_zeroed});     
	platform_shim_event_verify_lun_zeroing_completed({object => $self->{device},
                                    lun => $lun_zeroed});  

    foreach my $disk (@disks)
    {
	    platform_shim_event_verify_disk_zeroing_started({object => $self->{device},
                             	  	  disk => $disk});     
		platform_shim_event_verify_disk_zeroing_completed({object => $self->{device},
                                    	 disk => $disk});
	}


    #step 11. Validate that there are no error traces
    $self->platform_shim_test_case_log_step("Verify no error traces are present");
	platform_shim_device_check_error_trace_counters($self->{device});
	
	test_util_general_log_test_time($self);
	
    return;
}

sub zeroing_with_sp_reboots_subtest
{
    my $self = shift;
    my $disk_ref = shift;
    my @disks = @{$disk_ref};
    
    my $timeout = 10* 60; # 10 min
    my $device = $self->{device};
    
    return if ($self->platform_shim_test_case_is_promotion());
  
    $self->platform_shim_test_case_log_info("zeroing with sp reboots subtest");
    # Step 1: initiate disk zeroing
    $self->platform_shim_test_case_log_step("Zeroing starts on disk");
    my $disk_id = platform_shim_disk_get_property($disks[0], 'id');
    #test_util_disk_simulate_new_disk($self, \@disks);  
    my $zero_percent = platform_shim_disk_get_property($disks[0], 'zero_percent');
    $zero_percent = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disks[0], $zero_percent, $timeout);
    $self->platform_shim_test_case_log_info("Disk $disk_id zeroing at $zero_percent%");
    
    # Step 2: Verify disk zeroing continues on single SP reboot
    $self->platform_shim_test_case_log_step("Verify zeroing continues on single sp reboot");
    my @sp = platform_shim_device_get_sp($device);
    platform_shim_sp_reboot($sp[0]);
    $self->platform_shim_test_case_log_info("Check on ECOM");
    platform_shim_device_wait_for_ecom($device,3600);
    
    $zero_percent = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disks[0], $zero_percent, $timeout);
    $zero_percent = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disks[0], $zero_percent, $timeout);
    $self->platform_shim_test_case_log_info("Disk $disk_id zeroing increased to $zero_percent%");
    platform_shim_sp_wait_for_agent($sp[0], 3600);
    
    
    
    # Step 3: Verify disk zeroing continues on dual SP reboot
    $self->platform_shim_test_case_log_step("Verify zeroing continues on dual sp reboot");
    platform_shim_sp_reboot($sp[0]);
    platform_shim_sp_reboot($sp[1]);
    $self->platform_shim_test_case_log_info("Wait for Sps to come back up");
    sleep(20);
    platform_shim_sp_wait_for_agent($sp[0], 3600);
    platform_shim_sp_wait_for_agent($sp[1], 3600);
    platform_shim_device_wait_for_ecom($device,600);
    #$zero_percent = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disks[0], $zero_percent, $timeout);
    $zero_percent = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disks[0], $zero_percent, $timeout);
    $self->platform_shim_test_case_log_step("Disk $disk_id zeroing increased to $zero_percent%");
    
    
    return;
}


1;
