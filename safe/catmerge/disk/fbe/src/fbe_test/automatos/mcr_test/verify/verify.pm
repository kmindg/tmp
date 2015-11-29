package mcr_test::verify::verify;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_event;
use mcr_test::test_util::test_util_verify;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_raid_group;
use strict;
use warnings;


sub main
{
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    test_util_general_test_start();
    my $device = $self->{device};

    # Allow small array configurations to still run.   Commenting out following check.
    #test_util_zeroing_wait_min_disks_zeroed($self, 3600);  
    
    my %error_cases = ("FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH" => {lba => 0x0, blocks => 0x1}, 
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC" => {lba => 0x800, blocks => 0x1}, 
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR" => {lba => 0x1000, blocks => 0x1},
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR" => {lba => 0x1800, blocks => 0x1});
    my %rg_states = (0 => "healthy",
                     1 => "degrade");
                     #2 => "broken");                 
                     
    
    $self->platform_shim_test_case_add_to_cleanup_stack({'verify_cleanup' => {}});                     
    # disable load balancing so that all objects are active on the cmi active SP
    platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    platform_shim_device_disable_automatic_sparing($device);

    my @rgs = ();
    if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        # Step 1: Create the raid groups, different widths and drive types, parity
        if ($self->platform_shim_test_case_is_promotion())
        {
            my $rg = test_util_configuration_create_redundant_config($self, {b_zeroed_only => 1});
            push @rgs, $rg;
        } 
        else
        {
            @rgs = @{test_util_configuration_create_all_raid_types($self, {b_zeroed_only => 1})};
        }
    }
    else 
    {
        @rgs = platform_shim_device_get_raid_group($device, {});
    }
 
    # Set up test cases for each raid group
    my @test_cases = ();
    my @luns = ();
    foreach my $rg (@rgs) 
    {
        my $rg_id = platform_shim_raid_group_get_property($rg, "id"); 
        my $raid_type = platform_shim_raid_group_get_property($rg, "raid_type");             
        my $num_disks_to_remove = 0;
        my $b_initial_verify = ($rg_id % 2);
        if ($b_initial_verify && $raid_type =~ /r5|r6|r3|r1/ &&
            $self->platform_shim_test_case_is_functional())
        {
            $num_disks_to_remove = int(rand(scalar(keys %rg_states)));
        }
        
        my $lun;
        if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) { 
            if ($self->platform_shim_test_case_is_promotion())
            {
                $lun = test_util_configuration_create_lun($self, $rg, {initial_verify => $b_initial_verify});
            }
            else
            { 
                $lun = test_util_configuration_create_lun($self, $rg, {initial_verify => $b_initial_verify});
            }
        } 
        else
        {
            ($lun) = platform_shim_raid_group_get_lun($rg);
        }
        push @luns, $lun;
        
        my %test_case = ();
        $test_case{raid_group_id} = $rg_id;
        $test_case{raid_group} = $rg;
        $test_case{lun_id} = platform_shim_lun_get_property($lun, "id");
        $test_case{lun} = $lun;
        $test_case{initial_verify} = $b_initial_verify;
        $test_case{num_disks_to_remove} = $num_disks_to_remove; 
        
        if ($raid_type eq 'r10')
        {
            my @mirrors = @{platform_shim_raid_group_get_downstream_mirrors($rg)};
            $test_case{raid_group_fbe_id} = $mirrors[0];
        } 
        else 
        {
            $test_case{raid_group_fbe_id} = platform_shim_raid_group_get_property($rg, 'fbe_id');
        }
        
        
        push @test_cases, \%test_case;
    }
    
    # Step 2: After zeroing occurs on luns, verify that system verify starts on appropriate luns
    $self->platform_shim_test_case_log_step("Verifying system verify starts if initial verify is enabled");
    foreach my $test_case (@test_cases)
    {
        ##############################
        if (platform_shim_device_is_unified($self->{device}) && 
            platform_shim_device_is_simulator($self->{device})) {
            foreach my $disk (platform_shim_raid_group_get_disk($test_case->{raid_group})) {
                platform_shim_disk_enable_zeroing($disk);
            }
        }

        test_util_zeroing_wait_for_lun_zeroing_percent($self, $test_case->{lun}, 100, 3600);
        if ($test_case->{inital_verify})
        {
            platform_shim_event_verify_system_verify_started({object => $self->{device}, lun => $test_case->{lun}});
        } 
        else 
        {
            platform_shim_event_verify_system_verify_started({object => $self->{device},
                                                              lun => $test_case->{lun},
                                                              negation => 1,
                                                              timeout => '30S'});
        }
    }
    
    
    # Step 3: Wait for system verify to finish. Check event logs for appropriate messages. 
    $self->platform_shim_test_case_log_step("Verify system verify completes on appropriate luns ");
    foreach my $test_case (@test_cases)
    {
        if ($test_case->{inital_verify})
        {
            platform_shim_event_verify_system_verify_completed({lun => $test_case->{lun}});
        } 
    }

    foreach my $test_case (@test_cases) 
    {
        my $temprg = $test_case->{raid_group};
        $self->platform_shim_test_case_log_info("START testing on ".$test_case->{raid_group_id}. " ". platform_shim_raid_group_get_property($temprg, "raid_type"));
        
        $test_case->{lun_bv_pass_count} = test_util_verify_get_lun_total_bv_pass_count($self, $test_case->{lun});
        
        foreach my $sp (platform_shim_device_get_sp($device))
        {
            test_util_general_set_target_sp($self, $sp);
       
            # disable logical error injection
            platform_shim_device_disable_logical_error_injection($device);
            platform_shim_device_disable_logical_error_injection_records($device);
        
            # Step 4: Setup error injection
            $self->platform_shim_test_case_log_step("Set up logical error injection");
            foreach my $error_type (keys %error_cases) 
            {
                my $error_case = $error_cases{$error_type};
                my %lei_params = (active_sp            => $sp,
                                  error_injection_type => $error_type,
                                  error_mode           => 'FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS',
                                  error_count          => 0, 
                                  error_limit          => 1, 
                                  error_bit_count      => 1,
                                  lba_address          => $error_case->{lba}, 
                                  blocks               => $error_case->{blocks});
                platform_shim_device_create_logical_error_injection_record($device, \%lei_params);
            }
        
            # Enable logical error injection
            platform_shim_device_enable_logical_error_injection($device);
        }
        
    
        my $rg = $test_case->{raid_group};
        my @disks = platform_shim_raid_group_get_disk($rg);
        my $num_disks_to_remove = $test_case->{num_disks_to_remove};
        my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
        my @faults = ();
        if ($num_disks_to_remove)
        {
            # Step 5: Degrade, break, and healthy raid groups
            $self->platform_shim_test_case_log_step("Degrade some raid groups");
            $num_disks_to_remove++ if ($raid_type eq 'r6') ;
            # set the target to the passive sp so the disk is removed
            # on the passive side first and activeness remains in tact
            test_util_general_set_target_sp($self, $passive_sp);
            foreach (1..$num_disks_to_remove)
            {
                my $disk_to_remove = shift @disks;
                my $fault = platform_shim_disk_remove($disk_to_remove); 
                push @faults, $fault;
            }
            $test_case->{faults} = \@faults;
            test_util_general_set_target_sp($self, $active_sp);
        }
        
        foreach my $sp (platform_shim_device_get_sp($device))
        {
            test_util_general_set_target_sp($self, $sp);
            platform_shim_device_enable_logical_error_injection_fbe_object_id($device, $test_case->{raid_group_fbe_id});
        }

        # Step 6: Start rw bgv for lun. 
        $self->platform_shim_test_case_log_step("Start rw background verify");
    
        #  Verify pass counts and stats are correct for healthy raid group 
        #  but verify does not run on unhealthy raid group
        if ($test_case->{num_disks_to_remove}) 
        {
            my $max_retries = 3;
            my $num_retries = 0;
            my $need_retry = 1; 
            while ($need_retry)
            {
                platform_shim_lun_initiate_rw_background_verify($test_case->{lun});
                eval
                {
                    test_util_verify_wait_for_group_user_verify_checkpoint_reset($self, $rg, 600);
                };
                if ($@ && $num_retries < $max_retries) {
                    $need_retry = 1;
                    $num_retries++;
                } elsif ($@) {
                    # ran out of retries
                    $need_retry = 0;
                    platform_shim_test_case_throw_exception($@);
                } else {
                    $need_retry = 0;
                }
            }
            platform_shim_event_verify_rw_verify_started({object => $self->{device},
                                                          lun => $test_case->{lun},
                                                          negation => 1,
                                                          timeout => '30S'});
        }
        else
        {
            platform_shim_lun_initiate_rw_background_verify($test_case->{lun});
            platform_shim_event_verify_rw_verify_started({object => $self->{device}, lun => $test_case->{lun}});
        }
   
    
        
        if ($test_case->{num_disks_to_remove}) {
            # Step 7: Restore the degraded and broken raid groups. Verify will automatically start 
            #  and pass counts and stats are appropriate for injected errors
            platform_shim_test_case_log_info($self, "Restore degraded and broken raid groups");
            my @faults = @{$test_case->{faults}};
            foreach my $fault (@faults)
            {
                test_util_general_set_target_sp($self, $active_sp);
                platform_shim_disk_reinsert($fault);
                
            }
            # wait for rebuild to complete
            test_util_rebuild_wait_for_rebuild_complete($self, $test_case->{raid_group}, 3600);
            platform_shim_event_verify_rw_verify_started({object => $self->{device}, lun => $test_case->{lun}});
            
        } 
        
        $self->platform_shim_test_case_log_info("Verify errors were injected");
        my $num_injected = 0;
        foreach my $sp (platform_shim_device_get_sp($device))
        {
            $num_injected += platform_shim_sp_get_property($sp, 'error_injected_count');
        }
        $self->platform_shim_test_case_assert_true_msg(($num_injected != 0), "Errors were not injected");
        #my $rg_fbe_id = sprintf("%x", $test_case->{raid_group_fbe_id});
        
        foreach my $sp (platform_shim_device_get_sp($device))
        {
            test_util_general_set_target_sp($self, $sp);
            platform_shim_device_disable_logical_error_injection_fbe_object_id($device, $test_case->{raid_group_fbe_id});
        }
    
        # Step 8: Verify rw verify completed
        $self->platform_shim_test_case_log_step("Verify RW verify completed on all luns");
        test_util_verify_wait_for_lun_rw_bv_complete($self, $test_case->{lun}, 3600);
        platform_shim_event_verify_rw_verify_completed({object => $self->{device}, lun => $test_case->{lun}});
        my $lun_bv_pass_count = test_util_verify_get_lun_total_bv_pass_count($self, $test_case->{lun});    
        $self->platform_shim_test_case_assert_true_msg(($lun_bv_pass_count > $test_case->{lun_bv_pass_count}), "BV pass count did not incrememnt: $lun_bv_pass_count");  
        
        
    
        # Step 9&10: Verify the event log messages for errors found with verify 
        $self->platform_shim_test_case_log_step("Verify stats & event log messages");
        my $active_rg_sp = platform_shim_device_get_active_sp($device, {object => $test_case->{lun}});
        test_util_general_set_target_sp($self, $active_rg_sp); 
        my %lun_bv_report = platform_shim_lun_get_bv_report($test_case->{lun});
        
        my %before = %{$lun_bv_report{$test_case->{lun_id}}{recent_report}};
        my %after = %{$lun_bv_report{$test_case->{lun_id}}{total_report}};
        my $rg_fbe_id =  sprintf("%x", $test_case->{raid_group_fbe_id});
        foreach my $error_key (keys %error_cases)
        {
            my $error_case = $error_cases{$error_key};
            my $lba = sprintf("%x", $error_case->{lba});
            my $blocks = sprintf("%x", $error_case->{blocks});
            
            if ($error_key eq 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR')
            {
                my $before = $lun_bv_report{$test_case->{lun_id}}{recent_report}{softmedia_errors}{correctable};
                my $after = $lun_bv_report{$test_case->{lun_id}}{total_report}{softmedia_errors}{correctable};
                $self->platform_shim_test_case_assert_true_msg(($before < $after), "soft media errors did not increase $before $after");
            } 
            elsif ($error_key eq 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR' && 
                    $raid_type ne 'r0')
            {
                $self->platform_shim_test_case_log_info("check media error type: $raid_type lba: $lba blocks: $blocks");
                test_util_event_wait_for_media_error_events($self, {raid_group_fbe_id => $rg_fbe_id, lba => $lba});
                my $before = $lun_bv_report{$test_case->{lun_id}}{recent_report}{media_errors}{correctable};
                my $after = $lun_bv_report{$test_case->{lun_id}}{total_report}{media_errors}{correctable};
                $self->platform_shim_test_case_assert_true_msg(($before < $after), "media errors did not increase $before $after");
            }
            elsif ($error_key eq 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH' && 
                     $raid_type ne 'r0')
            {
                $self->platform_shim_test_case_log_info("check coherency error type: $raid_type lba: $lba blocks: $blocks");
                if ($raid_type eq 'r6')
                {
                    test_util_event_wait_for_soft_media_error_events($self, {raid_group_fbe_id => $rg_fbe_id, lba => $lba, blocks => $blocks});
                } 
                else
                {
                    test_util_event_wait_for_coherency_error_events($self, {raid_group_fbe_id => $rg_fbe_id, lba => $lba, blocks => $blocks});
                }
                my $before = $lun_bv_report{$test_case->{lun_id}}{recent_report}{coherency_errors}{correctable};
                my $after = $lun_bv_report{$test_case->{lun_id}}{total_report}{coherency_errors}{correctable};
                $self->platform_shim_test_case_assert_true_msg(($before < $after), "coherency errors did not increase $before $after");
            }
            elsif ($error_key eq 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC' && 
                     $raid_type ne 'r0')
            {
                $self->platform_shim_test_case_log_info("check checksum error type: $raid_type lba: $lba blocks: $blocks");
                test_util_event_wait_for_checksum_error_events($self, {raid_group_fbe_id => $rg_fbe_id, lba => $lba, blocks => $blocks});
                my $before = $lun_bv_report{$test_case->{lun_id}}{recent_report}{checksum_errors}{correctable};
                my $after = $lun_bv_report{$test_case->{lun_id}}{total_report}{checksum_errors}{correctable};
                $self->platform_shim_test_case_assert_true_msg(($before < $after), "checksum errors did not increase $before $after");
            }
        }  
    }
 
    my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($skip_reboot != 1) {
        $self->verify_with_sp_faults(\@rgs);
    }
    
    test_util_general_log_test_time($self);
}

sub verify_with_sp_faults
{
    my $self = shift;
    my $rg_ref = shift;
    my @rgs = @{$rg_ref};
    my $device = $self->{device};
    my $lun_capacity_gb = 5;
    
    return if ($self->platform_shim_test_case_is_promotion());
    
    # Step 1a: Disable load balancing - get cmi active SP
    platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    my %bv_pass_counts = ();
    
    # Step 1: Create the raid groups, different widths and drive types, parity
    #  Create luns, no initial verify luns ~ 100GB
    #  Verify the active SP for the raid groups are the cmi active SP
    # TODO: make this more intelligent
    my @luns = ();

    foreach my $rg (@rgs)
    {
        my $disk_bind_size = 50;
        my @disks = platform_shim_raid_group_get_disk($rg);
        
        if (platform_shim_disk_get_property($disks[0], "kind") =~ /FLASH/i)
        {
            $disk_bind_size = platform_shim_disk_get_property($disks[0], "user_capacity");
            my $lun_size_gb = test_util_configuration_get_data_disks($self, $rg) * $disk_bind_size;
            my $lun = test_util_configuration_create_lun($self, $rg, {size => $lun_size_gb."BC", initial_verify => 0});
            push @luns, $lun;
            next;
        }
        
        if (platform_shim_disk_get_property($disks[0], "kind") =~ /NL_SAS/i) 
        {
            $disk_bind_size = 10;
        }
                
        my $lun_size_gb = test_util_configuration_get_data_disks($self, $rg) * $disk_bind_size;
        my $lun = test_util_configuration_create_lun($self, $rg, {size => $lun_size_gb."GB", initial_verify => 0});
        push @luns, $lun;
    }
    
    foreach my $lun (@luns)
    {
        test_util_zeroing_wait_for_lun_zeroing_percent($self, $lun, 100, 3600);
    }

    
    # Step 2: Start io on luns
    platform_shim_device_initialize_rdgen($device);
    my %io_params = (thread_count => 4,
                     block_count => 4000,
                     available_for_writes => 1,
                     write_only=> 1);
    platform_shim_sp_start_rdgen($active_sp, \%io_params);                 
                     
    # Step 3: Start RW Backrgound verify on entire system. Verify bv checkpoint is increasing
    foreach my $sp (platform_shim_device_get_sp($device))
    {
        platform_shim_device_set_target_sp($device, $sp);
        platform_shim_device_initiate_rw_background_verify($device);
    }
    platform_shim_device_set_target_sp($device, $active_sp);
    my %bv_percent = ();
    foreach my $lun (@luns) {
        my $rw_verify = platform_shim_lun_get_rw_verify_state($lun);
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        $bv_percent{$lun_id} = $rw_verify->{background_verify_pct};
        $bv_pass_counts{$lun_id} = test_util_verify_get_lun_total_bv_pass_count($self, $lun);
    }
    
    # Step 4: Reboot the active SP. 
    $self->platform_shim_test_case_log_step("reboot active sp");
    platform_shim_sp_panic($active_sp);
    # Step 5: Verify that bv continues on the passive SP. (passive becomes active) 
    $self->platform_shim_test_case_log_step("verify that bv continues on passive sp");
    test_util_general_set_target_sp($self, $passive_sp);
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        $bv_percent{$lun_id} = test_util_verify_wait_for_rw_verify_percent_increase($self, $lun, $bv_percent{$lun_id}, 3600);
    }
    # Step 6: Wait for the panicked SP to come up. 
    $self->platform_shim_test_case_log_step("wait for active sp to come back up");
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        test_util_verify_wait_for_lun_rw_bv_complete($self, $lun, 3600);
        my $bv_pass_count = test_util_verify_get_lun_total_bv_pass_count($self, $lun);
        $self->platform_shim_test_case_assert_true_msg($bv_pass_count > $bv_pass_counts{$lun_id}, "BV pass count did not incrememnt: $bv_pass_count");
    }
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    
    # Step 6.2 restart rw verify
    foreach my $sp (platform_shim_device_get_sp($device))
    {
        platform_shim_device_set_target_sp($device, $sp);
        platform_shim_device_initiate_rw_background_verify($device);
    }
    platform_shim_device_set_target_sp($device, $active_sp);
    my %bv_percent = ();
    foreach my $lun (@luns) {
        my $rw_verify = platform_shim_lun_get_rw_verify_state($lun);
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        $bv_percent{$lun_id} = $rw_verify->{background_verify_pct};
        $bv_pass_counts{$lun_id} = 0;
    }
    
    # Step 7: Panic both SPs
    $self->platform_shim_test_case_log_step("reboot both sps");
    platform_shim_sp_panic($active_sp);
    platform_shim_sp_panic($passive_sp);
    
    # Step 8: Wait for both SPs to come up
    platform_shim_sp_wait_for_agent($passive_sp, 3600);
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    
    # Step 9: Verify bv continues and completes
    $self->platform_shim_test_case_log_step("verify bv continues after dual sp reboot");
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        $bv_percent{$lun_id} = test_util_verify_wait_for_rw_verify_percent_increase($self, $lun, $bv_percent{$lun_id}, 3600);
    }
    
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, "id");
        test_util_verify_wait_for_lun_rw_bv_complete($self, $lun, 3600);
        my $bv_pass_count = test_util_verify_get_lun_total_bv_pass_count($self, $lun);
        $self->platform_shim_test_case_assert_true_msg($bv_pass_count > $bv_pass_counts{$lun_id}, "BV pass count did not incrememnt: $bv_pass_count");
    }
    
    # Step 10: Verify proper event log messages for bv start and finish. 
    $self->platform_shim_test_case_log_step("verify bv started and completed events");
    foreach my $lun (@luns)
    {
        platform_shim_event_verify_rw_verify_started({object => $self->{device}, lun => $lun});
        platform_shim_event_verify_rw_verify_completed({object => $self->{device}, lun => $lun});
    }
    
    # Free rdgen memory
    platform_shim_device_cleanup_rdgen($device);

    # Step 11: Re-enable load balancing
    platform_shim_device_enable_load_balancing($device);
 
}

sub verify_cleanup
{
    my $self = shift;
    my $device = $self->{device};
    
    # enable automatic sparing
    platform_shim_device_enable_automatic_sparing($device);
    
    # disable logical error injection
    platform_shim_device_disable_logical_error_injection($device);
    platform_shim_device_disable_logical_error_injection_records($device);
    
    # restore load balancing
    platform_shim_device_enable_load_balancing($device);
}


1;
