package mcr_test::encryption::encryption_tests;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_copy;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_io;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::test_util::test_util_encryption;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_raid_group;
use strict;
use warnings;

# Global variables
our $b_rdgen_need_to_stop_io_before_starting_more = 1; # @todo Currently we need to stop I/O before starting any new I/O

# ==================Encryption Test Cases===========================================
#  Create raid group of each type with available drives.
#  Enable encryption
#  Bind (3) 10GB LUNs on each raid group.
#  Start `light' I/O using rdgen
#  Initiate proactive copy on one drive in each redundant raid group
#  Validate encryption pauses
#  Validate proactive copy completes
#  Validate encryption completes
#  
#   Common Tasks for ALL test Levels:
#   =================================
#   Task 1: Create raid groups for all raids types including using one or
#           more system drives for (1) redundant raid group.
#      
#   Task 2: Determine the capacity and number of LUNs for each raid group
#           based on platform and test level.
#      
#   Task 3: Initialize the raid groups configurations and bind the LUNs. This
#           includes unbinding a LUN to create a `hole'.  The actual number of
#           remaining LUNs bound is in $luns_per_rg.
#      
#   Task 4: Write a background pattern to all LUNs on the passive SP.
#   
#   Task 5: Start READ Only I/O on all the LUNs from the passive SP.
#   
#   Task 6: Set EOL on a raindom position in each raid group.
#   
#   Task 7: Stop read only I/O on all LUNs
#   
#   Test 8: Validate the background pattern on all remaining LUNs.
#   
#   Task 9: Clear any debug flags and cleanup rdgen
#   
#   Task 10: Clear EOL on source drives and perform any other cleanup.
#   
#   Common Tests for ALL test Levels:
#   =================================
#   Test 2: Verify proactive copy starts on all redundant raid groups.
#   
#   Test 4: Confirm that the copy operation is not started on non-redundant
#           raid types.  Remove the non-redundant raid group from the
#           raid groups under test (I/O will still be sent to the 
#           non-redundant LUNs).
#           
#   Test 5: Validate that the still active copy operations make progress.
#   
#   Test 8: Remove the destination drive from active copy virtual drives
#           and confirm that the copy does not proceed.
#           
#   Test 9: Re-insert the destination drive and confirm that the copy proceeds.
#   
#   Functional Tests:
#   =================
#   Test 1: Reboot the Active SP (I/O continues since ti was started on the Passive SP).
#   
#   Test 3: Set DRIVE_FAULT on the source drive for some number of copy drives.
#           confirm that the copy operation is immediately aborted and that the
#           copy position is marked rebuild logging.
#           
#   Test 6: For raid groups with copy in-progress, bind a new LUN. Write the
#           background pattern to the LUN and start read only I/O to LUN.
#           
#   Test 7: Reboot the passive SP.
#   
#   Test 10: Stop read only I/O on the previously bound additional LUN.  Validate
#            the background pattern then unbind it.
#            
#   Test 11: Stop read only I/O on all LUNS, reboot BOTH SPs.  Wait for BOTH SPs
#            to be ready.  Start read only I/O on all LUNs.
#            
#   Test 12: Wait for all active copy operations to complete.  Wait for rebuilds
#            (for those raid groups where the source drive was removed) to complete.
#   
# ======================================================================================
sub main
{
    my ($self) = @_;
    # Get the test start time
    test_util_general_test_start();
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    my $device = $self->{device};
    my $consumed_physical_capacity_gb = 10;
    my $io_physical_percent = 50;
    my $luns_per_rg     = 3;                  
    my $lun_capacity_gb = 10;
    my $lun_io_threads = 4;
    my $encryption_progress_timeout       = 1 * 60;       # 1 minutes is the default copy progress timeout
    my $no_encryption_progress_timeout    = 10;           # 10 seconds
    my $encryption_aborted_timeout        = 2 * 60;       # 2 minutes
    my $encryption_complete_timeout       = 4 * 60 * 60;  # 4 hours
    my $b_check_pattern = 0;                        # By default don't check the background pattern after writing it
    # @todo Currently the `wait for event' does not handle the case where the
    #       event may have occurred on the peer.  Therefore we must wait for
    #       the eer to be ready after a reboot.
    my $b_wait_for_sp_ready_after_reboot = 1;
    my $status = 0;
    my $task = 1;
    my $test = 1;
    my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($skip_reboot == 1) {
       print "Skiping reboot during test since skip_reboot parameter set.\n";
    }
    my $host_audit_log_path = $self->platform_shim_test_case_get_parameter(name => "host_audit_log_path");
    $self->{rg_config_ref} = ();

    my %error_cases = ("FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH" => {lba => 0x0, blocks => 0x80, stats_name => 'Current Correctable Coherency'}, 
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC" => {lba => 0x400, blocks => 0x80, stats_name => 'Current Correctable Single Bit Crc'}, 
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR" => {lba => 0x800, blocks => 0x80, stats_name => 'Current Correctable Media'},
                       "FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR" => {lba => 0xC00, blocks => 0x80, stats_name => 'Current Correctable Soft Media'});
    my %rg_states = (0 => "healthy",
                     1 => "degrade",
                     2 => "broken");                 
              
    # Add the cleanup (invoked BOTH when test passes and fails)
    $self->platform_shim_test_case_add_to_cleanup_stack({'encryption_cleanup' => {}});   

    # We do NOT disable load balancing since we want some parent raid groups NOT
    # on the CMI Active SP
    #platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    my $io_sp = $active_sp;
    test_util_general_set_target_sp($self, $active_sp);

    # Define the SP to run I/O based on the test level
    if (platform_shim_test_case_is_functional($self)) {
        $io_sp = $passive_sp;
        $encryption_progress_timeout = 2 * 60;    # Copy progress timeout is 2 minutes
        $b_check_pattern = 0; # Takes too long, skip it for now.
    }

    # Task 1: Enable encryption
    $status = encryption_enable_encryption($self, $task++, $encryption_progress_timeout);
    if ($status != 0) {
        # Can't proceed if encyption isn't enabled
        $self->platform_shim_test_case_assert_true_msg(($status == 0), "Failed to enable encryption");
    }

    # Task 2: Create the raid groups, different widths and drive types, parity
    #         Currently we check and set the raid group debug flags
    my @rgs = @{encryption_create_raid_groups($self, $task++)};
    test_util_io_set_raid_group_debug_flags($self, $io_sp, \@rgs, 1); 
    my $rg_count = scalar(@rgs);

    # Task 3: Validate no error traces
    $self->platform_shim_test_case_log_step("Validate no error traces are present");
    platform_shim_device_check_error_trace_counters($self->{device});

    # Task 4: Based on the platform under test configure the following:
    #           o $luns_per_rg
    #           o $consumed_physical_capacity_gb
    #           o $io_physical_percent
    #           o $lun_io_threads
    encryption_configure_params_based_on_platform($self, $task++, \@rgs, 
                                                      \$luns_per_rg,
                                                      \$consumed_physical_capacity_gb,
                                                      \$io_physical_percent,
                                                      \$lun_io_threads);

    # Task 5: Set up test raid group config for each raid group
    my @rg_configs = @{encryption_initialize_rg_configs($self, $task++, \@rgs, $active_sp, 
                                                            \$luns_per_rg,
                                                            $consumed_physical_capacity_gb,
                                                            $io_physical_percent)};
    $self->{rg_config_ref} = \@rg_configs;
    
    # Task 6: Write background pattern to all LUNs
    #         We need to limit the amount of data copied but at the same time
    #         make sure the copy proceeds slowly.  Therefore the value for
    #         $lun_io_max_lba is crucial.  The goal is to have all the copy
    #         operations take approximately 10 minutes.
    $status = encryption_write_background_pattern_to_all_luns($self, $task++, \@rg_configs, $io_sp,
                                                                  $io_physical_percent,
                                                                  $b_check_pattern);

    # Task 7: Start read only I/O on all LUNs
    $status = encryption_start_initial_read_only_io($self, $task++, \@rg_configs, $io_sp,
                                                    $lun_io_threads);

    # @note We no longer slow down the encryption speed (it doesn't seem to work
    #       anyway !!)

    # @note Copy and rekey CANNOT run at the same time !!!
    #       Therefore we MUST start the copy operation BEFORE the rekey !!

    # Task 8: Set EOL on random positions
    $status = encryption_set_eol_on_redundant_raid_groups($self, $task++, \@rg_configs);

    # Test 1: Verify that Proactive Copy is started on each raid group (except RAID-0)
    $status = encryption_test_verify_copy_starts_encryption_paused($self, $test++, \@rg_configs,
                                                                   $no_encryption_progress_timeout);

    # Test 2: If this is a functional test level reboot the CMI Active SP.
    #           o Validate copy starts on all raid groups
    #           o Stop I/O
    #           o Wait for the copy to complete
    #           o Clear EOL
    if (platform_shim_test_case_is_functional($self)) {
        $status = encryption_test_reboot_active_after_copy_start($self, $test++, \@rg_configs, $active_sp,
                                                                     \$active_sp, \$passive_sp, 
                                                                     $b_wait_for_sp_ready_after_reboot,
                                                                     $encryption_complete_timeout,
                                                                     $lun_io_threads);
    }

    # Test 3: Start rekey
    $status = encryption_test_start_rekey($self, $test++, \@rg_configs);
    if ($status != 0) {
        # Can't proceed if encyption isn't rinning
        $self->platform_shim_test_case_assert_true_msg(($status == 0), "Failed to start rekey");
    }

    # Test 4: Make sure rekey makes progress
    $status = encryption_test_verify_rekey_progress($self, $test++, \@rg_configs, $encryption_progress_timeout);

    # Test 5: If this is a functional test:
    #           o Wait for previously rebooted original active to be ready
    #           o Reboot the currently passive SP
    if (platform_shim_test_case_is_functional($self)) {
        $status = encryption_test_reboot_passive_during_encryption($self, $test++, \@rg_configs, $passive_sp, 
                                                                   \$active_sp, \$passive_sp, 
                                                                   $b_wait_for_sp_ready_after_reboot,
                                                                   $encryption_progress_timeout,
                                                                   $lun_io_threads);
    }

    # Test 6: If this is a functional test:
    #           o Bind a new LUN with rekey in-progress
    #           o Verify the rekey progresses
    #           o Write the background pattern to the LUN
    #           o Re-start read only I/O on new LUN
    if (platform_shim_test_case_is_functional($self)) {
        $status = encryption_test_bind_lun_verify_rekey_progress($self, $test++, \@rg_configs, $io_sp, 
                                                                 $lun_io_threads, $encryption_progress_timeout);
    }

    # Test 7: Remove drive from raid group and confirm no encryption progress
    $status = encryption_test_remove_drive_and_verify_no_progress($self, $test++, \@rg_configs, $no_encryption_progress_timeout);

    # Test 8: Re-insert the destination drive and verify that the copy progresses
    $status = encryption_test_reinsert_dest_verify_progress($self, $test++, \@rg_configs, $encryption_progress_timeout);

    # Test 9: If this is a functional test:
    #           o Stop I/O to newly bound LUN
    #           o Verify the background pattern
    #           o Unbind the LUN
    #           o Verify copy proceeds
    if (platform_shim_test_case_is_functional($self)) {
        $status = encryption_test_unbind_lun_verify_copy_progress($self, $test++, \@rg_configs, $io_sp, 
                                                                      $lun_io_threads,
                                                                      $encryption_progress_timeout);
    }

    # Test 10: If this is a functional test:
    #           o Wait for previously rebooted passive to be ready
    #           o Stop I/O
    #           o Reboot BOTH SPs
    #           o Wait for the new active to be ready
    #           o Start I/O on the active
    if (platform_shim_test_case_is_functional($self)) {
        $status = encryption_test_reboot_both_sps_and_start_io($self, $test++, \@rg_configs, $io_sp,
                                                                   \$active_sp, \$passive_sp,
                                                                   $lun_io_threads,
                                                                   $b_wait_for_sp_ready_after_reboot);
    }

    # Test 11: If this is a functional test level wait for either the parent
    #          raid group rebuild to complete (occurs immediately since DRIVE_FAULT
    #          does not wait for trigger spare timer) or the copy to complete.
    if (platform_shim_test_case_is_functional($self)) {
        # For those raid groups without source faulted, wait for copy to complete
        $status = encryption_test_wait_for_copy_and_rebuild_to_complete($self, $test++, \@rg_configs, $encryption_complete_timeout);
    }
   
    # Task 8: Stop read only I/O on all LUNs
    $status = encryption_stop_all_io_after_test_complete($self, $task++, $io_sp);

    # Task 9: Validate data pattern
    $status = encryption_check_background_pattern_after_test_complete($self, $task++, \@rg_configs, $io_sp);

    # Task 10: Clear any debug flags and cleanup rdgen
    $status = encryption_cleanup_debug_and_rdgen($self, $task++, \@rgs, $active_sp, $passive_sp);

    # Task 11: Clear EOL and DRIVE_FAULT on the source drives
    $status = encryption_cleanup_after_test_complete($self, $task++, \@rg_configs);

    # disable logical error injection
    #platform_shim_device_disable_logical_error_injection($device);
    
    # restore load balancing
    #platform_shim_device_enable_load_balancing($device);

    #$self->platform_shim_test_case_log_step("Validate no error traces are present");
    #platform_shim_device_check_error_trace_counters($self->{device});

    # Log total test time
    test_util_general_log_test_time($self);
}

#=
#           encryption_enable_encryption
#
# @brief    Enable encryption on the array under test
#
# @param    $self - Main argument pointer
# @param    $task - The task number
# @param    $encryption_complete_timeout - How many seconds to wait for vault to encrypt
#
# @note     All the matching text is from the FBE_CLI> encrypt -status/-mode 
#                                   AND
#           naviseccli securedata -feature -info
# @return   $status
#
#=cut
sub encryption_enable_encryption
{
    my ($self, $task, $encryption_complete_timeout) = @_;
     
    my $device = $self->{device};
    my $status = 0;
    my $encryption_supported;
    my $encryption_mode;
    my $encryption_status;
    my $encryption_enabler = $self->platform_shim_test_case_get_parameter(name => "encryption_enabler");

    $self->platform_shim_test_case_log_step("Task $task: Activating CBE Encryption");
    $encryption_supported = test_util_encryption_is_encryption_supported($self);
    $encryption_mode = test_util_encryption_get_encryption_mode($self);
    $encryption_status = test_util_encryption_get_encryption_status($self);

    # Install enabler if required
    if ($encryption_supported =~ /No/) {
        $self->platform_shim_test_case_log_info("Encryption enabler not installed");
        if ($encryption_enabler eq '') {
            $self->platform_shim_test_case_assert_msg(0, "Please supply -encyption_enabler to command line");
        }
        $status = test_util_encryption_install_enabler($self, $encryption_enabler);
        if ($status != 0) {
            $self->platform_shim_test_case_log_error("Failed to install enabler");
            return $status;            
        }
    }

    # Activate encryption if required
    if ($encryption_status =~ /Encryption mode not enabled/) {
        # Make sure user security is setup if needed
        $status = test_util_encryption_add_user_security($self);
        if ($status != 0) {
            $self->platform_shim_test_case_log_error("Failed to add user security");
            return $status;            
        }
        # Need to activate encryption
        $status = test_util_encryption_activate_encryption($self);
        if ($status != 0) {
            $self->platform_shim_test_case_log_error("Failed to activate encryption");
            return $status;            
        }
    }

    # Wait for the vault to encrypted
    $encryption_mode = test_util_encryption_get_encryption_mode($self);
    $encryption_status = test_util_encryption_get_encryption_status($self);
    $self->platform_shim_test_case_log_info("Enable encryption: mode: $encryption_mode status: $encryption_status");
    $self->platform_shim_test_case_log_info("Enable encryption: Wait for vault to be encrypted");
    $status = test_util_encryption_wait_for_encryption_complete($self, $encryption_complete_timeout);
    if ($status != 0) {
        $self->platform_shim_test_case_log_error("Failed to encrypt vault");
        return $status;            
    }
    
    # Return $status
    return $status;
}
# end encryption_enable_encryption

#=
#           encryption_create_raid_groups
#
# @brief    Create raid groups for proactive copy.  This includes (1) raid group
#           that uses one or more system drives.
#
# @param    $self - Main argument pointer
# @param    $task - The task number
#
# @return   \@rgs - Address of array of raid groups created.
#
#=cut
sub encryption_create_raid_groups
{
    my ($self, $task) = @_;

    my $device = $self->{device};
    my @rgs = ();

    # Log this step
    $self->platform_shim_test_case_log_step("Task $task: Create raid groups, one with system drives");

    # If we are using the existing raid groups just return those
    if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        # Get the existing raid groups
        @rgs = platform_shim_device_get_raid_group($device, {});
    } elsif (platform_shim_test_case_is_functional($self)) {
        # Else if functional test create raid groups of all types
        @rgs = @{test_util_configuration_create_all_raid_types_with_system_drives($self)};
        #@rgs = @{test_util_configuration_create_r1_only($self)};
    } else {
        @rgs = @{test_util_configuration_create_limited_raid_types_with_system_drives($self)};
    }

    # Return the address of the raid groups created
    return \@rgs;
}
# end encryption_create_raid_groups

#=
#*          encryption_initialize_rg_configs
#*
#* @brief   Create and initialize test cases using the raid groups passed.
#*          The LUNs must be created thru cache since encryption requires
#*          the LUN data to be cached during the encryption request.
#*
#* @param   $self - Main argument pointer
#* @param   $task - The current task of the test case 
#* @param   $rgs_ref  - Address of array of raid group configurations under test
#* @param   $luns_per_rg_ref - Address of LUNs per raid group will be updated
#* @param   $consumed_physical_capacity_gb - The amount of physical capacity to bind
#*              on each raid group.
#* @param   $io_physical_percent - Percent of physical to send I/O to
#*
#* @return  \@rg_configs - Address of array of test cases created.
#*
#=cut 
sub encryption_initialize_rg_configs
{
    my ($self, $task, $rgs_ref, $sp, $luns_per_rg_ref, $consumed_physical_capacity_gb, $io_physical_percent) = @_;

    $self->platform_shim_test_case_log_step("Task $task: Initialize raid group configs from array of raid groups");
    my @rgs = @{$rgs_ref};
    my @rg_configs = ();
    foreach my $rg (@rgs) {
        my $rg_id = platform_shim_raid_group_get_property($rg, 'id'); 
        my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
        my $width = platform_shim_raid_group_get_property($rg, 'raid_group_width'); 
        my @disks = platform_shim_raid_group_get_disk($rg);
        my $data_disks = test_util_configuration_get_data_disks($self, $rg);
        my $requested_logical_capacity_gb = $consumed_physical_capacity_gb * $data_disks;
        my $logical_capacity_available = platform_shim_raid_group_get_logical_capacity($self, $rg, $rg_id);
        my $logical_capacity_available_gb_bi = int($logical_capacity_available / 2097152);
        my $logical_capacity_available_gb = $logical_capacity_available_gb_bi->{value}->[0];
        my $eol_position = int(rand($width));
        my $remove_position = int(rand($width));
        my @luns = ();
        my $lun = 0;
        my $luns_per_rg = $$luns_per_rg_ref;
        my $lun_capacity = 0;
        my $lun_capacity_gb = int($requested_logical_capacity_gb / $luns_per_rg);
        my $lun_io_max_lba = 0;
        my $lun_io_max_lba_hex = 0;
            
        # Remove should be different than copy
        foreach my $position (1..$width) {
            if (($remove_position == $eol_position)  &&
                (($position -1) == $remove_position)    ) {
                if ($remove_position < ($width - 1)) {
                    $remove_position++;
                } else {
                    $remove_position = 0;
                }
                last;
            }
        }

        # If the raid groups doesn't have sufficient space change the desired
        # lun capacity
        if ($requested_logical_capacity_gb > $logical_capacity_available_gb) {
            my $available_lun_capacity_gb = int($logical_capacity_available_gb / $luns_per_rg);
            $self->platform_shim_test_case_log_info("Bind LUNs rg id: $rg_id capacity: $logical_capacity_available_gb GB" .
                                                    " is less than desired: $requested_logical_capacity_gb GB");
            $self->platform_shim_test_case_log_info("          Reduce lun size from: $lun_capacity_gb GB" .
                                                    " to: $available_lun_capacity_gb GB");
            $lun_capacity_gb = $available_lun_capacity_gb;
        }

        # platform_shim_raid_group_get_property
        # If `clean_config' is not set then use the exist luns
        if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
            # Use existing luns
            @luns = platform_shim_raid_group_get_lun($rg);
            my $num_luns = scalar(@luns);
            $lun_capacity = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
            $lun_capacity_gb = int($lun_capacity / 2097152);
            $self->platform_shim_test_case_log_info("Bind LUNs rg id: $rg_id using exiting: $num_luns LUNs" .
                                                    " with capacity: $lun_capacity_gb GB");
            while ($num_luns < $luns_per_rg) {
                my $lun = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
                push @luns, $lun;
                $num_luns++;
            }
            while ($num_luns > $luns_per_rg) {
                $lun = pop @luns;
                platform_shim_lun_unbind($lun);
                $num_luns--;
            }

        } else {
            # Bind `luns_per_rg'
            $self->platform_shim_test_case_log_info("Bind: $luns_per_rg LUNs on rg id: $rg_id" .
                                                    " with capacity: $lun_capacity_gb GB");
            foreach (1..$luns_per_rg) {
                my $lun = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
                push @luns, $lun;
            }
        }

        # Now unbind the second lun
        my $lun_id = platform_shim_lun_get_property($luns[1], 'id');
        $self->platform_shim_test_case_log_info("Unbind lun id: $lun_id from rg id: $rg_id");
        platform_shim_lun_unbind($luns[1]);
        splice(@luns, 1, 1);
      
        # @note Decrement the number of luns per raid group because this value
        #       will be used to calculate the per-lun I/O capacity and must
        #       not change until the test completes
        my $lun_io_capacity_gb = $lun_capacity_gb * $luns_per_rg;
        $luns_per_rg--;

        # @note Use the existing number of luns to determine the I/O max lba
        #       Because this value will be used to execute the data check
        $lun_io_capacity_gb = int($lun_io_capacity_gb / $luns_per_rg);
        $lun_io_max_lba = int(($lun_io_capacity_gb * 2097152) / (100 / $io_physical_percent));
        $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
        $self->platform_shim_test_case_log_info("Bound: $luns_per_rg LUNs with: 1 hole on rg id: $rg_id" .
                                                " max io lba per lun: $lun_io_max_lba_hex");

        # Setup the raid group config 
        my %rg_config = ();
        my @source_faults = ();
        my @dest_faults = ();
        $rg_config{width} = $width;
        $rg_config{data_disks} = $data_disks;
        $rg_config{raid_group_id} = $rg_id;
        $rg_config{raid_type} = $raid_type;
        $rg_config{raid_group} = $rg;
        $rg_config{luns} = \@luns;
        $rg_config{lun_capacity_gb} = $lun_capacity_gb;
        $rg_config{lun_io_max_lba} = $lun_io_max_lba;
        $rg_config{copy_position} = $eol_position;
        $rg_config{remove_position} = $remove_position;
        $rg_config{source_disk} = $disks[$eol_position];
        $rg_config{source_id} = platform_shim_disk_get_property($rg_config{source_disk}, 'id');
        $rg_config{remove_disk} = $disks[$remove_position];
        $rg_config{remove_id} = platform_shim_disk_get_property($rg_config{remove_disk}, 'id');
        #$rg_config{source_fbe_id} = platform_shim_disk_get_property($rg_config{source_disk}, 'fbe_id');
        # @note The vd_fbe_id is already in hex.
        # @note Since system drives return the list of PVD parents, the is more
        #       than 1.  Clean up the system raid groups which are the first.
        $rg_config{vd_fbe_id_hex} = platform_shim_disk_get_property($rg_config{source_disk}, 'vd_fbe_id');
        my @parent_ids = split ' ', $rg_config{vd_fbe_id_hex};
        my $number_of_parents = scalar(@parent_ids);
        if ($number_of_parents > 1) {
            $rg_config{vd_fbe_id_hex} = $parent_ids[($number_of_parents - 1)];
        }
        #$rg_config{vd_capacity} = test_util_copy_get_vd_capacity($self, $rg_config{vd_fbe_id}, $sp); 
        $rg_config{rg_fbe_id} = platform_shim_raid_group_get_property($rg, 'fbe_id');
        $rg_config{rg_fbe_id_hex} = sprintf "0x%lX", $rg_config{rg_fbe_id};
        $rg_config{percent_copied} = 0;
        # @todo Shouldn't need this long term for encryption test
        $rg_config{copy_checkpoint} = Math::BigInt->from_hex(platform_shim_disk_get_copy_checkpoint($rg_config{source_disk}));
        $rg_config{rekey_checkpoint} = test_util_encryption_get_rg_rekey_checkpoint($rg);
        $rg_config{source_removed_handle} = \@source_faults;
        $rg_config{b_source_drive_removed} = 0;
        $rg_config{b_source_drive_faulted} = 0;
        $rg_config{b_source_drive_eol} = 0;
        $rg_config{removed_drive_handle} = \@dest_faults;
        $rg_config{b_drive_removed} = 0;
        $rg_config{b_dest_drive_faulted} = 0;
        $rg_config{b_additional_lun_bound} = 0;
        $rg_config{additional_lun_id} = 0;
      
        # Push the rg config
        push @rg_configs, \%rg_config;
    }

    # We have removed (1) LUN from each raid group.  
    # Decrement the LUNs per raid group
    $$luns_per_rg_ref--;

    # Return the address of the raid groups test configs created
    return \@rg_configs;
}
# end encryption_initialize_rg_configs

#=
#*          encryption_refresh_rg_configs
#*
#* @brief   Refresh the raid group test configurations after copies
#*          have completed.  Select a new copy position and setup the
#*          information.
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $luns_per_rg_ref - Address of LUNs per raid group will be updated
#* @param   $consumed_physical_capacity_gb - The amount of physical capacity to bind
#*              on each raid group.
#* @param   $io_physical_percent - Percent of physical to send I/O to
#*
#* @return  $status
#*
#=cut 
sub encryption_refresh_rg_configs
{
   my ($self, $step, $rg_configs_ref) = @_;

    $self->platform_shim_test_case_log_step("Step $step: Refresh raid groups and setup copy");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $raid_type = $rg_config->{raid_type};
        my $rg_id = $rg_config->{raid_group_id};
        my $rg = $rg_config->{raid_group};
        my $width = $rg_config->{width};
        my $eol_position = int(rand($width));
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};
            
        # Mark this raid group `dirty' and get the updated disk information
        platform_shim_raid_group_mark_dirty($rg);
        my @disks = platform_shim_raid_group_get_disk($rg);

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            next;
        }

        # Skip any riad groups with the source faulted or destination removed
        if (!$b_dest_removed    &&
            !$b_source_faulted     ) {
            # Configure the source disk information
            $rg_config->{copy_position} = $eol_position;
            $rg_config->{source_disk} = $disks[$eol_position];
            $rg_config->{source_id} = platform_shim_disk_get_property($rg_config->{source_disk}, 'id');

            # Now update the virutal drive information
            #$rg_config{source_fbe_id} = platform_shim_disk_get_property($rg_config{source_disk}, 'fbe_id');
            # @note The vd_fbe_id is already in hex.
            # @note Since system drives return the list of PVD parents, the is more
            #       than 1.  Clean up the system raid groups which are the first.
            $rg_config->{vd_fbe_id_hex} = platform_shim_disk_get_property($rg_config->{source_disk}, 'vd_fbe_id');
            my @parent_ids = split ' ', $rg_config->{vd_fbe_id_hex};
            my $number_of_parents = scalar(@parent_ids);
            if ($number_of_parents > 1) {
                $rg_config->{vd_fbe_id_hex} = $parent_ids[($number_of_parents - 1)];
            }

            # Log some information
            $self->platform_shim_test_case_log_info("Refresh rg id: $rg_id type: $raid_type with copy position: $eol_position" .
                                                    " vd: $rg_config->{vd_fbe_id_hex} source disk: $rg_config->{source_id}");

        } # end if not destination removed or source faulted

    }

    # Return the $status
    return $status;
}
# end encryption_refresh_rg_configs

#=
#*           encryption_test_verify_no_progress_on_non_redundant
#*
#* @brief    Validate that copy does not start on non-redundant raid groups
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut 
sub encryption_test_verify_no_progress_on_non_redundant
{
    my ($self, $test, $rg_configs_ref, $no_encryption_progress_timeout) = @_;

    $self->platform_shim_test_case_log_step("Test $test: Verify proactive copy does not make progress on non-redundant raid types");
    my @rg_configs = @{$rg_configs_ref};
    my $rg_index = 0;
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $raid_type = $rg_config->{raid_type};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};

        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            # Verify that the copied denied event is generated
            my $rg_id = $rg_config->{raid_group_id};
            my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
            my @luns = @{$rg_config->{luns}};
            my $b_remove_non_redundant_rg = 0;
            my $expected_copy_checkpoint = Math::BigInt->from_hex('0x0');

            # @note We will only see the proacive copied denied event if this
            #       a newly bound raid group.
            if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {
                platform_shim_event_proactive_copy_denied({object => $self->{device}, disk => $source_disk});
            }
            $self->platform_shim_test_case_log_info("Verified no progress event for vd: $vd_fbe_id rg id: $rg_id");
            my $copy_checkpoint = test_util_copy_validate_copy_doesnt_start($self, $source_disk, $source_id,
                                                                            \$rg_config->{percent_copied},
                                                                            $expected_copy_checkpoint, # Should not make any progress
                                                                            $no_encryption_progress_timeout);
            # If enabled, destroy the luns and raid groups
            if ($b_remove_non_redundant_rg) {
                my $rg = $rg_config->{raid_group};
                my $lun_index = 0;
                foreach my $lun (@luns) {
                    $self->platform_shim_test_case_log_info("Remove non-redundant lun index: $lun_index");
                    platform_shim_lun_unbind($lun);
                    $lun_index++;
                }
                platform_shim_raid_group_destroy($rg);
            }

            # Now clear EOL on source
            eval {
                if (platform_shim_disk_get_eol_state($source_disk)) {
                    # @note Only wait for the last drive to be cleared to become Ready
                    #       takes some time.  Have seen it timeout waiting for `unbound'
                    #       yet EOL is clear and the PVD is ready.
                    $self->platform_shim_test_case_log_info("Remove non-redundant rg config disk: $source_id clear EOL state");
                    platform_shim_disk_clear_eol_state($source_disk);
                    if ( my $e = Exception::Class->caught() ) {
                        my $msg = ref($e) ? $e->full_message() : $e;
                        print "message received as exception: " . $msg;
                        $self->logPropertyInfo("Could not clear EOL state on source disk: $source_id", $msg );
                    }
                }
            }; # end eval

            # Always remove it from the raid groups under test
            splice(@{$rg_configs_ref}, $rg_index, 1);
            last;
        }

        # Goto next raid group
        $rg_index++;
    }

    # Return the status
    return $status;
}
# end encryption_test_verify_no_progress_on_non_redundant

#=
#*          encryption_start_read_only_io
#*
#* @brief   Start read only I/O to either ALL the bound LUNs OR to the
#*          LUN specified.
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $target_sp - The SP to run the I/O on
#* @param   $io_threads - How many I/O threads to start
#* @param   $lun_for_io - If defined then only start I/O on this LUN
#* @param   $max_lba_for_lun_io - If $lun_for_io is defined then this is the maximum lba for the request
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_start_read_only_io
{
    my ($self, $step, $rg_configs_ref, $target_sp, $io_threads, $lun_for_io, $max_lba_for_lun_io) = @_;
     
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $expected_number_of_ios = 0;
    my $maximum_lba = 0;
    my $min_io_size = 0x1;
    my $max_io_size = 0x80;
    my $status = 0;
    
    # @todo Although it is desired to perform a read check we cannot since
    #       the sequence id is incremented for each pass.
    #           /* Random requests increment the pass count on every new request.
    #           fbe_rdgen_ts_inc_pass_count(ts_p);
    #           static __forceinline void fbe_rdgen_ts_inc_pass_count(fbe_rdgen_ts_t *ts_p)
    #           /* Increment both the pass_count and the seqeunce count*/
    #           ts_p->pass_count++;
    #           if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
    #                                       FBE_RDGEN_DATA_PATTERN_FLAGS_FIXED_SEQUENCE_NUMBER))
    #               fbe_rdgen_ts_inc_sequence_count(ts_p); 
    #       So we cannot use the following params:
    #                   use_block => 1,
    #                   block_count => $io_size,
    #                   
    # @todo In addition to validating the data during copy operations the
    #       second goal of running background I/O is to slow the copy operations
    #       down.  Therefore:
    #           o Use random I/O
    #           o Do NOT set affinity_spread => 1
    #           o Attempt to saturate drive cache by using large I/O size
    # 
     
    # If neccessary change the target SP
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $target_sp);
    }

    # Determine if we are sending I/O to all or (1) LUN
    if (defined $lun_for_io) {
        # The lun and maximum lba were specified.  Simply start I/O to this lun
        my $lun_id = platform_shim_lun_get_property($lun_for_io, 'id');
        $expected_number_of_ios = $io_threads;

        # @note rdgen does not automatically truncate the I/O to guarantee that
        #       all blocks will be written.  Therefore round the max lba down to the
        #       next block count.
        $maximum_lba = $max_lba_for_lun_io;
        if ($max_lba_for_lun_io % $max_io_size) {
            $maximum_lba -= $max_io_size - ($max_lba_for_lun_io % $max_io_size);
        }
        my $maximum_lba_hex = sprintf "0x%lx", $maximum_lba;

        # Log the I/O an on which SP
        $self->platform_shim_test_case_log_step("Step $step: Start: $io_threads threads read only I/O on lun id: $lun_id" .
                                                " on: $target_sp_name max lba: $maximum_lba_hex");

        # @note Be careful of the thread_count since even (2) threads per lun can slow the system!
        my %rdgen_params = (read_only   => 1,
                            thread_count => $io_threads,
                            use_block => 1, 
                            block_count => $max_io_size,
                            minimum_blocks => $min_io_size,
                            start_lba => 0,
                            maximum_lba => $maximum_lba,
                            object => $lun_for_io,
                            );

        # Start the I/O and confirm the progress
        platform_shim_device_rdgen_start_io($device, \%rdgen_params);
        test_util_io_verify_rdgen_io_progress($self, $expected_number_of_ios, 10);
    } else {
        # Else start the I/O on all bound luns.  We need to walk thru
        # all the raid groups and determine the maximum LBA
        platform_shim_device_rdgen_reset_stats($device);
        foreach my $rg_config (@rg_configs) {
            my $rg_id = $rg_config->{raid_group_id};
            my @luns = platform_shim_raid_group_get_lun($rg_config->{raid_group});
            #my @luns = @{$rg_config->{luns}};
            $rg_config->{luns} = \@luns;
            my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
            my $data_disks = $rg_config->{data_disks};
            # Blocks is chunks stripe size
            my $blocks = $data_disks * 0x800;
            
            # Determine min and max io size
            $min_io_size = 0x80 - 1;
            $max_io_size = $blocks;

            # @note rdgen does not automatically truncate the I/O to guarantee that
            #       all blocks will be written.  Therefore round the max lba down to the
            #       next block count.
            $maximum_lba = $lun_io_max_lba;
            if ($lun_io_max_lba % $max_io_size) {
                $maximum_lba -= $max_io_size - ($lun_io_max_lba % $max_io_size);
            }
            my $maximum_lba_hex = sprintf "0x%lx", $maximum_lba;

            # Log the I/O an on which SP
            $self->platform_shim_test_case_log_step("Step $step: Start: $io_threads threads read only I/O on rg id: $rg_id" .
                                                    " on: $target_sp_name max lba: $maximum_lba_hex");

            # Currently we must start each LUN one at a time
            foreach my $lun (@luns) {
                # @note Be careful of the thread_count since even (2) threads per lun can slow the system!
                my %rdgen_params = (read_only   => 1, 
                                    thread_count => $io_threads,
                                    block_count => $max_io_size,
                                    use_block => 1,
                                    minimum_blocks => $min_io_size,
                                    start_lba => 0,
                                    maximum_lba => $maximum_lba,
                                    object => $lun,
                                    );

                # Start the I/O but don't confirm progress
                $expected_number_of_ios++;
                platform_shim_device_rdgen_start_io($device, \%rdgen_params);
            } #end for all luns

        } #end for all raid groups

        # Now wait for at least (1) I/o for each lun
        test_util_io_verify_rdgen_io_progress($self, $expected_number_of_ios, 10);

    } # end else run i/o on all bound luns

    # If neccessary change the target SP back to the current active
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }

    # Return the status
    return $status;
}
# end encryption_start_read_only_io

#*           encryption_set_eol
#*
#* @brief    Set EOL on a random position in each raid group
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_set_eol
{
    my ($self, $step, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Step: $step: Set and verify EOL on source disk");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $rg_fbe_id = $rg_config->{rg_fbe_id_hex};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $copy_position = $rg_config->{copy_position};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};
        my $b_eol_already_set = platform_shim_disk_get_eol_state($source_disk);

        # Now set EOL (if there isn't a copy in progress or aborted) or 
        # already set
        if ( ($raid_type ne 'r0') &&
            !$b_eol_already_set   &&
            !$b_dest_removed      &&
            !$b_source_faulted       ) {
            $self->platform_shim_test_case_log_info("Set EOL on rg id: $rg_id ($rg_fbe_id) type: $rg_config->{raid_type}" .
                                                    " position: $copy_position (vd: $vd_fbe_id) disk: $source_id");
            platform_shim_disk_set_eol($source_disk);
            $rg_config->{b_source_drive_eol} = 1;
        }

    } 

    # Return the status
    return $status;
}
# end encryption_set_eol

#*           encryption_set_eol_on_redundant_raid_groups
#*
#* @brief    Set EOL on a random position in each raid group
#*
#* @param    $self - Main argument pointer
#* @param    $task - The current task of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_set_eol_on_redundant_raid_groups
{
    my ($self, $task, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Task: $task: Set and verify EOL on selected position for each raid group");
    my @rg_configs = @{$rg_configs_ref};
    my $step = 1;
    my $status = 0;

    # Simply call the method to set EOL
    $status = encryption_set_eol($self, $step++, $rg_configs_ref);

    # Return the status
    return $status;
}
# end encryption_set_eol_on_redundant_raid_groups
#=
#*          encryption_test_reboot_active_after_copy_start
#*
#* @brief   The moment (1) copy operation starts reboot the active SP.
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $active_sp - The current CMI active SP
#* @param   $active_sp_ref - Address of active SP to update
#* @param   $passive_sp_ref - Address of passive SP to update
#* @param   $b_wait_for_sp_ready_after_reboot - True wait for peer to reboot before proceeding
#* @param   $encryption_complete_timeout_secs - How long to wait for the copy to complete
#* @param   $lun_io_threads - Number of per LUN I/O threads
#*
#* @return  $status - The status of this operation
#*
#=cut
sub encryption_test_reboot_active_after_copy_start
{
    my ($self, $test, $rg_configs_ref, $active_sp, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot, $encryption_complete_timeout_secs, $lun_io_threads) = @_;
     
    $self->platform_shim_test_case_log_step("Test $test: Reboot Active SP after copy job starts");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $io_sp = $$passive_sp_ref;
    my $step = 1;
    my $status = 0;
    my $num_rgs = scalar(@rg_configs);
    my $rg_index_to_wait_for = int(rand($num_rgs));
    my $rg_index = 0;

    # Select a random raid group to wait for but not a RAID-0
    foreach my $rg_config (@rg_configs) {
        my $raid_type = $rg_config->{raid_type};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            if ($rg_index == $rg_index_to_wait_for) {
                if ($rg_index_to_wait_for == ($num_rgs - 1)) {
                    $rg_index_to_wait_for--;
                } else {
                    $rg_index_to_wait_for++;
                }
                last;
            }
        }

        # Goto next
        $rg_index++;
    }

    # Wait for copy job to start a the select raid group
    my $rg_config = $rg_configs[$rg_index_to_wait_for];
    my $rg_id = $rg_config->{raid_group_id};
    my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
    my $source_disk = $rg_config->{source_disk};
    my $source_id = $rg_config->{source_id};

    # Verify the `Proactive Spare' swapped in event
    $self->platform_shim_test_case_log_step("Step $step: Wait for proactive copy job to start on vd: $vd_fbe_id disk: $source_id rg id: $rg_id");
    my $dest_id = platform_shim_event_proactive_spare_swapped_in({object => $device, disk => $source_disk});
    $self->platform_shim_test_case_log_info("Proactive copy started on vd: $vd_fbe_id source: $source_id" .
                                            " destination: $dest_id rg id: $rg_id");

    # Reboot the active SP
    $status = encryption_reboot_sp($self, $step++, $active_sp, $active_sp_ref, $passive_sp_ref, 
                                       $b_wait_for_sp_ready_after_reboot);

    # Now wait for copy to start on all raid groups
    # Wait for proactive copy to start on each raid group
    $status = encryption_verify_proactive_copy_starts($self, $step++, $rg_configs_ref);

    # Stop I/O so that the copy completes
    $status = encryption_stop_io($self, $step++, $io_sp);

    # Validate the background pattern on all LUNs
    # This takes too long, so we skip it for now.
    #$status = encryption_check_background_pattern_on_all_luns($self, $step++, $rg_configs_ref, $io_sp);

    # Wait for copy to complete
    $status = encryption_wait_for_copy_to_complete($self, $step++, $rg_configs_ref, $encryption_complete_timeout_secs);

    # Clear EOL and DRIVE_FAULT on the source drives
    $status = encryption_cleanup_source_disks($self, $step++, $rg_configs_ref);

    # Now refresh the raid group information and setup the new copy information
    $status = encryption_refresh_rg_configs($self, $step++, $rg_configs_ref);

    # Now start I/O on the active SP
    $io_sp = $$active_sp_ref;
    $status = encryption_start_read_only_io($self, $step++, $rg_configs_ref, $io_sp,
                                                $lun_io_threads);

    # Return the status
    return $status;
}
# end encryption_test_verify_encryption_starts

#=
#*           encryption_verify_rekey_progress
#*
#* @brief    Verify that reky makes progress
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_verify_rekey_progress
{
    my ($self, $step, $rg_configs_ref, $encryption_progress_timeout) = @_;

    $self->platform_shim_test_case_log_step("Step $step: Verify encryption rekey makes progress on each raid group");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $rekey_checkpoint = $rg_config->{rekey_checkpoint};
        my $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        my $b_wait_for_percent_increase = 0; # Don't wait for percent increase

        # Check if the rkey is already complete
        if (test_util_is_rekey_complete($rekey_checkpoint)) {
            $self->platform_shim_test_case_log_info("Encryption rekey check progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey complete");
            next;
        } else {
            $rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg_config);
            $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
            $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
            $self->platform_shim_test_case_log_info("Encryption rekey check progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey checkpoint: $rekey_checkpoint_hex");
        }

        # Wait for current checkpoint to increase
        $rekey_checkpoint = test_util_encryption_check_rekey_progress($self, $rg_config, 
                                                                      $encryption_progress_timeout);
        $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        $rg_config->{rekey_checkpoint} = $rekey_checkpoint;

        # If rekey is complete log it
        if (test_util_is_rekey_complete($rekey_checkpoint)) {
            $self->platform_shim_test_case_log_info("Encryption rekey check progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey complete");
        } else {
            $self->platform_shim_test_case_log_info("Encryption rekey check progress rg id: $rg_id type: $raid_type" .
                                                    " rekey checkpoint: $rekey_checkpoint_hex");
        }
    }

    # Return the status
    return $status;
}
# end encryption_verify_rekey_progress

#=
#*           encryption_test_verify_rekey_progress
#*
#* @brief    Verify that rekey makes progress
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_verify_rekey_progress
{
    my ($self, $test, $rg_configs_ref, $encryption_progress_timeout) = @_;

    $self->platform_shim_test_case_log_step("Test $test: Verify encryption rekey makes progress on each raid group");
    my @rg_configs = @{$rg_configs_ref};
    my $step = 1;
    my $status = 0;

    # Check that the rekey is making progress on all raid groups
    $status = encryption_verify_rekey_progress($self, $step++, $rg_configs_ref, $encryption_progress_timeout);

    # Return the status
    return $status;
}
# end encryption_test_verify_rekey_progress

#=
#*           encryption_test_start_rekey
#*
#* @brief    Start the rekey request.
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_start_rekey
{
    my ($self, $test, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Test $test: Start rekey on all raid groups");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $step = 1;
    my $status = 0;

    # Start the rekey
    $status = test_util_encryption_start_rekey($self);

    # Return the status
    return $status;
}
# end encryption_test_start_rekey

#=
#*           encryption_verify_proactive_copy_starts
#*
#* @brief    Validate EOL and copy starts
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_verify_proactive_copy_starts
{
    my ($self, $step, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Step: $step: Verify proactive copy starts on each raid group");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $status = 0;
    my $dest_disk = ();
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $raid_type = $rg_config->{raid_type};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            next;
        }

        # Verify the `Proactive Spare' swapped in event
        $self->platform_shim_test_case_log_info("Wait for proactive copy job to start on vd: $vd_fbe_id disk: $source_id rg id: $rg_id");
        my $dest_id = platform_shim_event_proactive_spare_swapped_in({object => $device, disk => $source_disk});
        my %dest_disk_params =(id => $dest_id);
        ($dest_disk) = platform_shim_device_get_disk($device, \%dest_disk_params);
        my $eol_state = platform_shim_disk_get_eol_state($source_disk);
        if (!$eol_state) {
            platform_shim_rg_config_assert_true_msg($eol_state != 0, "EOL Not set");
        }
        $rg_config->{dest_disk} = $dest_disk;
        $rg_config->{dest_id} = $dest_id;
        $self->platform_shim_test_case_log_info("Wait for proactive copy to start on vd: $vd_fbe_id source: $source_id" .
                                                " destination: $dest_id rg id: $rg_id");
        platform_shim_event_copy_started({object => $device, disk => $dest_disk});

        # Display the checkpoint info
        $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
        $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        $rg_config->{percent_copied} = $percent_copied;
        $rg_config->{copy_checkpoint} = $copy_checkpoint;
        $self->platform_shim_test_case_log_info("Proactive copy started vd: $vd_fbe_id rg id: $rg_id " .
                                                " percent copied: $percent_copied checkpoint: $copy_checkpoint_hex");
    } 

    # Return the status
    return $status;
}
# end encryption_verify_proactive_copy_starts

#=
#*           encryption_verify_encryption_paused
#*
#* @brief    Validate that encryption has started BUT is now
#*           paused.
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_verify_encryption_paused
{
    my ($self, $step, $rg_configs_ref, $no_rekey_progress_timeout) = @_;
     
    $self->platform_shim_test_case_log_step("Step: $step: Verify encryption has paused");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $rekey_checkpoint = $rg_config->{rekey_checkpoint};
        my $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();

        # @note There is no event log associated with the encryption not making any progress.
        #       Make sure encryption does not progress on this raid group.
        $rekey_checkpoint = test_util_encryption_check_rg_rekey_no_progress($self, $rg, 
                                                                            $no_rekey_progress_timeout);
        $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
        $self->platform_shim_test_case_log_info("Encryption rekey did not progress rg id: $rg_id type: $raid_type" .
                                                " rekey checkpoint: $rekey_checkpoint_hex");
    } 

    # Return the status
    return $status;
}
# end encryption_verify_encryption_paused

#=
#*           encryption_test_verify_copy_starts_encryption_paused
#*
#* @brief    Validate copy starts nad encyption is paused
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_verify_copy_starts_encryption_paused
{
    my ($self, $test, $rg_configs_ref, $no_rekey_progress_timeout) = @_;
     
    $self->platform_shim_test_case_log_step("Test $test: Verify proactive copy starts and encryption is paused");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $step = 1;
    my $status = 0;

    # Wait for proactive copy to start on each raid group
    $status = encryption_verify_proactive_copy_starts($self, $step++, $rg_configs_ref);

    # Verify encryption is paused during proactive copy
    $status = encryption_verify_encryption_paused($self, $step++, $rg_configs_ref, $no_rekey_progress_timeout);

    # Return the status
    return $status;
}
# end encryption_test_verify_copy_starts_encryption_paused

#=
#*           encryption_test_remove_drive_and_verify_no_progress
#*
#* @brief    Remove drive and confirm that encryption rekey doesn't proceed
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_remove_drive_and_verify_no_progress
{
    my ($self, $test, $rg_configs_ref, $no_encryption_progress_timeout) = @_;
  
    $self->platform_shim_test_case_log_step("Test $test: Remove destination drive and confirm no copy progress");
    my @rg_configs = @{$rg_configs_ref};
    my $num_of_rgs_wo_fault = 0;
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        # Remove a drive (except from RAID-0)
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $rekey_checkpoint = $rg_config->{rekey_checkpoint};
        my $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        my $b_drive_removed = $rg_config->{b_drive_removed};
        my $remove_disk = $rg_config->{remove_disk};
        my $remove_id = $rg_config->{remove_id};
        my $percent_encrypted;

        # Only attempt to remove a drive if not RAID-0 nad not already degraded
        if ( ($raid_type ne 'r0') &&
            !$b_drive_removed        ) {
            if ($num_of_rgs_wo_fault == 0) {
                $num_of_rgs_wo_fault++;
            } else {
                # Check if the rekey is already complete
                $percent_encrypted = test_util_encryption_get_percent_encrypted($self);
                if (test_util_is_rekey_complete($rekey_checkpoint)) {
                    $self->platform_shim_test_case_log_info("Cannot remove drive rg id: $rg_id type: $raid_type" .
                                                            " rekey percent: $percent_encrypted complete");
                    next;
                } else {
                    $rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg_config);
                    $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
                    $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
                    $self->platform_shim_test_case_log_info("Can remove drive rg id: $rg_id type: $raid_type" .
                                                            " encryption rekey checkpoint: $rekey_checkpoint_hex");
                }

                # If the percent encrypted is less than 90 percent then remove the drive
                if ($percent_encrypted < 90) {
                    my $fault = platform_shim_disk_remove($remove_disk); 
                    push @{$rg_config->{removed_drive_handle}}, $fault;
                    $self->platform_shim_test_case_log_info("Removed drive: $remove_id from rg id: $rg_id");
                    $rg_config->{b_drive_removed} = 1;
                }
            } # end else if at least one copy without destination remove

        } # end not RAID-0 and not drive removed
    }
    foreach my $rg_config (@rg_configs) {
        # Confirm that the encryption does not progress
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $remove_id = $rg_config->{remove_id};
        my $remove_disk = $rg_config->{remove_disk};
        my $rekey_checkpoint = $rg_config->{rekey_checkpoint};
        my $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();

        # Only if we removed the drive for this encryption request
        if ($rg_config->{b_drive_removed}) {
            # @note There is no event log associated with the rekey not making any progress.
            #       Make sure encryption does not progress
            $rekey_checkpoint = test_util_encryption_check_rg_rekey_no_progress($self, $rg_config,
                                                                               $no_encryption_progress_timeout);
            $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
            $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
            $self->platform_shim_test_case_log_info("Encryption did not progress rg id: $rg_id type: $raid_type" .
                                                    " rekey checkpoint: $rekey_checkpoint_hex");
        }
    }

    # Return the status
    return $status;
}
# end encryption_test_remove_drive_and_verify_no_progress

#=
#*           encryption_test_reinsert_dest_verify_progress
#*
#* @brief    Re-insert the destination drive and verify progress
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_reinsert_dest_verify_progress
{
    my ($self, $test, $rg_configs_ref, $encryption_progress_timeout) = @_;
  
    $self->platform_shim_test_case_log_step("Test $test: Re-insert destination drive and verify copy progresses");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $dest_id = $rg_config->{dest_id};

        # First re-insert the destination drive from each raid group
        if ($rg_config->{b_drive_removed}) {
            my @faults = @{$rg_config->{removed_drive_handle}};
            my $fault = pop @faults;
            my $reinsert = platform_shim_disk_reinsert($fault); 
            $self->platform_shim_test_case_log_info("Re-inserted: $dest_id vd: $vd_fbe_id rg id: $rg_id");
            # @note Do not clear destination removed until later.
        }
    }
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $dest_disk = $rg_config->{dest_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_id = $rg_config->{dest_id};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $percent_copied = $rg_config->{percent_copied};
        my $b_wait_for_percent_increase = 0;    # Don't wait for the percent copied to increase
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            next;
        }

        # Check if the copy is already complete
        if (test_util_is_copy_complete($copy_checkpoint)) {
            $self->platform_shim_test_case_log_info("Dest was removed: $b_dest_removed source faulted: $b_source_faulted");
            $self->platform_shim_test_case_log_info("     vd: $vd_fbe_id rg id: $rg_id" .
                                                    " source: $source_id destination: $dest_id copy complete");
            next;
        } else {
            $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
            $copy_checkpoint_hex = $copy_checkpoint->as_hex();
            $rg_config->{percent_copied} = $percent_copied;
            $rg_config->{copy_checkpoint} = $copy_checkpoint;
            $self->platform_shim_test_case_log_info("Dest was removed: $b_dest_removed source faulted: $b_source_faulted");
            $self->platform_shim_test_case_log_info("     vd: $vd_fbe_id rg id: $rg_id " .
                                                    " percent copied: $percent_copied checkpoint: $copy_checkpoint_hex");
        }

        # Wait for current checkpoint to increase
        if ($b_dest_removed) {
            $copy_checkpoint = test_util_copy_check_copy_progress($self, $dest_disk, $dest_id, 
                                                                  \$percent_copied, 
                                                                  $b_wait_for_percent_increase, 
                                                                  $encryption_progress_timeout);
            $rg_config->{percent_copied} = $percent_copied;
            $rg_config->{copy_checkpoint} = $copy_checkpoint;

            # If the copy is done mark it done
            if (test_util_is_copy_complete($copy_checkpoint)) {
                $self->platform_shim_test_case_log_info("Dest was removed: $b_dest_removed check progress on vd: $vd_fbe_id rg id: $rg_id" .
                                                    " source: $source_id destination: $dest_id copy complete");
            } else {
                $self->platform_shim_test_case_log_info("Dest was removed: $b_dest_removed on vd: $vd_fbe_id rg id: $rg_id" .
                                                    " source: $source_id destination: $dest_id" .
                                                    " percent copied: $percent_copied checkpoint: " . $copy_checkpoint_hex);
            }
            $rg_config->{b_drive_removed} = 0;
        }
    }
    
    # Return the status
    return $status;
}
# end encryption_test_reinsert_dest_verify_progress

#=
#*          encryption_wait_for_copy_to_complete
#*
#* @brief   Wait for copy to complete on all the raid groups
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $encryption_complete_timeout_secs - Number of seconds toe wait for copy complete
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_wait_for_copy_to_complete
{
    my ($self, $step, $rg_configs_ref, $encryption_complete_timeout_secs) = @_;
     
    $self->platform_shim_test_case_log_step("Step $step: Wait for proactive copy completion");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $copy_position = $rg_config->{copy_position};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_disk = $rg_config->{dest_disk};
        my $dest_id = $rg_config->{dest_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            next;
        }

        # If not complete report the checkpoint
        if (!$b_dest_removed   &&
            !$b_source_faulted    ) {
            if (!test_util_is_copy_complete($copy_checkpoint)) {
                $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
                $copy_checkpoint_hex = $copy_checkpoint->as_hex();
                $rg_config->{percent_copied} = $percent_copied;
                $rg_config->{copy_checkpoint} = $copy_checkpoint;
                $self->platform_shim_test_case_log_info("Proactive Copy complete on vd: $vd_fbe_id rg id: $rg_id" .
                                                        " percent copied: $percent_copied" .
                                                        " copy checkpoint $copy_checkpoint_hex wait for copy complete");
            }

            # Verify the copy complete event
            platform_shim_event_copy_complete({object => $self->{device}, disk => $dest_disk, timeout => $encryption_complete_timeout_secs . ' S'});
            $self->platform_shim_test_case_log_info("Proactive Copy completed on vd: $vd_fbe_id rg id: $rg_id" .
                                                " position: $copy_position source: $source_id destination: $dest_id");
        }
    } 

    # Return the status
    return $status;
}
# end encryption_wait_for_copy_to_complete

#=
#*           encryption_test_wait_for_copy_and_rebuild_to_complete
#*
#* @brief    Wait for copy to complete on all the raid groups
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_wait_for_copy_and_rebuild_to_complete
{
    my ($self, $test, $rg_configs_ref, $encryption_complete_timeout_secs) = @_;
     
    $self->platform_shim_test_case_log_step("Test $test: Wait for proactive copy and rebuild completion");
    my $step = 1;
    my $status = 0;

    # First wait for the copies to complete
    $status = encryption_wait_for_copy_to_complete($self, $step++, $rg_configs_ref, $encryption_complete_timeout_secs);

    # For those raid groups where the source drive was faulted
    # wait for the rebuild to complete
    $status = encryption_wait_for_parent_rebuild_to_complete($self, $step++, $rg_configs_ref, $encryption_complete_timeout_secs);

    # Return the status
    return $status;
}
# end encryption_test_wait_for_copy_and_rebuild_to_complete

#=
#*           encryption_cleanup_source_disks
#*
#* @brief    Clear EOL and remove any faults on source drives
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_cleanup_source_disks
{
    my ($self, $step, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Step $step: Cleanup source drives (clear EOL, marked failed etc)");
    my $device = $self->{device};
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $rg_index = 0;
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};

        # @todo Check if we need to clear DRIVE_FAULT etc

        # Clear EOL
        $self->platform_shim_test_case_log_info("Cleanup source disk: $source_id get EOL state");
        eval {
            if (platform_shim_disk_get_eol_state($source_disk)) {
                # @note Only wait for the last drive to be cleared to become Ready
                #       takes some time.  Have seen it timeout waiting for `unbound'
                #       yet EOL is clear and the PVD is ready.
                if ($rg_index < $rg_count) {
                    # For all except last don't wait for PVD Ready
                    $self->platform_shim_test_case_log_info("Cleanup source disk: $source_id clear EOL state");
                    platform_shim_disk_clear_eol_state($source_disk);
                    if ( my $e = Exception::Class->caught() ) {
                        my $msg = ref($e) ? $e->full_message() : $e;
                        print "message received as exception: " . $msg;
                        $self->logPropertyInfo("Could not clear EOL state on source disk: $source_id", $msg );
                    }
                } else {
                    # Else this is the last source disk to clear.
                    # Wait for the drive to become Ready
                    my @tempdisks = ($source_disk);
                    $self->platform_shim_test_case_log_info("Cleanup source disk: $source_id clear EOL and wait for Ready");
                    platform_shim_disk_clear_eol($device, $source_disk, \@tempdisks);
                    if ( my $e = Exception::Class->caught() ) {
                        my $msg = ref($e) ? $e->full_message() : $e;
                        print "message received as exception: " . $msg;
                        $self->logPropertyInfo("Could not clear EOL state on source disk: $source_id", $msg );
                    }
                }
                $rg_config->{b_source_drive_eol} = 0;
            }
        }; # end eval

        # Goto next raid group
        $rg_index++;
    } 

    # Return the status
    return $status;
}
# end encryption_cleanup_source_disks

#=
#*           encryption_cleanup_after_test_complete
#*
#* @brief    Clear EOL and remove any faults on source drives
#*
#* @param    $self - Main argument pointer
#* @param    $task - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_cleanup_after_test_complete
{
    my ($self, $task, $rg_configs_ref) = @_;
     
    $self->platform_shim_test_case_log_step("Task $task: Proactive copy cleanup after test complete");
    my $device = $self->{device};
    my @rg_configs = @{$rg_configs_ref};
    my $step = 1;
    my $status = 0;

    # Test is complete cleanup the source drives
    $status = encryption_cleanup_source_disks($self, $step++, $rg_configs_ref);

    # Return $status
    return $status;
}
#end encryption_cleanup_after_test_complete

#=
#*           encryption_stop_io
#*
#* @brief    Stop and per LUN I/O
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $target_sp - The SP to use when stopping I/O
#* @param    $lun - If defined this is teh lun to stop I/O for                               
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_stop_io
{
    my ($self, $step, $target_sp, $lun) = @_;
     
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my $status = 0;
    my %rdgen_params = ();

    # Stop any rdgen I/O on this SP
    if (defined $lun) {
        my $lun_id = platform_shim_lun_get_property($lun, 'id');
        $self->platform_shim_test_case_log_step("Step $step: Stop rdgen I/O on LUN: $lun_id on: $target_sp_name");
        $rdgen_params{object} = $lun;
    } else {
        $self->platform_shim_test_case_log_step("Step $step: Stop rdgen I/O on all LUNs on: $target_sp_name");
    }

    # If neccessary change the target SP
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $target_sp);
    }

    platform_shim_device_rdgen_stop_io($device, \%rdgen_params);

    # If neccessary change the target SP back to the active
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }

    # Return the status
    return $status;
}
# end encryption_stop_io

#=
#*           encryption_stop_all_io_after_test_complete
#*
#* @brief    Stop all I/O 
#*
#* @param    $self - Main argument pointer
#* @param    $task - The current step of the test case 
#* @param    $target_sp - The SP to use when stopping I/O
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_stop_all_io_after_test_complete
{
    my ($self, $task, $target_sp) = @_;
     
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my $step = 1;
    my $status = 0;

    # Stop any rdgen I/O on this SP
    $self->platform_shim_test_case_log_step("Task $task: Stop rdgen I/O on all LUNs on: $target_sp_name");
    $status = encryption_stop_io($self, $step, $target_sp);

    # Return the status
    return $status;
}
# end encryption_stop_all_io_after_test_complete

#=
#*          encryption_configure_params_based_on_platform
#*
#* @brief   Configure the parameters for this test based on platform:
#*              o How many luns per raid group
#*              o The capacity of each lun
#*              o What percentage of the lun capacity should I/O be sent to
#*              o The resulting per LUN maximum LBA to send I/O to
#*
#* @param   $self - Test case 
#* @param   $task - The task number
#* @param   $rgs_ref - Pointer to raid groups created
#* @param   $luns_per_rg_ref - Number of LUNs for each raid group
#* @param   $consumed_physical_capacity_gb_ref - The address of the per raid groups physical capacity to bind (consumed)
#* @param   $io_physical_percent_ref - Percent of consumed capacity to send I/O to 
#* @param   $lun_io_threads_ref - Address of per lun number of I/O threads for background I/O
#*
#* @return  none
#*
#=cut
sub encryption_configure_params_based_on_platform
{
    my ($self, $task, $rgs_ref, $luns_per_rg_ref, $consumed_physical_capacity_gb_ref, $io_physical_percent_ref, $lun_io_threads_ref) = @_;
     
    my @rgs = @{$rgs_ref};
    my $device = $self->{device};
    my $platform_type = 'HW';
    my $test_level = 'Promotion';
    if (platform_shim_test_case_is_functional($self)) {
        $test_level = 'Functional';
    }

    # Set the desired number of luns and consumed capacity
    if (platform_shim_device_is_simulator($device)) {
        # Simulator with maximum drive capacity of 10GB
        # Total bound capacity: (2 + (1 hole)) * 3 = 9GB
        $$luns_per_rg_ref = 3;
        $$consumed_physical_capacity_gb_ref = 9;
    } else {
        # Hardware with minimum drive capacity of 100GB
        if ($test_level eq 'Functional') {
            # Total bound capacity: (4 + (1 hole)) * 10 = 50GB
            $$luns_per_rg_ref = 5;
            $$consumed_physical_capacity_gb_ref = 50;
        } else {
            # Total bound capacity: (2 + (1 hole)) * 10 = 30GB
            $$luns_per_rg_ref = 3;
            $$consumed_physical_capacity_gb_ref = 30;
        }
    }

    # If `clean_config' is not set then use the existing raid groups
    # and any LUNs to determine consumed capacity
    if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        my @luns = platform_shim_raid_group_get_lun($rgs[0]);
        my $num_luns = scalar(@luns);
        if ($num_luns > 0) {
            my $data_disks = test_util_configuration_get_data_disks($self, $rgs[0]);
            my $lun_capacity = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
            my $physical_capacity = int($lun_capacity / 2097152) * $$luns_per_rg_ref;
            $$consumed_physical_capacity_gb_ref = int($physical_capacity / $data_disks);
        }
    }

    # Now set the I/O parameters
    if (platform_shim_device_is_simulator($device)) {
        # Simulator with maximum drive capacity of 10GB
        $platform_type = 'SIM';
        if ($test_level eq 'Functional') {
            $$io_physical_percent_ref = 10;
            $$lun_io_threads_ref = 32;
        } else {
            $$io_physical_percent_ref = 5;
            $$lun_io_threads_ref = 32;
        }
    } else {
        # Hardware with minimum drive capacity of 100GB
        if ($test_level eq 'Functional') {
            $$io_physical_percent_ref = 15;
            $$lun_io_threads_ref = 64;
        } else {
            $$io_physical_percent_ref = 10;
            $$lun_io_threads_ref = 32;
        }
    }
    $self->platform_shim_test_case_log_step("Task $task: Configure params based on platform: $platform_type test level: $test_level");
    $self->platform_shim_test_case_log_info("Params - lun per rg: $$luns_per_rg_ref consumed physical capacity gb: $$consumed_physical_capacity_gb_ref");
    $self->platform_shim_test_case_log_info("         percent of consumed for io: $$io_physical_percent_ref io threads per lun: $$lun_io_threads_ref");
    return;
}
# end encryption_configure_params_based_on_platform

#=
#*           encryption_test_fault_source_and_verify_copy_aborted
#*
#* @brief    Fault the source drive from a random set of raid groups and 
#*           confirm that copy is immediately aborted (When DRIVE_FAULT
#*           occurs the virtual drive does not wait the `trigger spare'
#*           period).
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#* @param    $active_sp - The CMI active SP
#* @param    $no_encryption_progress_timeout - The amount of time to wait for events
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_fault_source_and_verify_copy_aborted
{
    my ($self, $test, $rg_configs_ref, $active_sp, $no_encryption_progress_timeout) = @_;
  
    $self->platform_shim_test_case_log_step("Test $test: Set DRIVE_FAULT on source drive and confirm no copy progress");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $num_rgs = scalar(@rg_configs);
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        # Fault the source drive from a random number of raid groups
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $copy_position = $rg_config->{copy_position};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_disk = $rg_config->{dest_disk};
        my $dest_id = $rg_config->{dest_id};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $percent_copied = $rg_config->{percent_copied};
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Skip RAID-0
        if (($rg_config->{raid_type} eq 'r0')   ||
            ($rg_config->{raid_type} eq 'disk')    ) {
            next;
        }

        # Fault approximately 50% of the copy drives
        my $random = int(rand($num_rgs));
        if (!$b_dest_removed     &&
            !$b_source_faulted   &&
            (($random % 2) == 0)    ) {
            # Don't attempt to fault source if more than 90 percent copied.
            if (test_util_is_copy_complete($copy_checkpoint)) {
                $self->platform_shim_test_case_log_info("Can fault source drive: 0 vd: $vd_fbe_id rg id: $rg_id" .
                                                    " source: $source_id destination: $dest_id copy complete");
                next;
            } else {
                $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
                $copy_checkpoint_hex = $copy_checkpoint->as_hex();
                $rg_config->{percent_copied} = $percent_copied;
                $rg_config->{copy_checkpoint} = $copy_checkpoint;
                $self->platform_shim_test_case_log_info("Can fault source drive: 1 vd: $vd_fbe_id rg id: $rg_id " .
                                                    " percent copied: $percent_copied checkpoint: $copy_checkpoint_hex");
            }

            # If the percent copied is less than 90 percent then fault the source
            if ($percent_copied < 90) {
                platform_shim_disk_set_drive_fault($source_disk, $active_sp);
                $self->platform_shim_test_case_log_info("Set DRIVE_FAULT: $source_id from vd: $vd_fbe_id rg id: $rg_id" .
                                                        " position: $copy_position");
                $rg_config->{b_source_drive_faulted} = 1;
            }

        } # end if even random 
    }

    # Now confirm that copy position has been marked `rebuild logging' and
    # that the copy operation is aborted due to the source being failed.
    foreach my $rg_config (@rg_configs) {
        # Confirm that the copy operation is aborted
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $copy_position = $rg_config->{copy_position};
        my $source_id = $rg_config->{source_id};
        my $source_disk = $rg_config->{source_disk};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Skip RAID-0
        if (($rg_config->{raid_type} eq 'r0')   ||
            ($rg_config->{raid_type} eq 'disk')    ) {
            next;
        }

        # If we faulted the source drive validate rebuild logging and
        # copy aborted
        if ($b_source_faulted) {
            # Confirm that the parent raid group is marked rebuild logging
            $self->platform_shim_test_case_log_info("Validate rebuild logging event for rg id: $rg_id position: $copy_position" .
                                                    " vd: $vd_fbe_id source disk: $source_id");
            platform_shim_event_verify_rebuild_logging_started({object => $device,
							                                    raid_group => $rg_config->{raid_group}});

            # Confirm copy operation was aborted
            $self->platform_shim_test_case_log_info("Validate copy aborted for vd: $vd_fbe_id source disk: $source_id rg id: $rg_id");
            platform_shim_event_copy_failed_due_to_source_removed({object => $device, disk => $source_disk, timeout => $no_encryption_progress_timeout . ' S'});
        }
    }

    # Return the status
    return $status;
}
# end encryption_test_fault_source_and_verify_copy_aborted

#=
#*          encryption_wait_for_parent_rebuild_to_complete
#*
#* @brief   Wait for rebuilds to complete on all the raid groups
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $rebuild_complete_timeout_secs - Amount of time in seconds to wait for rebuilds
#*              to complete
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_wait_for_parent_rebuild_to_complete
{
    my ($self, $test, $rg_configs_ref, $rebuild_complete_timeout_secs) = @_;
     
    $self->platform_shim_test_case_log_step("Step$test: Wait for parent raid groups rebuild completion");
    my $device = $self->{device};
    my @rg_configs = @{$rg_configs_ref};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $status = 0;

    # Locate the raid groups with the source removed and wait for the parent
    # rebuild to complete.
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $copy_position = $rg_config->{copy_position};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_disk = $rg_config->{dest_disk};
        my $dest_id = $rg_config->{dest_id};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # If the source is faulted wait for the parent raid group to complete the rebuild
        if ($b_source_faulted) {
            # Now wait for the rebuild to complete
            $self->platform_shim_test_case_log_info("Wait for parent rebuild to complete source faulted: $b_source_faulted on vd: $vd_fbe_id rg id: $rg_id" .
                                                    " position: $copy_position disk to rebuild: $dest_id");
            platform_shim_event_verify_rebuild_completed({object => $device, disk => $dest_disk, timeout => $rebuild_complete_timeout_secs . ' S'});

            # Clear the drive fault
            $self->platform_shim_test_case_log_info("Clear DRIVE_FAULT on original source disk: $source_id");
            platform_shim_disk_clear_drive_fault($source_disk, $active_sp);
        }
    } 

    # Return the status
    return $status;
}
# end encryption_wait_for_parent_rebuild_to_complete

#=
#*           encryption_config_rebooted_sp
#*
#* @brief    Make any neccessary changes after an SP is rebooted
#*
#* @param    $self - Main argument pointer
#* @param    $rebooted_sp - The SP that was rebooted
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_config_rebooted_sp
{
    my ($self, $rebooted_sp) = @_;
     
    my $device = $self->{device};
    my $rebooted_sp_name = uc(platform_shim_sp_get_property($rebooted_sp, 'name'));
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $status = 0;

    # Re-apply any changes that were not persisted
    $self->platform_shim_test_case_log_info("Configure: $rebooted_sp_name after reboot");
    if ($rebooted_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $rebooted_sp); 
    }
    platform_shim_device_initialize_rdgen($device);
    # @note We no longer call test_util_set_copy_operation_speed
    #my $default_rg_reb_speed = 120;
    #test_util_set_copy_operation_speed($self, $rebooted_sp, $slow_copy_speed, \$default_rg_reb_speed);
    if ($rebooted_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp); 
    }

    # Return the status
    return $status;
}
# end encryption_config_rebooted_sp

#=
#*           encryption_reboot_sp
#*
#* @brief    Simply reboots the SP specified.  It does NOT wait for the
#*           rebooted SP to complete the reboot.
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $sp_to_reboot - The SP to reboot
#* @param    $active_sp_ref - Address of active SP to update
#* @param    $passive_sp_ref - Address of passive SP to update
#* @param    $b_wait_for_sp_ready_after_reboot - If True wait for the rebooted SP
#*              to be ready.
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_reboot_sp
{
    my ($self, $step, $sp_to_reboot, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot) = @_;
     
    my $device = $self->{device};
    my $active_sp = $$active_sp_ref;
    my $passive_sp = $$passive_sp_ref;
    my $sp_to_reboot_name = uc(platform_shim_sp_get_property($sp_to_reboot, 'name'));
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $passive_sp_name = uc(platform_shim_sp_get_property($passive_sp, 'name'));
    my $status = 0;
    my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    
    if ($skip_reboot == 1) {
       print "Skiping reboot since skip_reboot parameter set.\n";
       return 1;
    }

    # Determine if the active SP will change or not
    if ($sp_to_reboot_name eq $active_sp_name) {
        $self->platform_shim_test_case_log_step("Step $step: Reboot active: $active_sp_name making: $passive_sp_name active");
        platform_shim_sp_reboot($sp_to_reboot);
        test_util_general_set_target_sp($self, $passive_sp);
        $$active_sp_ref = $passive_sp;
        $$passive_sp_ref = $active_sp;
    } else {
        $self->platform_shim_test_case_log_step("Step $step: Reboot passive: $passive_sp_name active: $active_sp_name");
        platform_shim_sp_reboot($sp_to_reboot);
    }

    # If we need to wait for peer ready do so now
    if ($b_wait_for_sp_ready_after_reboot) {
        $self->platform_shim_test_case_log_info("Wait for: $sp_to_reboot_name to be ready");
        platform_shim_sp_wait_for_agent($sp_to_reboot, 3600);
        $self->platform_shim_test_case_log_info("$sp_to_reboot_name: is now ready");
        encryption_config_rebooted_sp($self, $sp_to_reboot);
    }

    # Return the status
    return $status;
}
# end encryption_reboot_sp

#=
#*           encryption_wait_for_sp_ready
#*
#* @brief    Wait for the specified SP to be ready
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $sp_to_wait_for - The SP to wait to become ready
#* @param    $b_wait_for_sp_ready_after_reboot - If True we have already
#*              wait so there is no need to do anything.
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_wait_for_sp_ready
{
    my ($self, $step, $sp_to_wait_for, $b_wait_for_sp_ready_after_reboot) = @_;
     
    my $status = 0;

    # If we have already waited just return
    if ($b_wait_for_sp_ready_after_reboot) {
        return $status;
    }
    # Simply wait for SP to become ready
    my $sp_to_wait_for_name = uc(platform_shim_sp_get_property($sp_to_wait_for, 'name'));
    $self->platform_shim_test_case_log_step("Step $step: Wait for: $sp_to_wait_for_name to be ready");
    platform_shim_sp_wait_for_agent($sp_to_wait_for, 3600);
    $self->platform_shim_test_case_log_info("$sp_to_wait_for_name: is now ready");
    encryption_config_rebooted_sp($self, $sp_to_wait_for);

    # Return the status
    return $status;
}
# end encryption_wait_for_sp_ready

#=
#*           encryption_test_bind_lun_verify_rekey_progress
#*
#* @brief    Stop I/O, bind another LUN, write the background pattern
#*           re-start I/O and confirm that copy progresses.
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $io_sp - The SP to write background pattern and start read only I/O
#* @param   $io_threads - Number of I/O threads for read only I/O
#* @param   $encryption_progress_timeout - How long to wait for copy progress
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_bind_lun_verify_rekey_progress
{
    my ($self, $test, $rg_configs_ref, $io_sp, $io_threads, $encryption_progress_timeout) = @_;
  
    $self->platform_shim_test_case_log_step("Test $test: Bind another LUN on rekey raid groups");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});;
    my $io_sp_name = uc(platform_shim_sp_get_property($io_sp, 'name'));
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $status = 0;
    my $step = 1;
    my $expected_io_count = 0;

    # Stop I/O on all LUNs
    $status = encryption_stop_io($self, $step++, $io_sp);

    # Bind a new LUN on any raid group with encryption rekey in progress
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $rekey_checkpoint = $rg_config->{rekey_checkpoint};
        my $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        my @luns = @{$rg_config->{luns}};
        my $lun_capacity = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
        my $lun_capacity_gb = int($lun_capacity / 2097152);
        my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
        my $data_disks = $rg_config->{data_disks};
        # Blocks is chunks stripe size
        my $blocks = $data_disks * 0x800;

        # Check if the rekey is already complete
        if (test_util_is_rekey_complete($rekey_checkpoint)) {
            $self->platform_shim_test_case_log_info("Bind LUN check progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey complete");
            next;
        } else {
            $rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg);
            $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
            $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
            $self->platform_shim_test_case_log_info("Bind LUN check progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey checkpoint: $rekey_checkpoint_hex");
        }


        # I/O has been stopped, bind another LUN
        $self->platform_shim_test_case_log_step("Step $step: Create a: $lun_capacity_gb GB LUN on rg id: $rg_id"); 
        my $lun = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
        my $lun_id = platform_shim_lun_get_property($lun, 'id');
        my @lun_for_io = ();
        push @luns, $lun;
        push @lun_for_io, $lun;

        # Mark the LUN bound
        $rg_config->{b_additional_lun_bound} = 1;
        $rg_config->{additional_lun_id} = $lun_id;;

        # Verify copy progresses
        $rekey_checkpoint = test_util_encryption_check_rekey_progress($self, $rg,                   
                                                                      $encryption_progress_timeout);
        $rekey_checkpoint_hex = $rekey_checkpoint->as_hex();
        $rg_config->{rekey_checkpoint} = $rekey_checkpoint;
    
        # If rekey is complete log it
        if (test_util_is_copy_complete($rekey_checkpoint)) {
            $self->platform_shim_test_case_log_info("Bound LUN: $lun_id rekey progress rg id: $rg_id type: $raid_type" .
                                                    " rekey complete");
        } else {
            $self->platform_shim_test_case_log_info("Bound LUN: $lun_id rekey progress rg id: $rg_id type: $raid_type" .
                                                    " encryption rekey checkpoint: $rekey_checkpoint_hex");
        }
        

        # Write the background pattern to this LUN
        $expected_io_count += int($lun_io_max_lba / $blocks);
        encryption_start_write_background_pattern($self, $step, $io_sp, 
                                                      \@lun_for_io, 
                                                      $lun_io_max_lba, 
                                                      $blocks);
    }
    $step++;
    
    # Now wait for the writes to complete
    if ($expected_io_count > 0) {
        foreach my $rg_config (@rg_configs) {
            my $rg_id = $rg_config->{raid_group_id};
            my $b_additional_lun_bound = $rg_config->{b_additional_lun_bound};
            my @luns = @{$rg_config->{luns}};
            my $lun_index = scalar(@luns) - 1;
            my $lun = $luns[$lun_index];
            my $lun_id = $rg_config->{additional_lun_id};
            my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
            my $data_disks = $rg_config->{data_disks};
            # Blocks is chunks stripe size
            my $blocks = $data_disks * 0x800;
            my $lun_expected_io_count = int($lun_io_max_lba / $blocks);

            # Only if there was another lun bound wait for the write background 
            # pattern to complete
            if ($b_additional_lun_bound) {
                # Get the lun
                my @lun_for_io = ();
                push @lun_for_io, $lun;

                # Wait for the write background to complete
                encryption_wait_for_io_to_complete($self, $step, $io_sp, $lun_expected_io_count, \@lun_for_io);

                # Start read only I/O to this LUN
                $self->platform_shim_test_case_log_step("Test $test: Step $step: Start read only IO to LUN: $lun_id rg id: $rg_id");
                encryption_start_read_only_io($self, $step, $rg_configs_ref, $io_sp, 
                                                  $io_threads, $lun, $lun_io_max_lba); # Only start I/O on newly bound LUN
            }

        } # end for each raid group
        $step++;
    }

    # Return the status
    return $status;
}
# end encryption_test_bind_lun_verify_rekey_progress

#=
#*           encryption_test_unbind_lun_verify_copy_progress
#*
#* @brief    For any raid group where we bound an additional LUN:
#*              o Stop the read only I/O
#*              o Verify the background pattern
#*              o Unbind the LUN
#*              o Confirm that the copy proceeds
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#* @param    $io_sp - The SP to write background pattern and start read only I/O
#* @param    $io_threads - Number of I/O threads for read only I/O
#* @param    $encryption_progress_timeout - Timeout in seconds for copy to make progress
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_unbind_lun_verify_copy_progress
{
    my ($self, $test, $rg_configs_ref, $io_sp, $io_threads, $encryption_progress_timeout) = @_;
  
    $self->platform_shim_test_case_log_step("Test $test: Unbind the additional LUN on copy raid groups");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});;
    my $io_sp_name = uc(platform_shim_sp_get_property($io_sp, 'name'));
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $step = 1;
    my $status = 0;

    # Change SP if needed
    if ($io_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $io_sp);
    }

    # First check the progress, stop I/O and start the background pattern check
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $dest_disk = $rg_config->{dest_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_id = $rg_config->{dest_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $b_wait_for_percent_increase = 0; # Don't wait for percent increase
        my $b_additional_lun_bound = $rg_config->{b_additional_lun_bound};
        my @luns = @{$rg_config->{luns}};
        my $lun_index = scalar(@luns) - 1;
        my $lun = $luns[$lun_index];
        my $lun_id = $rg_config->{additional_lun_id};;
        my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
        my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
        my $data_disks = $rg_config->{data_disks};
        # Blocks is chunks stripe size
        my $blocks = $data_disks * 0x800;
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Check if the copy is already complete
        if (test_util_is_copy_complete($copy_checkpoint)) {
            $self->platform_shim_test_case_log_info("Unbind LUN check progress source faulted: $b_source_faulted vd: $vd_fbe_id rg id: $rg_id" .
                                                    " source: $source_id destination: $dest_id copy complete");
        } else {
            $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
            $copy_checkpoint_hex = $copy_checkpoint->as_hex();
            $rg_config->{percent_copied} = $percent_copied;
            $rg_config->{copy_checkpoint} = $copy_checkpoint;
            $self->platform_shim_test_case_log_info("Unbind LUN check progress source faulted: $b_source_faulted vd: $vd_fbe_id rg id: $rg_id " .
                                                    " percent copied: $percent_copied checkpoint: $copy_checkpoint_hex");
        }

        # If we bound another LUN stop the background I/O and start checking
        # the background pattern (do NOT unbind it!)
        if ($b_additional_lun_bound) {
            # Get the lun
            my @lun_for_io = ();
            push @lun_for_io, $lun;

            # Stop the read only I/O
            $self->platform_shim_test_case_log_step("Test $test: Step $step: Stop I/O and check pattern on LUN: $lun_id rg id: $rg_id");
            encryption_stop_io($self, $step, $io_sp, $lun);

            # Start checking the background pattern
            encryption_start_check_background_pattern($self, $step, $io_sp,
                                                          \@lun_for_io,
                                                          $lun_io_max_lba,
                                                          $blocks);
        } # end if additional LUN bound
    }
    $step++;

    # Change SP back if needed
    if ($io_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }
    
    # Now un-bind the lun and confirm that the copy proceeds
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $dest_disk = $rg_config->{dest_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_id = $rg_config->{dest_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $b_wait_for_percent_increase = 0; # Don't wait for percent increase
        my $b_additional_lun_bound = $rg_config->{b_additional_lun_bound};
        my $lun_id = $rg_config->{additional_lun_id};
        my @luns = @{$rg_config->{luns}};
        my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
        my $data_disks = $rg_config->{data_disks};
        # Blocks is chunks stripe size
        my $blocks = $data_disks * 0x800;
        my $expected_lun_io_count = int($lun_io_max_lba / $blocks);
        my $b_dest_removed = $rg_config->{b_drive_removed};
        my $b_source_faulted = $rg_config->{b_source_drive_faulted};

        # Skip RAID-0
        if (($raid_type eq 'r0')   ||
            ($raid_type eq 'disk')    ) {
            next;
        }

        # If we bound another LUN unbind it now
        if ($b_additional_lun_bound) {
            # Get the lun
            my $lun = pop @luns;
            my @lun_for_io = ();
            push @lun_for_io, $lun;

            # Wait for the check pattern to complete
            encryption_wait_for_io_to_complete($self, $step, $io_sp, $expected_lun_io_count, \@lun_for_io);

            # Un-bind the LUN
            $self->platform_shim_test_case_log_step("Test $test: Step $step: Unbind LUN: $lun_id rg id: $rg_id");
            platform_shim_lun_unbind($lun);

            # Mark the LUN un-bound
            $rg_config->{b_additional_lun_bound} = 0;

            # Verify copy progresses
            if (!$b_dest_removed) {
                $copy_checkpoint = test_util_copy_check_copy_progress($self, $dest_disk, $dest_id, 
                                                                      \$percent_copied, 
                                                                      $b_wait_for_percent_increase, 
                                                                      $encryption_progress_timeout);
                $copy_checkpoint_hex = $copy_checkpoint->as_hex();
                $rg_config->{percent_copied} = $percent_copied;
                $rg_config->{copy_checkpoint} = $copy_checkpoint;
        
                # If copy is complete log it
                if (test_util_is_copy_complete($copy_checkpoint)) {
                    $self->platform_shim_test_case_log_info("Unbind LUN check progress vd: $vd_fbe_id rg id: $rg_id" .
                                                            " source: $source_id destination: $dest_id copy complete");
                } else {
                    $self->platform_shim_test_case_log_info("Unbind LUN check progress vd: $vd_fbe_id rg id: $rg_id" .
                                                            " source: $source_id destination: $dest_id" .
                                                            " percent copied: $percent_copied checkpoint: $copy_checkpoint_hex");
                }
            }

        } # end if additional LUN bound
    }

    # Return the status
    return $status;
}
# end encryption_test_unbind_lun_verify_copy_progress

#=
#*           encryption_cleanup
#*
#* @brief    Clear EOL and remove any faults on source drives
#*
#* @param    $self - Main argument pointer
#* @param    $test - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_cleanup
{
    my ($self) = @_;
    my $host_audit_log_path = $self->platform_shim_test_case_get_parameter(name => "host_audit_log_path");
     
    $self->platform_shim_test_case_log_step("Encryption cleanup: host audit log path: $host_audit_log_path");
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my @rg_configs = ();
    my $rg_count = scalar(@rg_configs);
    my $rg_index = 0;
    my $status = 0;

    # Always save the audit log
    test_util_encryption_get_audit_log($self, $host_audit_log_path);

    # Now determine if a config was created
    if (defined $self->{rg_config_ref}) {
        @rg_configs = @{$self->{rg_config_ref}};
    } else {
        $self->platform_shim_test_case_log_info("Encryption cleanup: Nothing to cleanup");
        return $status;
    }
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_disk = $rg_config->{dest_disk};
        my $dest_id = $rg_config->{dest_id};
        
        # First re-insert any removed drives
        if ($rg_config->{b_drive_removed}) {
            my @faults = @{$rg_config->{removed_drive_handle}};
            my $fault = pop @faults;
            my $reinsert = platform_shim_disk_reinsert($fault); 
            $self->platform_shim_test_case_log_info("Encryption cleanup: re-insert removed destination: $dest_id vd: $vd_fbe_id rg id: $rg_id");
            $rg_config->{b_drive_removed} = 0;
        }

        # Now clear DRIVE_FAULT
        if ($rg_config->{b_source_drive_faulted}) {
            $self->platform_shim_test_case_log_info("Encryption cleanup: clear DRIVE_FAULT on original source disk: $source_id");
            platform_shim_disk_clear_drive_fault($source_disk, $active_sp);
            $rg_config->{b_source_drive_faulted} = 0;
        }
        
        # Now clear EOL
        if ($rg_config->{b_source_drive_eol}) {
            $self->platform_shim_test_case_log_info("Encryption cleanup: source disk: $source_id clear EOL state");
            eval {
                if (platform_shim_disk_get_eol_state($source_disk)) {
                    # @note Only wait for the last drive to be cleared to become Ready
                    #       takes some time.  Have seen it timeout waiting for `unbound'
                    #       yet EOL is clear and the PVD is ready.
                    if ($rg_index < $rg_count) {
                        # For all except last don't wait for PVD Ready
                        $self->platform_shim_test_case_log_info("Cleanup source disk: $source_id clear EOL state");
                        platform_shim_disk_clear_eol_state($source_disk);
                        if ( my $e = Exception::Class->caught() ) {
                            my $msg = ref($e) ? $e->full_message() : $e;
                            print "message received as exception: " . $msg;
                            $self->logPropertyInfo("Could not clear EOL state on source disk: $source_id", $msg );
                        }
                    } else {
                        # Else this is the last source disk to clear.
                        # Wait for the drive to become Ready
                        my @tempdisks = ($source_disk);
                        $self->platform_shim_test_case_log_info("Cleanup source disk: $source_id clear EOL and wait for Ready");
                        platform_shim_disk_clear_eol($device, $source_disk, \@tempdisks);
                        if ( my $e = Exception::Class->caught() ) {
                            my $msg = ref($e) ? $e->full_message() : $e;
                            print "message received as exception: " . $msg;
                            $self->logPropertyInfo("Could not clear EOL state on source disk: $source_id", $msg );
                        }
                    }
                }
            }; # end eval
            $rg_config->{b_source_drive_eol} = 0;
        } # end if EOL set

        # Goto next raid group
        $rg_index++;
    } 

    # Return the status
    return $status;
}
# end encryption_cleanup

#=
#*          encryption_start_write_background_pattern
#*
#* @brief   Using the $lun_io_max_lba for each raid group, start the I/O
#*          for the luns specified (it does not wait for the writes to complete).
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $target_sp - The SP to send I/O to (this must be the same SP the final
#*              data check is
#* @param   $luns_ref - Address of array of luns to write to
#* @param   $lun_io_max_lba - The maximum lba on each lun to write to
#* @param   $blocks_per_io - The fixed I/O size for the writes
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_start_write_background_pattern
{
   my ($self, $step, $target_sp, $luns_ref, $lun_io_max_lba, $blocks_per_io) = @_;
     
    my $device = $self->{device};
    my @luns = @{$luns_ref};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
    my $blocks_per_io_hex = sprintf "0x%lx", $blocks_per_io;
    my $status = 0;

    # Log the I/O an on which SP
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, 'id');
        $self->platform_shim_test_case_log_info("Step $step: Start write background pattern on: $target_sp_name" .
                                                " lun id: $lun_id io max lba: $lun_io_max_lba_hex blocks: $blocks_per_io_hex");
    }
    
    # If neccessary change the target SP
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $target_sp);
    }

    # Write the background pattern
    test_util_io_start_write_background_pattern($self, {maximum_lba => $lun_io_max_lba, 
                                                luns => $luns_ref, 
                                                block_count => $blocks_per_io});

    # If neccessary change the target SP back to the current active
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }

    # Return $status
    return $status;
}
# end encryption_start_write_background_pattern

#=
#*          encryption_wait_for_io_to_complete
#*
#* @brief   Wait for the previously started I/O to complete
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $target_sp - The SP to send I/O to (this must be the same SP the final
#*              data check is
#* @param   $expected_io_count - The number of I/Os expected
#* @param   $luns_ref - Address of array of luns to wait for (optional)
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_wait_for_io_to_complete
{
   my ($self, $step, $target_sp, $expected_io_count, $luns_ref) = @_;
     
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my @luns = ();
    my $status = 0;

    # If neccessary change the target SP
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $target_sp);
    }

    # If $luns_ref defined then just check the number of I/Os for LUNs
    if (defined $luns_ref) {
        @luns = @{$luns_ref};
        my $num_luns = scalar(@luns);
        my $lun_expected_io_count = int($expected_io_count / $num_luns);
        my $lun_id = platform_shim_lun_get_property($luns[0], 'id');
        # Log the I/O an on which SP
        $self->platform_shim_test_case_log_step("Step $step: Wait for: $lun_expected_io_count I/Os for: $num_luns LUNs lun id: $lun_id to complete on: $target_sp_name");
        foreach my $lun (@luns) {
            if ($b_rdgen_need_to_stop_io_before_starting_more) {
                test_util_io_wait_rdgen_lun_io_complete($self, 3600*3, $lun_expected_io_count, $lun);
                test_util_io_verify_rdgen_io_complete($self, $lun_expected_io_count, $lun);
            } else {
                test_util_io_wait_rdgen_thread_complete($self, 3600*3, $lun);
                test_util_io_verify_rdgen_io_complete($self, $lun_expected_io_count, $lun);
            }
        }
    } else {
        # Else validate that ALL I/O is stopped
        $self->platform_shim_test_case_log_step("Step $step: Wait for ALL I/O to complete on: $target_sp_name");
        test_util_io_wait_rdgen_thread_complete($self, 3600*3);
        test_util_io_verify_rdgen_io_complete($self, $expected_io_count);
    }

    # If neccessary change the target SP back to the current active
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }

    # Return $status
    return $status;
}
# end encryption_wait_for_io_to_complete

#=
#*          encryption_start_check_background_pattern
#*
#* @brief   Using the $lun_io_max_lba for each raid group, start the I/O
#*          for the luns specified (it does not wait for the reads to complete).
#*
#* @param   $self - Main argument pointer
#* @param   $step - The current step of the test case 
#* @param   $target_sp - The SP to send I/O to (this must be the same SP the final
#*              data check is
#* @param   $luns_ref - Address of array of luns to write to
#* @param   $lun_io_max_lba - The maximum lba on each lun to write to
#* @param   $blocks_per_io - The fixed I/O size for the writes
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_start_check_background_pattern
{
   my ($self, $step, $target_sp, $luns_ref, $lun_io_max_lba, $blocks_per_io) = @_;
     
    my $device = $self->{device};
    my @luns = @{$luns_ref};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $target_sp_name = uc(platform_shim_sp_get_property($target_sp, 'name'));
    my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
    my $blocks_per_io_hex = sprintf "0x%lx", $blocks_per_io;
    my $status = 0;

    # Log the I/O an on which SP
    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, 'id');
        $self->platform_shim_test_case_log_info("Step $step: Start check background pattern on: $target_sp_name" .
                                                " lun id: $lun_id io max lba: $lun_io_max_lba_hex blocks: $blocks_per_io_hex");
    }
    
    # If neccessary change the target SP
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $target_sp);
    }

    # Check the background pattern
    test_util_io_start_check_pattern($self, {maximum_lba => $lun_io_max_lba, 
                                     luns => $luns_ref, 
                                     block_count => $blocks_per_io});

    # If neccessary change the target SP back to the current active
    if ($target_sp_name ne $active_sp_name) {
        platform_shim_device_set_target_sp($device, $active_sp);
    }

    # Return $status
    return $status;
}
# end encryption_start_check_background_pattern

#=
#*          encryption_write_background_pattern_to_all_luns
#*
#* @brief   Using the $lun_io_max_lba for each raid group, write and then
#*          verify the background pattern.
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current task of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $io_sp - The SP to send I/O to (this must be the same SP the final
#*              data check is done on)
#* @param   $io_physical_percent - Percent of bound capacity to send I/O to
#* @param   $b_check_pattern - True - Validate that the pattern has been written
#*
#* @note    Write background pattern to all LUNs. We need to limit the amount of 
#*          data copied but at the same time make sure the copy proceeds slowly.  
#*          Therefore the value for $lun_io_max_lba is crucial.  The goal is to 
#*          have all the copy operations take approximately 10 minutes.
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_write_background_pattern_to_all_luns 
{
   my ($self, $task, $rg_configs_ref, $io_sp, $io_physical_percent, $b_check_pattern) = @_;
     
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $device = $self->{device};
    my $step = 1;
    my $status = 0;

    # Log some info
    $self->platform_shim_test_case_log_step("Task $task: Write background pattern for: $io_physical_percent percent of consumed capacity");
    $self->platform_shim_test_case_log_info("            check pattern: $b_check_pattern");

    # Setup rdgen
    platform_shim_device_initialize_rdgen($device);
    platform_shim_device_rdgen_reset_stats($device);
    if (platform_shim_test_case_is_functional($self)) {
        platform_shim_device_set_target_sp($device, $io_sp); 
        platform_shim_device_initialize_rdgen($device);
        platform_shim_device_rdgen_reset_stats($device);
    }

    # Now walk thru each LUN and start the writes
    my $expected_io_count = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my @luns = @{$rg_config->{luns}};
        my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
        my $num_luns = scalar(@luns);
        my $data_disks = $rg_config->{data_disks};
        # Blocks is chunks stripe size
        my $blocks = $data_disks * 0x800;
        # Add the expected I/O for each raid group
        $expected_io_count += int($lun_io_max_lba / $blocks) * $num_luns;
        
        # Start the write I/O
        encryption_start_write_background_pattern($self, $step, $io_sp, 
                                                      \@luns, 
                                                      $lun_io_max_lba, 
                                                      $blocks);
    }
    $step++;  

    # Now wait for the writes to complete
    $self->platform_shim_test_case_log_step("Step $step: Wait for rdgen set pattern to complete ios: $expected_io_count");
    encryption_wait_for_io_to_complete($self, $step++, $io_sp, $expected_io_count / 2);

    # If set verify the data
    if ($b_check_pattern) {
        $expected_io_count = 0;
        foreach my $rg_config (@rg_configs) {
            my $rg = $rg_config->{raid_group};
            my $rg_id = $rg_config->{raid_group_id};
            my @luns = @{$rg_config->{luns}};
            my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
            my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
            my $num_luns = scalar(@luns);
            my $data_disks = $rg_config->{data_disks};
            # Blocks is chunks stripe size
            my $blocks = $data_disks * 0x800;
            # Add the expected I/O for each raid group
            $expected_io_count += int($lun_io_max_lba / $blocks) * $num_luns;
        
            # Start the check of the background pattern
            encryption_start_check_background_pattern($self, $step, $io_sp,
                                                          \@luns,
                                                          $lun_io_max_lba,
                                                          $blocks);
        }
        $step++;

        # Now wait for the reads to complete
        $self->platform_shim_test_case_log_step("Step $step: Wait for rdgen check pattern to complete ios: $expected_io_count");
        encryption_wait_for_io_to_complete($self, $step++, $io_sp, $expected_io_count / 2);

    } # end if check pattern

    # Return $status
    return $status;
}
# end encryption_write_background_pattern_to_all_luns

#=
#*          encryption_test_reboot_passive_during_encrption
#*
#* @brief   Validate that encryption continues after reboot
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current step of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $passive_sp - The current CMI passive SP
#* @param   $active_sp_ref - Address of active SP to update
#* @param   $passive_sp_ref - Address of passive SP to update
#* @param   $b_wait_for_sp_ready_after_reboot - True wait for peer to reboot before proceeding
#* @param   $encryption_progress_timeout - How long to wait for each raid group to make progress on encryption
#* @param   $lun_io_threads - Number of per LUN I/O threads
#*
#* @return  $status - The status of this operation
#*
#=cut
sub encryption_test_reboot_passive_during_encryption
{

    my ($self, $test, $rg_configs_ref, $passive_sp, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot, $encryption_progress_timeout, $lun_io_threads) = @_;
     
    $self->platform_shim_test_case_log_step("Test $test: Reboot Passive SP and start new copy");
    my @rg_configs = @{$rg_configs_ref};
    my $io_sp = $$active_sp_ref;
    my $step = 1;
    my $status = 0;

    # Wait for now passive to be ready
    $status = encryption_wait_for_sp_ready($self, $step++, $passive_sp, $b_wait_for_sp_ready_after_reboot);

    # Reboot the passive    
    $status = encryption_reboot_sp($self, $step++, $passive_sp, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot);

    # Validate that encryption rekey is making progress
    $status = encryption_verify_rekey_progress($self, $step++, $rg_configs_ref, $encryption_progress_timeout);

    # Return the status
    return $status;
}
# end encryption_test_reboot_passive_during_encryption

#=
#*          encryption_start_initial_read_only_io
#*
#* @brief   Start read only I/O to all bound luns with the number of threads
#*          specified.
#*
#* @param   $self - Main argument pointer
#* @param   $task - The current task of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $io_sp - The SP to send I/O to (this must be the same SP the final
#*              data check is done on)
#* @param   $lun_io_threads - How many I/O threads per LUN
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_start_initial_read_only_io
{
   my ($self, $task, $rg_configs_ref, $io_sp, $lun_io_threads) = @_;
     
    my $step = 1;
    my $status = 0;

    $self->platform_shim_test_case_log_step("Task $task: Start read only I/O to all bound LUNs");
    $status = encryption_start_read_only_io($self, $step++, $rg_configs_ref, $io_sp,
                                                $lun_io_threads);

    # Return $status
    return $status;
}
# end encryption_start_initial_read_only_io

#=
#*          encryption_test_reboot_both_sps_and_start_io
#*
#* @brief   Reboot both SPs and re-start I/O on the originla I/O SP
#*
#* @param   $self - Main argument pointer
#* @param   $test - The current test of the test case 
#* @param   $rg_configs_ref  - Address of array of raid group configurations under test
#* @param   $io_sp - The SP to start I/O on
#* @param   $active_sp_ref - Address of active SP to update
#* @param   $passive_sp_ref - Address of passive SP to update
#* @param   $lun_io_threads - Number of read only threads per lun
#* @param   $b_wait_for_sp_ready_after_reboot - If True we have already
#*              wait so there is no need to do anything.
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_test_reboot_both_sps_and_start_io
{
    my ($self, $test, $rg_configs_ref, $io_sp, $active_sp_ref, $passive_sp_ref, $lun_io_threads, $b_wait_for_sp_ready_after_reboot) = @_;
     
    my $active_sp = $$active_sp_ref;
    my $passive_sp = $$passive_sp_ref;
    my $step = 1;
    my $status = 0;

    $self->platform_shim_test_case_log_step("Test $test: Reboot both SPs and start read only I/O to all LUNs");

    # Wait for now passive to be ready
    $status = encryption_wait_for_sp_ready($self, $step++, $passive_sp, $b_wait_for_sp_ready_after_reboot);

    # Stop I/O on all LUNs
    $status = encryption_stop_io($self, $step, $io_sp);

    # Reboot the passive and active
    my $current_active_sp = $active_sp;
    $status = encryption_reboot_sp($self, $step++, $passive_sp, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot);
    $status = encryption_reboot_sp($self, $step++, $current_active_sp, $active_sp_ref, $passive_sp_ref, $b_wait_for_sp_ready_after_reboot);

    # Wait for both SPs to be ready
    $status = encryption_wait_for_sp_ready($self, $step++, $active_sp, $b_wait_for_sp_ready_after_reboot);
    $status = encryption_wait_for_sp_ready($self, $step++, $passive_sp, $b_wait_for_sp_ready_after_reboot);

    # Start I/O on the original I/O SP on all LUNs
    $status = encryption_start_read_only_io($self, $step++, $rg_configs_ref, $io_sp,
                                                $lun_io_threads);

    # Return status
    return $status;
}
#end encryption_test_reboot_both_sps_and_start_io

#=
#*          encryption_check_background_pattern_on_all_luns
#*
#* @brief   For all raid groups walk thru each lun and start the
#*          check of the background pattern.  Then wait for all I/O
#*          to complete.
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#* @param    $io_sp - The SP to write background pattern and start read only I/O
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_check_background_pattern_on_all_luns
{
    my ($self, $step, $rg_configs_ref, $io_sp) = @_;
  
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $io_sp_name = uc(platform_shim_sp_get_property($io_sp, 'name'));
    my $expected_io_count = 0;
    my $status = 0;

    # First each raid group use the per-lun max io lba and start the data
    # pattern check
    $self->platform_shim_test_case_log_step("Step $step: Validate background pattern on all LUNs on: $io_sp_name");
    foreach my $rg_config (@rg_configs) {
        my @luns = @{$rg_config->{luns}};
        my $num_luns = scalar(@luns);
        my $lun_io_max_lba = $rg_config->{lun_io_max_lba};
        my $data_disks = $rg_config->{data_disks};
        # Blocks is chunks stripe size
        my $blocks = $data_disks * 0x800;
        $expected_io_count += int($lun_io_max_lba / $blocks) * $num_luns;

        # Start the data pattern check to all LUNs
        encryption_start_check_background_pattern($self, $step, $io_sp,
                                                      \@luns,
                                                      $lun_io_max_lba,
                                                      $blocks);
    }
    
    # Now wait for the check to complete on all LUNs
    encryption_wait_for_io_to_complete($self, $step, $io_sp, $expected_io_count / 2);

    # Return the status
    return $status;
}
# end encryption_check_background_pattern_on_all_luns

#=
#*          encryption_check_background_pattern_after_test_complete
#*
#* @brief   For all raid groups walk thru each lun and start the
#*          check of the background pattern.  The wait for all I/O
#*          to complete.
#*
#* @param    $self - Main argument pointer
#* @param    $task - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#* @param    $io_sp - The SP to write background pattern and start read only I/O
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_check_background_pattern_after_test_complete
{
    my ($self, $task, $rg_configs_ref, $io_sp) = @_;
  
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $io_sp_name = uc(platform_shim_sp_get_property($io_sp, 'name'));
    my $expected_io_count = 0;
    my $step = 1;
    my $status = 0;

    # First each raid group use the per-lun max io lba and start the data
    # pattern check
    $self->platform_shim_test_case_log_step("Task $task: Validate background pattern on all LUNs on: $io_sp_name");
    $status = encryption_check_background_pattern_on_all_luns($self, $step++, $rg_configs_ref, $io_sp);

    # Return the status
    return $status;
}
# end encryption_check_background_pattern_after_test_complete

#=
#*          encryption_cleanup_debug_and_rdgen
#*
#* @brief   Clear any debug flags and 
#*
#* @param    $self - Main argument pointer
#* @param    $task - The current step of the test case 
#* @param    $rgs_ref  - Address of array of raid groups under test
#* @param    $active_sp - Active
#* @param    $passive_sp - Passive
#*
#* @return   $status - The status of this operation
#*
#=cut
sub encryption_cleanup_debug_and_rdgen
{
    my ($self, $task, $rgs_ref, $active_sp, $passive_sp) = @_;

    my $device = $self->{device};
    my $status = 0;

    $self->platform_shim_test_case_log_step("Task $task: Clear any debug flags and cleanup rdgen");
    platform_shim_device_cleanup_rdgen($device);
    test_util_io_set_raid_group_debug_flags($self, $active_sp, $rgs_ref, 0);
    if (platform_shim_test_case_is_functional($self)) {
        platform_shim_device_set_target_sp($device, $passive_sp);
        platform_shim_device_cleanup_rdgen($device);
        test_util_io_set_raid_group_debug_flags($self, $passive_sp, $rgs_ref, 0);
        platform_shim_device_set_target_sp($device, $active_sp);
    }
    
    # Return $status
    return $status;
}
# end encryption_cleanup_debug_and_rdgen

1;
