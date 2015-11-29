package mcr_test::user_copy::user_copy;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_user_copy;
use mcr_test::test_util::test_util_copy;
use mcr_test::test_util::test_util_io;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::test_util::test_util_rebuild;
use strict;
use warnings;

# ==================User Copy Test Cases===========================================
# 
# 1) Test that the start of a user copy from A -> B fails if A is a system disk with no user LUNs and B is unbound disk.
# 2) create rg of each type.
# 3)Copy of any disk in a RG with no LUNs fails. We only do this for the first RG.
# 4) create LUNs.
# 5) Either 
#    A) Validate a Copy on a non-redundant raid group fails. (RAID-0).
#    or
#    B) For other RAID types start a user copy 2 LUNs, 1 RG.  
#
# 6) Look for movement in the checkpoint to ensure that the copy has started.
#
# 7) Look for additional movement to make sure the copy is making progress.
#
# 8) Unbind the first LUN in a RAID Group and make sure the copy still is in progress.
#
# 9) a) If a copy A -> B is in progress, validate that a copy C -> A fails.
#    b) User copy of a drive fails if the drive is copying already.
#    c) User copy.  Validate that if A -> B copy is in progress C -> B copy request fails.
#    d) If A -> B is in progress, Copy B -> C fails.
#
# 10) A -> B copy fails if A and B are both in RAID Groups.
# 
# Tests below also run in functional tests:
# 
# *)  User copy, LUN bound above checkpoint.  I/Os run and copy completes successfully.
# *)  User copy.  LUN above copy checkpoint is unbound.  Make sure copy completes without issue.
# *)  Reboot active SP.  Ensure copy continues on passive SP.
# *)  Reboot both SPs.  Ensure copy continues upon reboot.
# *)  Cause Permanent spare of A -> B.  B Rebuilds.  If A is reinserted, make sure B -> A copy is allowed.
# *)  If the destination disk removed then copy to aborts after timer expires.  
# *) Given that redundant raid group is degraded a copy to should fail.
# *) Allow all copies to complete. Verify destination, not source is in RG after copy.  Verify source alive. 
# ======================================================================================
sub main
{
   test_util_general_test_start();
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    my $device = $self->{device};
    my $luns_per_rg     = 1;                  
    my $per_disk_capacity_gb = 40;
    my $lun_io_capacity_percent = 2;
    my $lun_io_max_lba = 0x10000;
    my $lun_io_threads = 4;
    my $copy_complete_timeout = 4 * 60 * 60;  # 4 hours
    my $skip_reboot = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($skip_reboot == 1) {
       $self->platform_shim_test_case_log_info("Skipping reboot during test since skip_reboot parameter set.\n");
    }
    # We do NOT disable load balancing since we want some parent raid groups NOT
    # on the CMI Active SP
    platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});

    my $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
    my $passive_sp_name = uc(platform_shim_sp_get_property($passive_sp, 'name'));
    $self->platform_shim_test_case_log_step("Active SP: " . $active_sp_name . " Passive SP: " . $passive_sp_name);

    test_util_general_set_target_sp($self, $active_sp);
    #platform_shim_device_disable_automatic_sparing($device);
    platform_shim_device_enable_automatic_sparing($device);
    
    # Check that we get an error when we try to start a user copy on an unbound system drive.
    user_copy_for_unbound_system_drive($self);

    # Create the raid groups, different widths and drive types, parity
    my %rg_params = (id => '0');
    my @rgs = platform_shim_device_get_raid_group($self->{device});
    my $total_rgs = scalar(@rgs);
    $self->platform_shim_test_case_log_info("found $total_rgs raid groups");

    # Step 1: Based on the platform under test configure the following:
    #           o $luns_per_rg
    #           o $per_disk_capacity_gb
    #           o $lun_io_capacity_percent
    #           o $lun_io_max_lba
    $self->platform_shim_test_case_log_info("Configure test parameters (luns per rg, lun io percent, etc) based on platform");
    user_copy_configure_params_based_on_platform($self, \@rgs, \$luns_per_rg, \$per_disk_capacity_gb, 
                                                 \$lun_io_capacity_percent, \$lun_io_max_lba, \$lun_io_threads);
    if (platform_shim_device_is_simulator($device)){
       my $slow_copy_speed = 1;
       my $default_rg_reb_speed = 1;
       #$self->platform_shim_test_case_log_step("Slow down copy operations");
       #test_util_set_copy_operation_speed($self, $active_sp, $slow_copy_speed, \$default_rg_reb_speed);
    }

    # Step 2: Set up test cases for each raid group
    $self->platform_shim_test_case_log_info("Create raid groups and start user copy on random position");
    my @rg_configs = @{user_copy_initialize_rg_configs($self, $luns_per_rg, $per_disk_capacity_gb)};

    # Write background pattern to all LUNs
    # We need to limit the amount of data copied but at the same time
    # make sure the copy proceeds slowly.  Therefore the value for
    # $max_io_lba_per_lun is crucial.  The goal is to have all the copy
    # operations take approximately 10 minutes.
    test_util_io_set_raid_group_debug_flags($self, $active_sp, \@rgs, 1); 
    my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
    platform_shim_device_initialize_rdgen($device);
    my $orig_write_sp = $active_sp;
    my $orig_write_sp_name = $active_sp_name;
    $self->platform_shim_test_case_log_info("Write background pattern ($orig_write_sp_name) to the first: $lun_io_capacity_percent percent of each LUN");
    user_copy_write_background_pattern($self, \@rg_configs, $luns_per_rg, $per_disk_capacity_gb, $lun_io_capacity_percent, $lun_io_threads);
    #user_copy_read_background_pattern($self, \@rg_configs, $luns_per_rg, $per_disk_capacity_gb, $lun_io_capacity_percent, $lun_io_threads);

    # Start a read only I/O thread
     platform_shim_device_initialize_rdgen($device);
    
    user_copy_start_random_read_only_io($self, \@rg_configs, $luns_per_rg, $lun_io_threads);
    user_copy_start_copy($self, \@rg_configs);
    user_copy_display_progress($self, \@rg_configs);
    
     

    if (platform_shim_test_case_is_functional($self)) {
       # (optional) Make sure copy makes more progress
       #user_copy_check_progress($self, \@rg_configs);
       
       # Unbind the LUN that is running copy.
             
       user_copy_unbind_first_lun($self, \@rg_configs);


      
       # Make sure the copy makes more progress (we cannot wait for it to complete)
       user_copy_validate_start_copy_error($self, \@rg_configs);

       # Make sure copy fails if source and destination are in bound RAID Groups.
       user_copy_destination_bound_drive_error($self, \@rg_configs);
       
       # Bind and unbind a LUN above the copy checkpoint
      user_copy_bind_above_checkpoint($self, \@rg_configs);
       
     
       
       my $slow_copy_speed = 1;
       my $default_rg_reb_speed = 1;
       #$self->platform_shim_test_case_log_step("Slow down copy operations");
       #test_util_set_copy_operation_speed($self, $active_sp, $slow_copy_speed, \$default_rg_reb_speed);
    }

    if (platform_shim_test_case_is_functional($self)) {
    	
    	
       user_copy_display_progress($self, \@rg_configs);
   
       $self->platform_shim_test_case_log_step("Stop I/O.");
       user_copy_stop_io($self, \@rg_configs);
       $self->platform_shim_test_case_log_step("Stop I/O Successful.");

       $self->platform_shim_test_case_log_info("Wait for copy completion before reboot");
       user_copy_wait_for_copy_complete($self, 6, \@rg_configs, $copy_complete_timeout);

       $self->platform_shim_test_case_log_step("Start read only I/O.");
       test_util_general_set_target_sp($self, $passive_sp);
       platform_shim_device_initialize_rdgen($device);
      user_copy_start_random_read_only_io($self, \@rg_configs, $luns_per_rg, $lun_io_threads);
       $self->platform_shim_test_case_log_step("Read only I/O started.");

       $self->platform_shim_test_case_log_info("Start copy before Active SP Reboot.");
       test_util_general_set_target_sp($self, $active_sp);
       user_copy_start_copy($self, \@rg_configs);
       user_copy_display_progress($self, \@rg_configs);
       # (optional) Make sure it starts.
       #user_copy_check_start($self, \@rg_configs);

       $self->platform_shim_test_case_log_info("reboot active sp " . $active_sp_name);

      if ($skip_reboot == 0) {
          platform_shim_sp_reboot($active_sp);
       }
       # Verify that Copy continues on the passive SP. (passive becomes active) 
       $self->platform_shim_test_case_log_info("confirm that copy continues on passive sp"); 

       test_util_general_set_target_sp($self, $passive_sp);
      user_copy_display_progress($self, \@rg_configs);

       # Wait for the panicked SP to come up. 
       $self->platform_shim_test_case_log_info("wait for active sp ". $active_sp_name . " to come back up");
       platform_shim_sp_wait_for_agent($active_sp, 3600);
       $self->platform_shim_test_case_log_info("active sp " . $active_sp_name . " is back up");
       user_copy_display_progress($self, \@rg_configs);

       $active_sp = platform_shim_device_get_active_sp($device, {});
       $passive_sp = platform_shim_device_get_passive_sp($device, {});
       $active_sp_name = uc(platform_shim_sp_get_property($active_sp, 'name'));
       $passive_sp_name = uc(platform_shim_sp_get_property($passive_sp, 'name'));

       $self->platform_shim_test_case_log_step("Stop I/O.");
      user_copy_stop_io($self, \@rg_configs);
       $self->platform_shim_test_case_log_step("Stop I/O Successful.");

       $self->platform_shim_test_case_log_info("Wait for copy completion before Dual SP Reboot");
      user_copy_wait_for_copy_complete($self, 6, \@rg_configs, $copy_complete_timeout);

       $self->platform_shim_test_case_log_step("Start read only I/O.");
     test_util_general_set_target_sp($self, $passive_sp);
      platform_shim_device_initialize_rdgen($device);
       user_copy_start_random_read_only_io($self, \@rg_configs, $luns_per_rg, $lun_io_threads);
       $self->platform_shim_test_case_log_step("Start read only I/O Successful.");
       test_util_general_set_target_sp($self, $active_sp);

       $self->platform_shim_test_case_log_info("Start copy before Dual SP Reboot.");
       test_util_general_set_target_sp($self, $active_sp);
       user_copy_start_copy($self, \@rg_configs);
     user_copy_display_progress($self, \@rg_configs);
    
       # Reboot both SPs
       $self->platform_shim_test_case_log_info("reboot both sps " . $active_sp_name . " And then " . $passive_sp_name);
       if ($skip_reboot == 0) {
          platform_shim_sp_reboot($active_sp);
          platform_shim_sp_reboot($passive_sp);
       }
       # Wait for both SPs to come up
       $self->platform_shim_test_case_log_step("Waiting for Active SP " . $active_sp_name . " to reboot.");
       platform_shim_sp_wait_for_agent($active_sp, 3600);
       $self->platform_shim_test_case_log_info("Active SP " . $active_sp_name. " Finished rebooting");
       user_copy_display_progress($self, \@rg_configs); 

       $self->platform_shim_test_case_log_step("Start read only I/O.");
       test_util_general_set_target_sp($self, $active_sp);
       platform_shim_device_initialize_rdgen($device);
       user_copy_start_random_read_only_io($self, \@rg_configs, $luns_per_rg, $lun_io_threads);
       $self->platform_shim_test_case_log_step("Start read only I/O Successful.");

       user_copy_display_progress($self, \@rg_configs);

      $self->platform_shim_test_case_log_info("Wait for Passive SP ". $passive_sp_name . " to reboot.");
       platform_shim_sp_wait_for_agent($passive_sp, 3600);
       $self->platform_shim_test_case_log_info("Passive SP ". $passive_sp_name . " Finished rebooting");
       $self->platform_shim_test_case_log_info("Both SPs Finished rebooting");
       user_copy_display_progress($self, \@rg_configs); 
    } 

    # Stop light read only I/O    
    $self->platform_shim_test_case_log_step("Stop I/O.");
    user_copy_stop_io($self, \@rg_configs);
    $self->platform_shim_test_case_log_step("Stop I/O Successful.");


    # Validate data pattern and clear debug flags
    $self->platform_shim_test_case_log_info("Validate data. ($orig_write_sp_name)");
    test_util_general_set_target_sp($self, $orig_write_sp);
    platform_shim_device_initialize_rdgen($device);
          
    user_copy_read_background_pattern($self, \@rg_configs, $luns_per_rg, $per_disk_capacity_gb, $lun_io_capacity_percent, $lun_io_threads);
    $self->platform_shim_test_case_log_info("Validate data Successful.");
    platform_shim_device_cleanup_rdgen($device);
    test_util_io_set_raid_group_debug_flags($self, $active_sp, \@rgs, 0); 

    if (platform_shim_test_case_is_functional($self)) {

       $self->platform_shim_test_case_log_info("Wait for copy completion before faulting drive");
       user_copy_wait_for_copy_complete($self, 6, \@rg_configs, $copy_complete_timeout);

       # Set copy timer so permanent sparing starts immediately.
       platform_shim_device_set_sparing_timer($device, 5);
       user_copy_fault_destination($self, \@rg_configs);

       user_copy_wait_for_copy_complete($self, 6, \@rg_configs, $copy_complete_timeout);

       # Set timer so that permanent sparing does not start.
       platform_shim_device_set_sparing_timer($device, 3600);
       user_copy_degraded_raid_group($self, \@rg_configs);
    }
    
    # disable logical error injection
    #platform_shim_device_disable_logical_error_injection($device);
    
    # restore load balancing
    platform_shim_device_enable_load_balancing($device);

    #$self->platform_shim_test_case_log_step("Validate no error traces are present");
    #platform_shim_device_check_error_trace_counters($self->{device});

    test_util_general_log_test_time($self);
}

sub user_copy_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_per_disk_gb) = @_;
   my @rgs;
   my $device = $self->{device};

   if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
      @rgs = platform_shim_device_get_raid_group($device, {});
   } elsif (platform_shim_test_case_is_functional($self)) {
      @rgs = @{test_util_configuration_create_all_raid_types_reserve($self)};
   } else {
      @rgs = @{test_util_configuration_create_limited_raid_types_reserve($self)};
   }
   
   $self->platform_shim_test_case_log_info("Initialize raid group configs from array of raid groups");
   my @rg_configs = ();
   my $rg_index = 0;
   my $tested_empty_rg_copy = 0;

   foreach my $rg (@rgs) {
      my $rg_id = platform_shim_raid_group_get_property($rg, 'id'); 
      my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
      my $width = platform_shim_raid_group_get_property($rg, 'raid_group_width');
      my @disks = platform_shim_raid_group_get_disk($rg);
      
      my $num_disks_to_remove = 0;
      my $copy_position = int(rand($width));
      
      # Setup the test case   
      my %rg_config = ();
      $rg_config{disks} = \@disks;
      $rg_config{raid_group_id} = $rg_id;
      $rg_config{raid_group} = $rg;
      $rg_config{lun_per_rg} = $luns_per_rg;
      $rg_config{position_to_copy} = $copy_position;
      $rg_config{source_disk} = $disks[$copy_position];
      $rg_config{vd_fbe_id_hex} = platform_shim_disk_get_property($rg_config{source_disk}, 'vd_fbe_id');
      $rg_config{width} = $width;
      $rg_config{raid_type} = $raid_type;
      $rg_config{percent_copied} = 0;
      $rg_config{copy_checkpoint} = Math::BigInt->from_hex("0x0");
      my $copy_checkpoint_hex = $rg_config{copy_checkpoint}->as_hex();
      my $data_disks = test_util_configuration_get_data_disks($self, $rg);
      $rg_config{data_disks} = $data_disks;

      my @disks_to_fault = (shift(@disks));
      if ($rg_config{raid_type} eq 'r6') {
         push @disks_to_fault , shift(@disks);
      }
      $rg_config{disks_to_fault} = \@disks_to_fault;

      my $source_id = platform_shim_disk_get_property($rg_config{source_disk}, 'id');
      $rg_config{source_id} = $source_id;
      $self->platform_shim_test_case_log_info('Raid Group ' . $rg_config{raid_group_id} . 
                                              ' Raid Type ' . $rg_config{raid_type} . 
                                              ' Width ' . $rg_config{width} .
                                              ' source pos ' . $copy_position . 
                                              ' ' . $source_id .
                                              ' kind: ' . platform_shim_disk_get_property($rg_config{source_disk}, 'kind').
                                              ' technology: ' . platform_shim_disk_get_property($rg_config{source_disk},'technology'));

      if (($raid_type ne "r0") and ($tested_empty_rg_copy == 0)) {
         my %copy = test_util_find_copy_disk_for_rg($self, $rg);
         $rg_config{destination_disk} = $copy{'dest'};
         if (defined($rg_config{destination_disk})) {
            my $dest_id = platform_shim_disk_get_property($rg_config{destination_disk}, 'id');
            $rg_config{dest_id} = $dest_id;

            $self->platform_shim_test_case_log_info('Start copy on RG with no LUNs and expect errors');
            $self->platform_shim_test_case_log_step('Starting copy on RG: ' . $rg_config{raid_group_id} .
                                                    ' of type ' .  $rg_config{raid_type}.
                                                    ' position ' . $copy_position .
                                                    ' disk ' . $source_id .
                                                    ' to disk ' . $dest_id); 
            test_util_start_copy_with_error($self, $rg_config{source_disk}, $rg_config{destination_disk});
            $self->platform_shim_test_case_log_step('Start copy on RG with no LUNs and error encountered as expected.');
            $tested_empty_rg_copy = 1;
         } else {
            $self->platform_shim_test_case_log_info('Skip start copy on empty raid group since no suitable spares.');
         }
      } 
      my @luns = ();
      # If `clean_config' is not set then use the exist luns
      if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
          # Use existing luns
          @luns = platform_shim_raid_group_get_lun($rg);
      } else {
          # Bind `luns_per_rg'
         my $lun_capacity_gb = $rg_config{data_disks} * $lun_capacity_per_disk_gb;
         $lun_capacity_gb = test_util_configuration_get_per_lun_capacity($self, \%rg_config, $lun_capacity_gb, $luns_per_rg + 1);

         $self->platform_shim_test_case_log_step('RG ' . $rg_config{raid_group_id} .
                                                 ' lun capacity GB: ' . $lun_capacity_gb .
                                                 ' type ' . $rg_config{raid_type} .
                                                 ' width ' . $rg_config{width});
          foreach (1..$luns_per_rg ) {
              my $lun = test_util_configuration_create_lun_no_initial_verify($self, $rg, $lun_capacity_gb."GB");
              push @luns, $lun;
          }
      }
      my $num_luns = @luns;

      $rg_config{luns} = \@luns;

      # Return the address of the raid groups test configs created
      if ($raid_type ne "r0") {
         # Push the test case
         push @rg_configs, \%rg_config;
      } else {
         my %copy = test_util_find_copy_disk_for_rg($self, $rg);
         $rg_config{destination_disk} = $copy{'dest'};
         if (defined($rg_config{destination_disk})) {
            my $dest_id = platform_shim_disk_get_property($rg_config{destination_disk}, 'id');
            $rg_config{dest_id} = $dest_id;

            # Do not push this config, this is the only test we will run with it.
            $self->platform_shim_test_case_log_info('Start copy on R0 and expect errors.');
            $self->platform_shim_test_case_log_step('Starting copy on R0 ' . $rg_config{raid_group_id} .
                                                    ' of type ' .  $rg_config{raid_type}.
                                                    ' position ' . $copy_position .
                                                    ' disk ' . $source_id .
                                                    ' to disk ' . $dest_id); 
            test_util_start_copy_with_error($self, $rg_config{source_disk}, $rg_config{destination_disk});
         } else {
            $self->platform_shim_test_case_log_info('Skip start copy on R0 since no suitable spares.');
         }
      }
   }
   # Return the address of the raid groups test configs created
   return \@rg_configs;
}
sub user_copy_start_copy
{
   my ($self, $rg_configs_ref) = @_;

   my @rg_configs = @{$rg_configs_ref};
   $self->platform_shim_test_case_log_info("Start copy on all raid groups");

   foreach my $rg_config (@rg_configs) {
      my $rg = $rg_config->{raid_group};
      my $width = $rg_config->{width};
      my @disks = platform_shim_raid_group_get_disk($rg); 
#      my @disks = @{$rg_config->{disks}};
      my $copy_position = int(rand($width));
      
      $rg_config->{position_to_copy} = $copy_position;
      
      $self->platform_shim_test_case_log_step("find copy disk for RG " . $rg_config->{raid_group_id});

      platform_shim_raid_group_mark_dirty($rg);
      my %copy = test_util_find_copy_disk_for_rg($self, $rg);
      $rg_config->{destination_disk} = $copy{'dest'};
      $rg_config->{source_disk} = $copy{'source'};
      my $source_id = platform_shim_disk_get_property($rg_config->{source_disk}, 'id');
      $self->platform_shim_test_case_log_step('Source: ' . $source_id);

      my $dest_id = platform_shim_disk_get_property($rg_config->{destination_disk}, 'id');
      $self->platform_shim_test_case_log_step('Dest: ' . $dest_id);
      $rg_config->{destination_disk} = $copy{'dest'};
      $rg_config->{dest_id} = $dest_id;
      my $copy_disk = platform_shim_disk_user_copy_to($copy{'source'}, $copy{'dest'});
      $self->platform_shim_test_case_log_step('Copy started on RG ' . $rg_config->{raid_group_id} .
                                              ' of type ' .  $rg_config->{raid_type}.
                                              ' position ' . $copy_position .
                                              ' disk ' . $source_id .
                                              ' to disk ' . $dest_id);
      my $state = platform_shim_disk_get_property($copy{'dest'}, "state"); 
      while ($state eq "Unbound") {
         $self->platform_shim_test_case_log_step('Waiting for copy to start on dest disk ' . $dest_id. " current state " . $state);
         sleep(5);
         $state = platform_shim_disk_get_property($copy{'dest'}, "state"); 
      }
      $self->platform_shim_test_case_log_step('Copy started on dest disk ' . $dest_id . " state " . $state);
   } 
}
sub user_copy_configure_params_based_on_platform
{
    my ($self, $rgs_ref, $luns_per_rg_ref, $per_disk_capacity_gb_ref, $lun_io_capacity_percent_ref, $lun_io_max_lba_ref, $lun_io_threads_ref) = @_;
     
    my @rgs = @{$rgs_ref};
    my $device = $self->{device};
    my $platform_type = 'HW';
    my $test_level = 'Promotion';
    if (platform_shim_test_case_is_functional($self)) {
        $test_level = 'Functional';
    }

    # If `clean_config' is not set then use the exist luns
    # This assumes that the number of luns in each raid group is the same
    if (!$self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        my @luns = platform_shim_raid_group_get_lun($rgs[0]);
        $$luns_per_rg_ref = scalar(@luns);
        my $lun_capacity = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
        my $data_disks = test_util_configuration_get_data_disks($self, $rgs[0]);
        $lun_capacity = $lun_capacity / $data_disks;
        $$per_disk_capacity_gb_ref = int($lun_capacity / 2097152);
    } elsif (platform_shim_device_is_simulator($device)) {
        # Simulator with maximum drive capacity of 10GB
        $$luns_per_rg_ref = 2;
        $$per_disk_capacity_gb_ref = 2;
    } else {
       if ($test_level eq 'Functional') {
          # Hardware with minimum drive capacity of 100GB
          $$luns_per_rg_ref = 2;
          $$per_disk_capacity_gb_ref = 4;
       } else {
          # Hardware with minimum drive capacity of 100GB
          $$luns_per_rg_ref = 2;
          $$per_disk_capacity_gb_ref = 4;
       }
    }

    # Now set the I/O parameters
    if (platform_shim_device_is_simulator($device)) {
        # Simulator with maximum drive capacity of 10GB
        $platform_type = 'SIM';
        $$lun_io_capacity_percent_ref = 5;
        $$lun_io_threads_ref = 2;
    } else {
        # Hardware with minimum drive capacity of 100GB
        if ($test_level eq 'Functional') {
            $$lun_io_capacity_percent_ref = 99;
            $$lun_io_threads_ref = 32;
        } else {
            $$lun_io_capacity_percent_ref = 7;
            $$lun_io_threads_ref = 32;
        }
    }
    $$lun_io_max_lba_ref = int(($$per_disk_capacity_gb_ref * 2097152) / (100 / $$lun_io_capacity_percent_ref));
    my $lun_io_max_lba_hex = sprintf "0x%lx", $$lun_io_max_lba_ref;
    $self->platform_shim_test_case_log_step("Configure params based on platform: $platform_type test level: $test_level");
    $self->platform_shim_test_case_log_info("Params - lun per rg: $$luns_per_rg_ref per disk capacity gb: $$per_disk_capacity_gb_ref lun io capacity percent: $$lun_io_capacity_percent_ref");
    $self->platform_shim_test_case_log_info("         lun io max lba: $lun_io_max_lba_hex io threads per lun: $$lun_io_threads_ref");
    return;
}

sub user_copy_start_read_only_io
{
    my ($self, $rg_configs_ref, $luns_per_rg, $max_lun_lba, $lun_io_threads) = @_;
     
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $device = $self->{device};
    my $element_size = 0x80;
    my $status = 0;

    # Randomly select the width of one of the raid groups and generate maximum io size
    my $rg_index = int(rand($rg_count));
    my $width = $rg_configs[$rg_index]->{width};
    my $max_io_size = $element_size * $width;

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
    # ? affinity_spread => 1,
    platform_shim_device_rdgen_reset_stats($device);
    # @note Be careful of the thread_count since even (2) threads pe lun can slow the system!
    my %rdgen_params = (read_only   => 1, 
                        thread_count => $lun_io_threads,
                        sequential_increasing_lba => 1,
                        block_count => $max_io_size,
                        minimum_blocks   => 1,
                        start_lba => 0,
                        maximum_lba => $max_lun_lba,
                        );
    #print "Max lba " . $max_lun_lba . " block count " . $max_io_size;
    my $expected_number_of_ios = ($rg_count * $luns_per_rg) * 2;
    platform_shim_device_rdgen_start_io($device, \%rdgen_params);
    test_util_io_verify_rdgen_io_progress($self, $expected_number_of_ios, 10);

    # Return the status
    return $status;
}
sub user_copy_start_random_read_only_io
{
    my ($self, $rg_configs_ref, $luns_per_rg, $lun_io_threads) = @_;
     
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $device = $self->{device};
    my $element_size = 0x80;
    my $status = 0;

    # Randomly select the width of one of the raid groups and generate maximum io size
    my $rg_index = int(rand($rg_count));
    my $width = $rg_configs[$rg_index]->{width};
    my $max_io_size = $element_size * $width;

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
    # ? affinity_spread => 1,
    platform_shim_device_rdgen_reset_stats($device);
    # @note Be careful of the thread_count since even (2) threads pe lun can slow the system!
    my %rdgen_params = (read_only   => 1, 
                        thread_count => $lun_io_threads,
                        block_count => $max_io_size,
                        minimum_blocks   => 1,
                        start_lba => 0,
                        use_block => 1,
                        );
    #print "Max lba " . $max_lun_lba . " block count " . $max_io_size;
    my $expected_number_of_ios = ($rg_count * $luns_per_rg) * 2;
    platform_shim_device_rdgen_start_io($device, \%rdgen_params);
    test_util_io_verify_rdgen_io_progress($self, $expected_number_of_ios, 10);

    # Return the status
    return $status;
}

sub user_copy_display_progress
{
    my ($self, $rg_configs_ref) = @_;

    $self->platform_shim_test_case_log_step(" ");
    my @rg_configs = @{$rg_configs_ref};

    $self->platform_shim_test_case_log_info("Verify copy status");
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $destination_disk = $rg_config->{destination_disk};
        my $destination_id = $rg_config->{dest_id};
        my $copy_percent;
        # Wait for current checkpoint to increase
        my $copy_checkpoint = test_util_copy_display_progress($self, $destination_disk, $destination_id, \$copy_percent);

        $rg_config->{percent_copied} = $copy_percent;
        $rg_config->{copy_checkpoint} = $copy_checkpoint;
    }
}
sub user_copy_check_start
{
    my ($self, $rg_configs_ref) = @_;
    my $timeout = 5 * 60; # 5 min

    $self->platform_shim_test_case_log_step(" ");
    my @rg_configs = @{$rg_configs_ref};

    $self->platform_shim_test_case_log_step("Verify copy starts on each raid group");
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $copy_position = $rg_config->{position_to_copy};
        my $source_disk = $rg_config->{source_disk};
        my $destination_disk = $rg_config->{destination_disk};
        my $source_id = platform_shim_disk_get_property($source_disk, 'id');
        my $destination_id = platform_shim_disk_get_property($destination_disk, 'id');
        my $b_wait_for_percent_increase = 0;
        my $copy_percent = $rg_config->{percent_copied};

        # Wait for current checkpoint to increase
        my $copy_checkpoint = test_util_copy_check_copy_progress($self, $destination_disk, $destination_id, \$copy_percent, $b_wait_for_percent_increase, $timeout);
        $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $destination_disk, $destination_id, \$copy_percent);
        $rg_config->{copy_checkpoint} = $copy_checkpoint;
        $rg_config->{percent_copied} = $copy_percent;
        $self->platform_shim_test_case_log_info('Check CopyTo start RG ' . $rg_config->{raid_group_id} .
                                                ' position ' . $copy_position .
                                                ' source ' . $source_id .
                                                ' destination ' . $destination_id .
                                                ' checkpoint ' . $copy_checkpoint .
                                                ' percent_copied ' . $copy_percent);
    }
    $self->platform_shim_test_case_log_step("Check Start Success.");
}
sub user_copy_unbind_first_lun
{
    my ($self, $rg_configs_ref) = @_;
    my $timeout = 5 * 60; # 5 min
    my @rg_configs = @{$rg_configs_ref};
    # Pick a random position
    my $rg_count = @rg_configs;
    my $rg_index = rand($rg_count);

    $self->platform_shim_test_case_log_step("Unbind copy LUN in each RG.");
    my $rg_config = $rg_configs[$rg_index];

    my $rg = $rg_config->{raid_group};
    $self->platform_shim_test_case_log_step("Unbind the first LUN of RG ". $rg_config->{raid_group_id});
    # Now unbind the first LUN
    my $luns_ref = $rg_config->{luns};
    my @luns = @$luns_ref;
    #my @luns = $rg_config->{luns};
    my $num_luns = @luns;
    
    platform_shim_lun_unbind($luns[0]);
    # my @new_luns = splice($rg_config->{luns}, 0, 1);
    my $unbind_lun = shift @luns;
    $rg_config->{luns} = \@luns;
    my @current_rg_config = ();
    push @current_rg_config, $rg_config;
    user_copy_display_progress($self, \@rg_configs);
   }
sub user_copy_check_progress
{
    my ($self, $rg_configs_ref) = @_;
    my @rg_configs = @{$rg_configs_ref};
    my $timeout = 5 * 60; # 5 min

    $self->platform_shim_test_case_log_step("Verify copy makes progress on each raid group");
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $copy_position = $rg_config->{position_to_copy};
        my $source_disk = $rg_config->{source_disk};
        my $destination_disk = $rg_config->{destination_disk};
        my $source_id = platform_shim_disk_get_property($source_disk, 'id');
        my $destination_id = platform_shim_disk_get_property($destination_disk, 'id');
        my $b_wait_for_percent_increase = 0;
        my $copy_percent = 0;

        # Wait for current checkpoint to increase
        my $copy_checkpoint = test_util_copy_check_copy_progress($self, $destination_disk, $destination_id, \$copy_percent, $b_wait_for_percent_increase, $timeout);
        $rg_config->{percent_copied} = $copy_percent;
        $rg_config->{copy_checkpoint} = $copy_checkpoint;
        $self->platform_shim_test_case_log_info('CopyTo progress RG ' . $rg_config->{raid_group_id} .
                                                ' position ' . $copy_position .
                                                ' source ' . $source_id .
                                                ' destination ' . $destination_id .
                                                ' checkpoint ' . $copy_checkpoint .
                                                ' percent_copied ' . $copy_percent);
    }
    $self->platform_shim_test_case_log_step("Check Progress Success.");
}
sub user_copy_bind_above_checkpoint
{
    my ($self, $rg_configs_ref) = @_;
  
    $self->platform_shim_test_case_log_step("Bind a lun above the copy checkpoint - copy progress");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 1;
    my $rg_index = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg = $rg_config->{raid_group};
        my $rg_id = $rg_config->{raid_group_id};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $dest_disk = $rg_config->{destination_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_id = $rg_config->{dest_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        my $b_wait_for_percent_increase = 0; # Don't wait for percent increase
        my $luns_ref = $rg_config->{luns};
        my @luns = @$luns_ref;
        my @current_rg_config = ();
        push @current_rg_config, $rg_config;

        # Find a virtual drive who is well below the maximum consumed space
        $self->platform_shim_test_case_log_info("Attempt to bind lun on rg id: $rg_id vd id: $vd_fbe_id percent copied: $percent_copied");
        if ($percent_copied < 50) {
            # Bind a lun with the same capacity as first
            my $lun_capacity = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
            my $lun_capacity_gb = int($lun_capacity / 2097152);
            my $lun = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
            # If the bind failed try another raid group
            if (!defined $lun) {
                $self->platform_shim_test_case_log_info("Bind on rg id: $rg_id for $lun_capacity_gb GB failed. Try another");
                next;
            }
            # Bind succeeded log a message and unbind
            my $lun_id = platform_shim_lun_get_property($lun, 'id');
            $self->platform_shim_test_case_log_info("Bind on rg id: $rg_id vd id: $vd_fbe_id lun id: $lun_id" .
                                                    " percent copied: $percent_copied success");

            user_copy_display_progress($self, \@rg_configs);
            #user_copy_check_progress($self, \@current_rg_config);
            platform_shim_lun_unbind($lun);
            $self->platform_shim_test_case_log_info("Unbound on rg id: $rg_id vd id: $vd_fbe_id lun id: $lun_id" .
                                                    " percent copied: $percent_copied success");
            user_copy_display_progress($self, \@rg_configs);
            #user_copy_check_progress($self, \@current_rg_config);
            $status = 0;
            # Only do this for (1) raid group
            last;
        }

        # Goto next raid group
        $rg_index++;
    }

    # If we could not locate a vd with less than 50% copied log an error
    if ($status != 0) {
        $self->platform_shim_test_case_log_error("Failed to locate a vd that is less than 50 percent copied");
    }

    return $status;
}

sub user_copy_validate_second_copy_error
{
    my ($self, $rg_configs_ref) = @_;

    $self->platform_shim_test_case_log_step(" ");
    my @rg_configs = @{$rg_configs_ref};
    my $rg_index = 0;
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
       my $rg = $rg_config->{raid_group};
        my $copy_position = $rg_config->{position_to_copy};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = platform_shim_disk_get_property($source_disk, 'id');

        my %copy = test_util_find_copy_disk_for_rg($self, $rg);
        my $destination_disk = $copy{'dest'};
        my $destination_id = platform_shim_disk_get_property($destination_disk, 'id');

        $self->platform_shim_test_case_log_info('Start copy again on already copying drive and expect errors.');
        $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' position ' . $copy_position .
                                                ' disk ' . $source_id .
                                                ' to disk ' . $destination_id); 
        my $copy_disk = platform_shim_disk_user_copy_to($source_disk, $destination_disk);
        test_util_start_copy_with_error($self, $source_disk, $destination_disk);
    }
}
sub user_copy_validate_start_copy_error
{
    my ($self, $rg_configs_ref) = @_;

    $self->platform_shim_test_case_log_step(" ");
    my @rg_configs = @{$rg_configs_ref};
    my $status = 0;
    my $rg_count = @rg_configs;
    my $rg_index = int(rand($rg_count));
    $self->platform_shim_test_case_log_info('Testing RG Index ' . $rg_index . " For copy errors");
    my $rg_config = $rg_configs[$rg_index];

       my $rg = $rg_config->{raid_group};
        my $copy_position = $rg_config->{position_to_copy};
        # a is current source where a -> b copy is in progress
        my $a_disk = $rg_config->{source_disk};
        my $a_source_id = platform_shim_disk_get_property($a_disk, 'id');

        my $b_disk = $rg_config->{destination_disk};
        my $b_dest_id = platform_shim_disk_get_property($b_disk, 'id');

        my %copy = test_util_find_copy_disk_for_rg($self, $rg);
        my $c_spare_disk = $copy{'dest'};
        my $c_spare_id = platform_shim_disk_get_property($c_spare_disk, 'id');

        $self->platform_shim_test_case_log_info('With copy  A -> B in progress try to start copy A -> C and expect error.');
        $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' position ' . $copy_position .
                                                ' disk ' . $a_source_id .
                                                ' to disk ' . $c_spare_id); 
        test_util_start_copy_with_error($self, $a_disk, $c_spare_disk);

        $self->platform_shim_test_case_log_info('With copy  A -> B in progress try to start copy B -> C and expect error.');
        $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' disk ' . $b_dest_id .
                                                ' to disk ' . $c_spare_id); 
        test_util_start_copy_with_error($self, $b_disk, $c_spare_disk);

        $self->platform_shim_test_case_log_info('With copy A -> B in progress try to start copy C -> B.');
        $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' disk ' . $c_spare_id .
                                                ' to disk ' . $b_dest_id); 
        test_util_start_copy_with_error($self, $c_spare_disk, $b_disk);


        $self->platform_shim_test_case_log_info('With copy A -> B in progress try to start copy C -> A.');
        $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' disk ' . $c_spare_id .
                                                ' to disk ' . $a_source_id); 
        test_util_start_copy_with_error($self, $c_spare_disk, $a_disk);
}

sub user_copy_destination_bound_drive_error
{
    my ($self, $rg_configs_ref) = @_;

    my @rg_configs = @{$rg_configs_ref};
    my $rg_index = 0;
    my $status = 0;
    my $rg_count = @rg_configs;
    if ($rg_count < 2) {
       $self->platform_shim_test_case_log_info('This test case not running since only one raid group and we need 2.');
       return;
    }
    $self->platform_shim_test_case_log_info('Validate copy fails when source and destination are bound RAID Groups.');
    my $source_rg = undef;
    my $dest_rg = undef;
    my $source_raid_type;
    my $source_rg_id; 
    foreach my $rg_config (@rg_configs) {
       my $rg = $rg_config->{raid_group};
       $source_raid_type = $rg_config->{raid_type};
       $source_rg_id = $rg_config->{raid_group_id};
       if ($source_raid_type ne "r0") {
          $source_rg = $rg;
          last;
       }
    }

    my $dest_raid_type;
    my $dest_rg_id; 
    foreach my $rg_config (@rg_configs) {
       my $rg = $rg_config->{raid_group};
       $dest_raid_type = $rg_config->{raid_type};
       $dest_rg_id = $rg_config->{raid_group_id};
       my $rg_type = $rg_config->{raid_type};
       if (($rg != $source_rg) and $rg_type ne "r0") {
          $dest_rg = $rg;
          last;
       }
    }
    
    my $copy_position = 0;
    my @source_disks = platform_shim_raid_group_get_disk($source_rg);
    my $source_disk = $source_disks[$copy_position];
    my $source_id = platform_shim_disk_get_property($source_disk, 'id');
    my %copy = test_util_find_copy_disk_for_rg($self, $source_rg);
    my $dest_disk = $copy{'dest'};
    my $dest_id = platform_shim_disk_get_property($dest_disk, 'id');

    $self->platform_shim_test_case_log_info('Start copy between two disks of two different bound raid groups.');
    $self->platform_shim_test_case_log_info('Starting copy on RG ' . $source_rg_id .
                                            ' of type ' . $source_raid_type .
                                            ' position ' . $copy_position .
                                            ' disk ' . $source_id .
                                            ' to disk ' . $dest_id); 
     $self->platform_shim_test_case_log_info('destination is part of on RG ' . $dest_rg_id .
                                            ' of type ' . $dest_raid_type .
                                            ' position ' . $copy_position .
                                            ' disk ' . $dest_id);
    test_util_start_copy_with_error($self, $source_disk, $dest_disk);
}

sub user_copy_for_unbound_system_drive
{
   my ($self) = @_;

   # Unbound system drive copy should be an error.  We should not copy system RGs.

   # First get one drive that is a system drive.  
   my %rg_params = (state      => "unused",
                     vault      => 1,);

   my @disks    = platform_shim_device_get_disk($self->{device}, \%rg_params);
  
   my $source_disk = $disks[0];
   my $source_id = platform_shim_disk_get_property($source_disk, 'id');
   my $dest_disk = test_util_find_copy_disk($self, $source_disk);
   if (!defined($dest_disk)) {
      $self->platform_shim_test_case_log_info('No suitable spare found for system drive. Skip unbound system drive copy test.');
      return;
   }
   my $dest_id = platform_shim_disk_get_property($dest_disk, 'id');

   $self->platform_shim_test_case_log_info('Start copy between unbound system drive . ' . $source_id . 
                                           ' and unbound drive ' . $dest_id . ' and expect error ');
   test_util_start_copy_with_error($self, $source_disk, $dest_disk);
}

sub user_copy_stop_io
{
    my ($self, $rg_configs_ref, $max_lun_lba, $min_io_size) = @_;
     
    $self->platform_shim_test_case_log_step("Stop rdgen I/O on all LUNs");
    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $status = 0;
    platform_shim_device_rdgen_stop_io($device, {});

    # Return the status
    return $status;
}

#=
#*           user_copy_wait_for_copy_complete
#*
#* @brief    Wait for copy to complete on all the raid groups
#*
#* @param    $self - Main argument pointer
#* @param    $step - The current step of the test case 
#* @param    $rg_configs_ref  - Address of array of raid group configurations under test
#*
#* @return   $status - The status of this operation
#*
#=cut
sub user_copy_wait_for_copy_complete
{
    my ($self, $step, $rg_configs_ref, $copy_complete_timeout_secs) = @_;
     
    $self->platform_shim_test_case_log_step("Wait for copy completion");
    my @rg_configs = @{$rg_configs_ref};
    my $rg_index = 0;
    my $status = 0;
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $copy_position = $rg_config->{position_to_copy};
        my $vd_fbe_id = $rg_config->{vd_fbe_id_hex};
        my $source_disk = $rg_config->{source_disk};
        my $source_id = $rg_config->{source_id};
        my $dest_disk = $rg_config->{destination_disk};
        my $dest_id = $rg_config->{dest_id};
        my $percent_copied = $rg_config->{percent_copied};
        my $copy_checkpoint = $rg_config->{copy_checkpoint};
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();

        # If not complete report the checkpoint
        if (!test_util_is_copy_complete($copy_checkpoint)) {
           my $wait_seconds = 0;
            $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);

            while ($percent_copied != 100) {
               $copy_checkpoint_hex = $copy_checkpoint->as_hex();
               $rg_config->{percent_copied} = $percent_copied;
               $rg_config->{copy_checkpoint} = $copy_checkpoint;
               $self->platform_shim_test_case_log_info("Proactive Copy complete percent copied: $percent_copied" .
                                                       " copy checkpoint $copy_checkpoint_hex wait for copy complete seconds: $wait_seconds");

               $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($self, $dest_disk, $dest_id, \$percent_copied);
               sleep(30);
               $wait_seconds += 30;
            }
            if ($percent_copied == 100) {
               $self->platform_shim_test_case_log_info("Proactive Copy complete");
            }
        }

        $self->platform_shim_test_case_log_info("Wait for User Copy completion event on vd: $vd_fbe_id rg id: $rg_id" .
                                                " position: $copy_position source: $source_id destination: $dest_id");
        platform_shim_event_copy_complete({object => $self->{device}, disk => $dest_disk, timeout => $copy_complete_timeout_secs . ' S'});
        $self->platform_shim_test_case_log_info("User Copy complete event found on vd: $vd_fbe_id rg id: $rg_id" .
                                                " position: $copy_position source: $source_id destination: $dest_id");

        # Goto next raid group
        $rg_index++;
    } 

    # Return the status
    return $status;
}
# end user_copy_wait_for_copy_complete

sub user_copy_fault_destination
{
    my ($self, $rg_configs_ref) = @_;

    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $rg_config;
    my $current_rg_config;
    foreach $current_rg_config (@rg_configs) {
       if ($current_rg_config->{raid_type} ne "r0") {
          $rg_config = $current_rg_config;
          last;
       }
    }
    my $rg = $rg_config->{raid_group};

    $self->platform_shim_test_case_log_step("Fault drive that was copied to. Raid Group: " . $rg_config->{raid_group_id});

    my $disk = $rg_config->{destination_disk};
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my @faults = ();
    $self->platform_shim_test_case_log_info("Remove disk " . $disk_id);
    push @faults, platform_shim_disk_remove($disk); 
    $rg_config->{disk_faults} = \@faults;
    platform_shim_disk_wait_for_disk_removed($device, [$disk]);

    $self->platform_shim_test_case_log_step('Verify that rebuild logging starts for raid group ' . $rg_config->{raid_group_id});                    
    platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                       raid_group => $rg_config->{raid_group}});

    $self->platform_shim_test_case_log_info("Verify that rebuild logging has stopped for raid group ".$rg_config->{raid_group_id});
    platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                       raid_group => $rg_config->{raid_group}});

    $self->logStep("Verify that rebuild started on permanent spare ");
    platform_shim_event_verify_rebuild_started({object => $device, 
                                               raid_group => $rg_config->{raid_group}});

    $self->platform_shim_test_case_log_step("Recover disk faults");
    platform_shim_device_recover_from_faults($device);

    $self->logStep("Wait for rebuild to complete ");
    test_util_rebuild_wait_for_rebuild_complete($self, $rg_config->{raid_group}, 3600 * 10);
    platform_shim_raid_group_mark_dirty($rg);

    $self->logStep("Rebuild is complete.  Try to copy back to original source.");
    
    my @disks = platform_shim_raid_group_get_disk($rg_config->{raid_group});
    my $source_disk = $disks[$rg_config->{position_to_copy}];
    my $source_id = platform_shim_disk_get_property($source_disk, 'id');
    $rg_config->{destination_disk} = $disk;
    $self->platform_shim_test_case_log_info('Start Copy on RG ' . $rg_config->{raid_group_id} .
                                              ' of type ' .  $rg_config->{raid_type}.
                                              ' disk ' . $source_id .
                                              ' to disk ' . $disk_id); 
    my $copy_disk = platform_shim_disk_user_copy_to($source_disk, $disk);
    user_copy_check_start($self, \@rg_configs);
}

sub user_copy_degraded_raid_group
{
    my ($self, $rg_configs_ref) = @_;

    my @rg_configs = @{$rg_configs_ref};
    my $device = $self->{device};
    my $rg_config;
    my $current_rg_config;
    foreach $current_rg_config (@rg_configs) {
       if ($current_rg_config->{raid_type} ne "r0") {
          $rg_config = $current_rg_config;
          last;
       }
    }
    $self->platform_shim_test_case_log_info("Try to start copy on degraded raid group rg " . $rg_config->{raid_group_id});
    # Need to refresh the complement of disks, since they likely changed as the result of
    # a completed copy or a completed permanent spare.

    my $rg = $rg_config->{raid_group};
    my @disks = platform_shim_raid_group_get_disk($rg);

    my $disk = $disks[0];
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my @faults = ();
    my %copy = test_util_find_copy_disk_for_rg($self, $rg);
    $self->platform_shim_test_case_log_info("Remove disk " . $disk_id);
    push @faults, platform_shim_disk_remove($disk); 
    $rg_config->{disk_faults} = \@faults;
    platform_shim_disk_wait_for_disk_removed($device, [$disk]);

    $self->platform_shim_test_case_log_step('Verify that rebuild logging starts for raid group ' . $rg_config->{raid_group_id});                    
    platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                       raid_group => $rg_config->{raid_group}});

    my $copy_position = $rg_config->{position_to_copy};
    my $source_disk = $rg_config->{source_disk};
    my $source_id = platform_shim_disk_get_property($source_disk, 'id');
    my $dest_disk = $copy{'dest'};
    my $dest_id = platform_shim_disk_get_property($dest_disk, 'id');

    $self->platform_shim_test_case_log_info('Start copy on degraded raid group and expect error.');
    $self->platform_shim_test_case_log_info('Starting copy on RG ' . $rg_config->{raid_group_id} .
                                                ' of type ' . $rg_config->{raid_type} .
                                                ' position ' . $copy_position .
                                                ' disk ' . $source_id .
                                                ' to disk ' . $dest_id); 
    test_util_start_copy_with_error($self, $source_disk, $dest_disk);

    $self->platform_shim_test_case_log_step("Recover disk faults");
    platform_shim_device_recover_from_faults($device);
    
    $self->logStep("Wait for rebuild to complete.");
    test_util_rebuild_wait_for_rebuild_complete($self, $rg_config->{raid_group}, 3600);

    $self->logStep("Rebuild is complete.");
}

sub user_copy_read_background_pattern {
   my ($self, $rg_configs_ref, $luns_per_rg, $lun_capacity_per_disk_gb, $lun_io_capacity_percent, $lun_io_threads) = @_;
     
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $device = $self->{device};

    $self->platform_shim_test_case_log_step("Read check background pattern on all raid groups");
    
    platform_shim_device_rdgen_reset_stats($device);
	 
    my $expected_io_count = 0;
    foreach my $rg_config (@rg_configs) {
    	
      my $rg = $rg_config->{raid_group};
      my $width = $rg_config->{width};
      my $luns_ref = $rg_config->{luns};
      my @luns = @$luns_ref;
      
      my $data_disks = $rg_config->{data_disks};
      my $lun_io_max_lba = int(($lun_capacity_per_disk_gb * 2097152 * $data_disks)  / (100 / $lun_io_capacity_percent));
      my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
      my $blocks = $data_disks * 0x800;
      my $num_luns = @luns;
      $expected_io_count += int(($lun_io_max_lba / $blocks)) * $num_luns;
      $self->platform_shim_test_case_log_step("Start Check pattern for RG: " . $rg_config->{raid_group_id} .
                                              " data disks " . $data_disks .
                                              " blocks " . $blocks .
                                              " per disk GB " . $lun_capacity_per_disk_gb . 
                                              " capacity pct " . $lun_io_capacity_percent .
                                              " Max Lba: " . $lun_io_max_lba_hex);
      test_util_io_start_check_pattern($self, {maximum_lba => $lun_io_max_lba, 
                                 luns => \@luns, 
                                 block_count => $blocks});
   }   
    $self->platform_shim_test_case_log_step("Wait for rdgen check pattern to complete ios: " . $expected_io_count);
  test_util_io_wait_rdgen_thread_complete($self, 3600);
   test_util_io_verify_rdgen_io_complete($self, $expected_io_count/2);
   test_util_io_display_rdgen_stats($self); 

}

sub user_copy_write_background_pattern {
   my ($self, $rg_configs_ref, $luns_per_rg, $lun_capacity_per_disk_gb, $lun_io_capacity_percent, $lun_io_threads) = @_;
     
    my @rg_configs = @{$rg_configs_ref};
    my $rg_count = scalar(@rg_configs);
    my $device = $self->{device};

    $self->platform_shim_test_case_log_step("Write pattern on all raid groups");
    
    platform_shim_device_rdgen_reset_stats($device);
    my $expected_io_count = 0;
    foreach my $rg_config (@rg_configs) {
      my $rg = $rg_config->{raid_group};
      my $width = $rg_config->{width};
      my $luns_ref = $rg_config->{luns};
      my @luns = @$luns_ref;

      my $data_disks = $rg_config->{data_disks};
      my $lun_io_max_lba = int(($lun_capacity_per_disk_gb * 2097152 * $data_disks)  / (100 / $lun_io_capacity_percent));
      my $lun_io_max_lba_hex = sprintf "0x%lx", $lun_io_max_lba;
      my $blocks = $data_disks * 0x800;
      my $num_luns = @luns;
      $expected_io_count += int($lun_io_max_lba / $blocks) * $num_luns;

      $self->platform_shim_test_case_log_step("Start I/O for RG: " . $rg_config->{raid_group_id} .
                                              " data disks " . $data_disks .
                                              " blocks " . $blocks .
                                              " per disk GB " . $lun_capacity_per_disk_gb . 
                                              " capacity pct " . $lun_io_capacity_percent .
                                              " Max Lba: " . $lun_io_max_lba_hex);

      test_util_io_start_write_background_pattern($self, {maximum_lba => $lun_io_max_lba, 
                                                  luns => \@luns, 
                                                  block_count => $blocks});   
    }

    $self->platform_shim_test_case_log_step("Wait for rdgen set pattern to complete. ios: " . $expected_io_count);
    test_util_io_wait_rdgen_thread_complete($self, 3600);
    test_util_io_verify_rdgen_io_complete($self, $expected_io_count/ 2);
    test_util_io_display_rdgen_stats($self);

}
1;
