package mcr_test::rebuild::rebuild;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_event;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::test_util::test_util_io;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_raid_group;
use strict;
use warnings;

#rdgen -clar_lun 0 -affine 1 -align w 0 1 0x400
our $g_sparing_timer = 60; # 60 seconds (full rebuild swap time)
sub main
{
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    test_util_general_test_start();
    my $device = $self->{device};
    $self->platform_shim_test_case_log_start();
    $self->platform_shim_test_case_add_to_cleanup_stack({'rebuild_cleanup' => {}});  
    my $rdgen_sp = platform_shim_device_get_active_sp($device, {});
    
    # Step: Setup the raid groups and test cases
    $self->platform_shim_test_case_log_step("setup the raid groups and test cases");
    my @rgs = @{$self->rebuild_create_raid_groups()};
    my @test_cases = @{$self->rebuild_setup_test_cases(\@rgs)};
 
    # set the sparing timer to something really high for differential rebuild test case
    platform_shim_device_set_sparing_timer($device, 10000);   
  
    # remove drives and verify the right things occur  
    $self->rebuild_start_drive_faults(\@test_cases);
   
    # Step: Start io on all luns, paint pattern on first lun
    platform_shim_device_initialize_rdgen($device);
    test_util_general_set_target_sp($self, $rdgen_sp);
    $self->rebuild_start_io(\@test_cases);

    # Step: Recover drives to start rebuild
    $self->platform_shim_test_case_log_step("Recover disk faults");
    platform_shim_device_recover_from_faults($device);
    $self->rebuild_verify_rebuild_starts(\@test_cases);

    # Step: Verify rebuild completes
    $self->rebuild_verify_rebuild_completes(\@test_cases);
 
    # FULL REBUILD START
    $self->platform_shim_test_case_log_info("Full rebuild test start");
    platform_shim_device_enable_automatic_sparing($device);
    
    # remove drives and verify the right things occur   
    $self->rebuild_start_drive_faults(\@test_cases);
   
    platform_shim_device_set_sparing_timer($device, $g_sparing_timer);
    $self->platform_shim_test_case_log_info("Wait for sparing timer to expire");                                  
    sleep($g_sparing_timer);
    
    # verify that a spare swaps in and full rebuild starts
    $self->rebuild_verify_full_rebuild_starts(\@test_cases);
    test_util_rebuild_set_speed($self, 10) if ($self->platform_shim_test_case_is_functional());
    
    # Step: Verify reinserted disks become available spares
    platform_shim_device_recover_from_faults($device);
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion());
        next if ($test_case->{raid_type} eq 'r0');
        foreach my $disk (@{$test_case->{disks_to_fault}})
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info('Waiting for Unbound state for disk ' . $disk_id);
            platform_shim_device_wait_for_property_value($device, {objects => [$disk],
                                                                property => 'state',
                                                                value => 'Unbound'});
                                                                            
        }
    }

    # Step: Fault rebuilding drive during rebuild
    $self->rebuild_fault_rebuilding_drives_sub_test_case(\@test_cases); 

    # Step: Verify if there are no spares available and drives are faulted, no spares swap in
    $self->rebuild_no_spares_available_sub_test_case(\@test_cases);
   
    # Step: Bind and unbind during rebuild
    $self->rebuild_bind_unbind_during_rebuild_sub_test_case(\@test_cases);  

    # Step: Single and Dual SP panics 
    my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($skip_reboot != 1) {
        $self->rebuild_reboot_sp_during_rebuild_sub_test_case(\@test_cases);
    }
    
    # let rebuild continue more quickly
    test_util_rebuild_restore_speed($self);
    
    # Step: Verify rebuild full rebuild completes
    $self->rebuild_verify_rebuild_completes(\@test_cases, 1);

    test_util_general_set_target_sp($self, $rdgen_sp);
    $self->platform_shim_test_case_log_info("Stopping rdgen threads");
    platform_shim_device_rdgen_stop_io($device, {});
    
    # Step: Verify data pattern on luns
    $self->platform_shim_test_case_log_step("Checking data pattern on luns");
    my @luns_to_check = ();
    foreach my $test_case (@test_cases)
    {
        push @luns_to_check, $test_case->{lun1};
    }

    test_util_io_check_background_pattern($self,  {luns => \@luns_to_check});
     
    # reset the sparing timer
    platform_shim_device_set_sparing_timer($device, 5*60);
    
    test_util_general_log_test_time($self);
}



sub rebuild_create_raid_groups
{
    my $self = shift;
    my $device = $self->{device};
    
    my @rgs = ();
    if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        # Step 1: Create the raid groups, different widths and drive types, parity
        if ($self->platform_shim_test_case_is_promotion())
        {
            my $rg = test_util_configuration_create_redundant_config($self, {reserve => 2});
            push @rgs, $rg;
        } 
        else
        {
            @rgs = @{test_util_configuration_create_all_raid_types_reserve($self)};
            eval {
                my $rg = test_util_configuration_create_raid_group_only_system_disks($self);
                push @rgs, $rg;
            };
        }
    } 
    else 
    {
        @rgs = platform_shim_device_get_raid_group($device, {});
    }
    
    return \@rgs;
}


sub rebuild_setup_test_cases
{
    my ($self, $rg_ref) = @_;
    my $device = $self->{device};
    
    my @test_cases = ();
    my $lun_capacity_per_disk = 10;
    my $lun_capacity_gb;
    
    if (platform_shim_device_is_simulator($self->{device}))
    {
        $lun_capacity_per_disk = 2;   
    }

    foreach my $rg (@{$rg_ref})
    {
        my %test_case = ();
        my $lun1;
        my $lun2;
        if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {

            if ($self->platform_shim_test_case_is_promotion())
            {
                $lun1 = test_util_configuration_create_lun($self, $rg, {});
                $lun2 = test_util_configuration_create_lun($self, $rg, {});
            }
            else 
            {
                $lun_capacity_gb = test_util_configuration_get_data_disks($self, $rg) * $lun_capacity_per_disk;
                my $lun_cap_buffer = $lun_capacity_gb * 2;
                $lun1 = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
                $lun2 = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
                my $lun3 = test_util_configuration_create_lun_size($self, $rg, $lun_cap_buffer."GB");
            }
        } 
        else 
        {
            my @rg_luns = platform_shim_raid_group_get_lun($rg);
            $lun1 = $rg_luns[0];
            $lun2 = $rg_luns[1];
        }
        
        my @disks = platform_shim_raid_group_get_disk($rg);
        
        $test_case{raid_group} = $rg;
        $test_case{raid_group_id} = platform_shim_raid_group_get_property($rg, 'id');
        $test_case{raid_type} = platform_shim_raid_group_get_property($rg, 'raid_type');
        $test_case{lun1} = $lun1;
        $test_case{lun2} = $lun2;
        my @disks_to_fault = (shift(@disks));
        if (test_util_disk_is_vault_disk($self, $disks_to_fault[0]))
        {
            # skip removing system drives if ntmirror is not rebuilt
            my @sps = platform_shim_device_get_sp($device);
            if (!test_util_rebuild_is_ntmirror_complete($self, $sps[0]) ||
                !test_util_rebuild_is_ntmirror_complete($self, $sps[1]))
            {
                $self->platform_shim_test_case_log_info("NTMirror rebuilding. Skip system drive test");
                next;
            }
            
            my $disk_id = platform_shim_disk_get_property($disks_to_fault[0], "id");
            $self->platform_shim_test_case_log_info("Planning to fault vault disk: $disk_id");
            my $psm_rg_fbe_id = platform_shim_device_get_psm_raid_group_id($device);
            my $vault_rg_fbe_id = platform_shim_device_get_vault_raid_group_id($device);
            $test_case{check_system_rebuilds} = 1;
            
            if (test_util_disk_is_psm_disk($self, $disks_to_fault[0])) {
                $test_case{system_rg_fbe_ids} = [$psm_rg_fbe_id, $vault_rg_fbe_id];
            } else {
                $test_case{system_rg_fbe_ids} = [$vault_rg_fbe_id];
            }
        } 
        elsif ($test_case{raid_type} eq 'r6')
        {
            push @disks_to_fault , shift(@disks);
        }
        
        $test_case{variable} = ();
        $test_case{disks_to_fault} = \@disks_to_fault;
        push @test_cases, \%test_case;
    }
    
    return \@test_cases;
}

sub rebuild_start_drive_faults
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    
    my $device = $self->{device};
    # Step 2: Fault a disk in the raid group and verify rl
    foreach my $test_case (@test_cases) {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        my $rg_id = platform_shim_raid_group_get_property($test_case->{raid_group}, 'id');
        my @faults = ();
        foreach my $disk (@{$test_case->{disks_to_fault}})
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info("Remove disk ".$disk_id." in RAID group ".$rg_id);
            push @faults, platform_shim_disk_remove($disk);   
        }
        $test_case->{disk_faults} = \@faults;
    }
    
    foreach my $test_case (@test_cases) {
        foreach my $disk (@{$test_case->{disks_to_fault}})
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info('Waiting for removed state for disk ' . $disk_id);
            platform_shim_disk_wait_for_disk_removed($device, [$disk]);
        }
    }
        
    foreach my $test_case (@test_cases) {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        # Wait for the luns to be in a Faulted state                  
        my @lun_objects = platform_shim_raid_group_get_lun($test_case->{raid_group});
        my $temp = join(', ',  map {platform_shim_lun_get_property($_, 'id')} @lun_objects);
        $self->platform_shim_test_case_log_step('Waiting for Faulted state for luns ' . $temp);
        platform_shim_lun_wait_for_faulted_state($device, \@lun_objects);
                                        
        if ($test_case->{raid_type} eq 'r0')
        {
            # Step: Verify rebuild logging does not start for nonredundant raid types
            $self->platform_shim_test_case_log_step('Verify that rebuild logging did not start for raid group '.$test_case->{raid_group_id});                    
            platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                               raid_group => $test_case->{raid_group},
                                                               negation => 1,
                                                               timeout => '2 M'}); 
            foreach my $fault (@{$test_case->{disk_faults}})
            {
                platform_shim_disk_reinsert($fault);
            }                                                                                                                    
        } 
        else 
        {
            # Step: Verify rebuild logging starts for redundant raid types
            $self->platform_shim_test_case_log_step('Verify that rebuild logging starts for raid group '.$test_case->{raid_group_id});                    
            platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                               raid_group => $test_case->{raid_group}}); 
                                                               
            $self->platform_shim_test_case_log_info("Verify rebuild checkpoint is at 0");
            my $rebuild_checkpoint = platform_shim_raid_group_get_property($test_case->{raid_group}, 'rebuild_checkpoint');
            $self->platform_shim_test_case_assert_true_msg(hex($rebuild_checkpoint) == 0,
                "The checkpoint did not go to 0: $rebuild_checkpoint");
                
            $self->rebuild_check_system_rebuild_logging_started($test_case);
        }
    }
    
    $test_case_ref = \@test_cases;
}

sub rebuild_verify_rebuild_starts
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    
    my $device = $self->{device};
    
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        next if ($test_case->{raid_type} eq 'r0');
        
        # Wait for luns to be ready
        my @lun_objects = platform_shim_raid_group_get_lun($test_case->{raid_group});
        my $temp = join(', ',  map {platform_shim_lun_get_property($_, 'id')} @lun_objects);
        $self->platform_shim_test_case_log_info('Waiting for ready state for luns ' . $temp);
        platform_shim_lun_wait_for_ready_state($device, \@lun_objects);
        
        # check that rebuild logging has stopped 
        $self->platform_shim_test_case_log_info("Verify that rebuild logging has stopped for raid group ".$test_case->{raid_group_id});
        platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                            raid_group => $test_case->{raid_group}});
        $self->rebuild_check_system_rebuild_logging_stopped($test_case);                                                              
        
        # Verify rebuild started
        foreach my $disk (@{$test_case->{disks_to_fault}})
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->logStep("Verify that rebuild started on disk ".$disk_id);
            test_util_rebuild_wait_for_rebuild_checkpoint_increase($self, $test_case->{raid_group}, '0x0', 600);
            platform_shim_event_verify_rebuild_started({object => $device, disk => $disk});
        }
    }
}

sub rebuild_verify_full_rebuild_starts
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    
    my $device = $self->{device};
    # wait for full rebuild
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        next if ($test_case->{raid_type} eq 'r0');
        # check that rebuild logging has stopped 
        $self->platform_shim_test_case_log_info("Verify that rebuild logging has stopped for raid group ".$test_case->{raid_group_id});
        platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                            raid_group => $test_case->{raid_group}});
        # check that rebuild does not occur on the system drives to a spare
        $self->rebuild_check_system_rebuild_logging_stopped($test_case, 1); 
        my @spares = ();
        foreach my $disk (@{$test_case->{disks_to_fault}})
        {
            # Check that a spare has swapped in
            my $spare_loc = platform_shim_event_verify_spare_swapped_in({object => $device, 
                                                                        disk => $disk}); 
            $self->platform_shim_test_case_log_info("Spare location is $spare_loc");
            my ($spare) = platform_shim_device_get_disk($device, {id => $spare_loc});
            push @spares, $spare;
            
            # verify raid group config
            next if ($test_case->{raid_type} eq 'r6');
            platform_shim_raid_group_verify_swap($test_case->{raid_group}, $disk, $spare);
                                                                            
        }
        $test_case->{spares} = \@spares;

        if ($test_case->{raid_type} eq 'r6' && $self->platform_shim_test_case_is_functional())
        {
            $self->logStep("Verify that rebuild started on permanent spare ");
            platform_shim_event_verify_rebuild_started({object => $device, 
                                                        raid_group => $test_case->{raid_group}});
        }
        else
        {
            # Verify rebuild started
            foreach my $disk (@{$test_case->{spares}})
            {
                my $disk_id = platform_shim_disk_get_property($disk, 'id');
                $self->logStep("Verify that rebuild started on permanent spare ".$disk_id);
                platform_shim_event_verify_rebuild_started({object => $device, 
                                                            raid_group => $test_case->{raid_group}, 
                                                            disk => $disk});
            }
        }

    }
    
    $test_case_ref = \@test_cases;
    
}

sub rebuild_verify_rebuild_completes
{
    my $self = shift;
    my $test_case_ref = shift;
    my $is_full_rebuild = shift || 0;
    my @test_cases = @{$test_case_ref};
    
    my $device = $self->{device};
    
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        next if ($test_case->{raid_type} eq 'r0');
        my @rebuilding_disks = @{$test_case->{disks_to_fault}};
        @rebuilding_disks = @{$test_case->{spares}} if ($is_full_rebuild);

        if ($test_case->{raid_type} eq 'r0')
        {
            my @disks = @{$test_case->{disks_to_fault}};
            $self->platform_shim_test_case_log_step("Verify that rebuild did not occur for raid_group ".$test_case->{raid_group_id});
            platform_shim_event_verify_rebuild_completed({object => $device, 
                                                         disk => $disks[0],
                                                         negation => 1, 
                                                         timeout => '2 M'}); 
            next;
        } 
        else
        {
            test_util_rebuild_wait_for_rebuild_complete($self, $test_case->{raid_group}, 3600);
            foreach my $disk (@rebuilding_disks)
            {
                my $disk_id = platform_shim_disk_get_property($disk, 'id');
                $self->platform_shim_test_case_log_info("Verify that rebuild completed on disk  ".$disk_id);
                platform_shim_event_verify_rebuild_completed({object => $device, 
                                                              disk => $disk});
                $self->rebuild_check_system_rebuild_completed($test_case, $disk);
            }
            $self->platform_shim_test_case_log_info("Verify that rebuild percent is 100%"); 
            my $rebuild_percent = platform_shim_raid_group_get_property($test_case->{raid_group},'rebuilt_percent');
            $self->platform_shim_test_case_assert_true_msg($rebuild_percent == 100, "The rebuild percent has not reached 100: ".$rebuild_percent);
        }
        
        my @spares;
        my @disks; 
        if ($is_full_rebuild)
        {
           
            if($self->platform_shim_test_case_is_promotion())
	    {
            	 @spares = @{$test_case->{spares}};
                 @disks = @{$test_case->{disks_to_fault}};
            }
            else
	    {
	    	 @spares = @{$test_case->{disks_to_fault}};
            	 @disks = @{$test_case->{spares}};
 	    }      
        
           # skip if r6 has 2 spares it is possible one spare replaced the other
            next if ($test_case->{raid_type} eq 'r6' && scalar(@spares) > 1);
            foreach my $disk (@disks) {
                my $spare = shift(@spares);
                # Verify raid group config
                platform_shim_raid_group_verify_swap($test_case->{raid_group}, $disk, $spare);
            }
        }
        
        # Wait for luns to be ready
        my @lun_objects = platform_shim_raid_group_get_lun($test_case->{raid_group});
        my $temp = join(', ',  map {platform_shim_lun_get_property($_, 'id')} @lun_objects);
        $self->platform_shim_test_case_log_info('Waiting for ready state for luns ' . $temp);
        platform_shim_lun_wait_for_ready_state($device, \@lun_objects);
        
    }
}

sub rebuild_start_io
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    my $device = $self->{device};
    
    $self->platform_shim_test_case_log_step("start rdgen to all luns");
    my $DEFAULT_MAX_LBA = 0xF000;
    my $TEST_ELEMENT_SIZE = 0x80;
                
    
    test_util_io_write_background_pattern($self);
    
    my %rdgen_params = (write_read_check   => 1, 
                        thread_count => 4, 
                        block_count => 0x80, 
                        #object_id    => $FBE_OBJECT_ID_INVALID,
                        #class_id     => $FBE_CLASS_ID_LUN,
                        #sequential_increasing_lba => 1,
                        affinity_spread => 1,
                        use_block => 1,
                        start_lba => 0,
                        minimum_lba => 0,
                        minimum_blocks => $TEST_ELEMENT_SIZE,
                        
                        );
                        
    foreach my $test_case (@test_cases)
    {
        platform_shim_lun_rdgen_start_io($test_case->{lun2}, \%rdgen_params);
    }
}

sub rebuild_fault_rebuilding_drives_sub_test_case
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    my $device = $self->{device};
    
    return if ($self->platform_shim_test_case_is_promotion());
    
    $self->platform_shim_test_case_log_step("VERIFY THAT A SPARE CAN SWAP IN FOR A PERMANENT SPARE");
    
    platform_shim_device_disable_automatic_sparing($device);
    
    # Step: remove rebuilding drives
    foreach my $test_case (@test_cases)
    {
        next if ($test_case->{raid_type} eq 'r0');
         @{$test_case->{variable}} = @{$test_case->{disks_to_fault}};
        @{$test_case->{disks_to_fault}} =  @{$test_case->{spares}};
        @{$test_case->{spares}} = ();
    }   
    $self->rebuild_start_drive_faults(\@test_cases);

    # Step: verify full rebuild starts again
    platform_shim_device_enable_automatic_sparing($device);
    $self->platform_shim_test_case_log_info("Wait for sparing timer to expire");                                  
    sleep($g_sparing_timer + 20); # add some buffer time
    $self->rebuild_verify_full_rebuild_starts(\@test_cases);
    #platform_shim_device_recover_from_faults($device);
    
}

sub rebuild_no_spares_available_sub_test_case
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    my $device = $self->{device};
    
    return if ($self->platform_shim_test_case_is_promotion());
    $self->platform_shim_test_case_log_step("VERIFY IF THERE ARE NO AVAILABLE SPARES NO DRIVES WILL SWAP IN");
    
    # Step: Verify if there are no spares available that nothing swaps in
    # and rebuild continues
    my %unbound_params = (state      =>"unused",
                          vault      => 0);
    my @unbound_faults = ();                          
    my @unbound_disks = platform_shim_device_get_disk($device, \%unbound_params);
    foreach my $unbound_disk (@unbound_disks) 
    {
        push @unbound_faults, platform_shim_disk_remove($unbound_disk);
    }
    
    foreach my $unbound_disk (@unbound_disks) 
    {
        my $disk_id = platform_shim_disk_get_property($unbound_disk, 'id');
        $self->platform_shim_test_case_log_info('Waiting for Empty state for unbound disk ' . $disk_id);
        platform_shim_disk_wait_for_disk_removed($device, [$unbound_disk]);
    }
    
    my @rg_faults = ();  
    foreach my $test_case (@test_cases)
    {
        next if ($test_case->{raid_type} eq 'r0');
        next if ($test_case->{check_system_rebuilds});
        foreach my $spare (@{$test_case->{spares}})
        {
            my $disk_id = platform_shim_disk_get_property($spare, 'id');
            push @rg_faults, platform_shim_disk_remove($spare);
            $self->platform_shim_test_case_log_info('Waiting for Removed state for disk ' . $disk_id);
            platform_shim_disk_wait_for_disk_removed($device, [$spare]);
        }
    }
    
    # wait for the sparing timer to expire
    sleep($g_sparing_timer);
    
    # reinsert all drives
    $self->platform_shim_test_case_log_info("Reinsert all drives to allow rebuild to continue");

    foreach my $rg_fault (@rg_faults)
    {
        platform_shim_disk_reinsert($rg_fault);
    }
    
    sleep(5); 
    platform_shim_device_recover_from_faults($device);
}

sub rebuild_bind_unbind_during_rebuild_sub_test_case
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    my $device = $self->{device};
    
    return if ($self->platform_shim_test_case_is_promotion());
    
    $self->platform_shim_test_case_log_step("BIND AND UNBIND DURING REBUILD");
    
    # Step: Bind lun during rebuild
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        my @prev = platform_shim_raid_group_get_lun($test_case->{raid_group});
        my $lun = test_util_configuration_create_lun($self, $test_case->{raid_group}, {});
        my @rg_luns = platform_shim_raid_group_get_lun($test_case->{raid_group});
        $self->platform_shim_test_case_assert_true_msg(scalar(@rg_luns) > scalar(@prev) ,"binding during rebuild failed for rg ". $test_case->{raid_group_id});
        $test_case->{new_lun} = $lun;
    }
    
    # Step: Unbind lun during rebuild
    foreach my $test_case (@test_cases)
    {
        #next if ($self->platform_shim_test_case_is_promotion() && $test_case->{raid_type} ne 'r5');
        my @prev = platform_shim_raid_group_get_lun($test_case->{raid_group});
        platform_shim_lun_unbind($test_case->{new_lun});
        my @rg_luns = platform_shim_raid_group_get_lun($test_case->{raid_group});
        $self->platform_shim_test_case_assert_true_msg(scalar(@rg_luns) < scalar(@prev) ,"unbinding during rebuild failed for rg ". $test_case->{raid_group_id});
    }
}

sub rebuild_reboot_sp_during_rebuild_sub_test_case
{
    my ($self, $test_case_ref) = @_;
    my @test_cases = @{$test_case_ref};
    my $device = $self->{device};
    
    return if ($self->platform_shim_test_case_is_promotion());
    $self->platform_shim_test_case_log_step("SP FAILURES DURING REBUILD");
    
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    
    my %rebuild_percent = ();
    my %rebuild_checkpoint  =();
    map {$rebuild_percent{$_->{raid_group_id}} = platform_shim_raid_group_get_property($_->{raid_group}, 'rebuilt_percent')} @test_cases;
    map {$rebuild_checkpoint{$_->{raid_group_id}} = platform_shim_raid_group_get_property($_->{raid_group}, 'rebuild_checkpoint')} @test_cases;
    $self->platform_shim_test_case_log_step("reboot active sp");
    platform_shim_sp_reboot($active_sp);
    
    # Step 5: Verify that rebuild continues on the passive SP. (passive becomes active) 
    $self->platform_shim_test_case_log_step("Verify rebuild continues on the passive sp");
    test_util_general_set_target_sp($self, $passive_sp);
    foreach my $test_case (@test_cases)
    {
        next if ($test_case->{raid_type} eq 'r0');
        my $rg = $test_case->{raid_group};
        my $rg_id = $test_case->{raid_group_id};
        #$rebuild_percent{$rg_id} = test_util_rebuild_wait_for_rebuild_percent_increase($self , $rg, $rebuild_percent{$rg_id}, 3600);
        $rebuild_checkpoint{$rg_id} = test_util_rebuild_wait_for_rebuild_checkpoint_increase($self , $rg, $rebuild_checkpoint{$rg_id}, 3600, $passive_sp);
    }
    
    # Step 6: Wait for the panicked SP to come up. 
    $self->platform_shim_test_case_log_step("wait for active sp to come back up");
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    
    # Step 7: Reboot both SPs
    $self->platform_shim_test_case_log_step("reboot both SPs");
    platform_shim_sp_panic($active_sp);
    platform_shim_sp_panic($passive_sp);
    
    # Step 8: Wait for both SPs to come up
    $self->platform_shim_test_case_log_step("wait for both sps to come back up");
    sleep(20);
    platform_shim_sp_wait_for_agent($passive_sp, 3600);
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    
    # Step 9: Verify rebuild continues after dual sp reboot
    $self->platform_shim_test_case_log_step("verify rebuild continues after sps come back");
    foreach my $test_case (@test_cases)
    {
        next if ($test_case->{raid_type} eq 'r0');
        my $rg = $test_case->{raid_group};
        my $rg_id = $test_case->{raid_group_id};
        #$rebuild_percent{$rg_id} = test_util_rebuild_wait_for_rebuild_percent_increase($self , $rg, $rebuild_percent{$rg_id}, 3600);
        $rebuild_checkpoint{$rg_id} = test_util_rebuild_wait_for_rebuild_checkpoint_increase($self , $rg, $rebuild_checkpoint{$rg_id}, 3600);
    }
}

sub rebuild_check_system_rebuild_logging_started
{
    my ($self, $test_case) = @_;
    my $device = $self->{device};
    
    return if (!$test_case->{check_system_rebuilds});
    foreach my $fbe_id (@{$test_case->{system_rg_fbe_ids}}) {
        $fbe_id = sprintf("%#x", $fbe_id) if ($fbe_id !~ /x/i);
        $self->platform_shim_test_case_log_step("Verify rebuild logging started for system raid group $fbe_id");
        platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                            regex => '\(obj '.$fbe_id.'\)'}); 
    } 
}

sub rebuild_check_system_rebuild_logging_stopped
{
    my ($self, $test_case, $b_negative) = @_;
    my $device = $self->{device};
    
    return if (!$test_case->{check_system_rebuilds});
    foreach my $fbe_id (@{$test_case->{system_rg_fbe_ids}}) {
        if ($b_negative) 
        {
            $self->platform_shim_test_case_log_step("Verify rebuild logging is not stopped for system raid group $fbe_id");
            platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                            timeout => '1 M',
                                                            negation => 1,
                                                            regex => '\(obj '.$fbe_id.'\)'}); 
        }
        else
        {
            $self->platform_shim_test_case_log_step("Verify rebuild logging stopped for system raid group $fbe_id");
            platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                                regex => '\(obj '.$fbe_id.'\)'});
        } 
    } 
}

sub rebuild_check_system_rebuild_completed
{
    my ($self, $test_case, $disk) = @_;
    my $device = $self->{device};
    
    return if (!$test_case->{check_system_rebuilds});
    return if (!test_util_disk_is_vault_disk($self, $disk));
    foreach my $fbe_id (@{$test_case->{system_rg_fbe_ids}}) {
        $fbe_id = sprintf("%#x", $fbe_id) if ($fbe_id !~ /x/i);
        $self->logStep("Verify rebuild completed for raid group fbe_id $fbe_id");
        platform_shim_event_verify_rebuild_completed({object => $device, 
                                                      disk => $disk,
                                                      regex => '\(obj '.$fbe_id.'\)'}); 
    } 
}

sub rebuild_cleanup
{
    my ($self) = @_;
    my $device = $self->{device};
    
    # reset the sparing timer
    platform_shim_device_set_sparing_timer($device, 5*60);
    
    platform_shim_device_enable_automatic_sparing($device);
    
}


1;
