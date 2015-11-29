package mcr_test::write_hole::write_hole;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::test_util::test_util_event;
use mcr_test::test_util::test_util_io;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use strict;
use warnings;

# ==================Write Hole Test Cases===========================================
# * Create RAID groups of type R5, R3, R6
# * Create LUNs.
# * Degrade RAID Groups by pulling 1 (R5), 1 (R3), and 2 drives for the R6.
# * Wait for rebuild logging to start.
# * Wait for LUNs to show as faulted.
# * Reinsert drives.
# * Wait for rebuild logging to stop.
# * Wait for rebuild to start.
# * Wait for rebuild to complete.
# * Validate we did not get any serious errors such as: 
#    uncorrectable, sector reconstructed, coherency, checksum, 
# 
# For Functional level test we should use larger LUNs and more than 3 raid groups.
# 
# ======================================================================================
sub main
{
   test_util_general_test_start();
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    my $device = $self->{device};
    my $luns_per_rg     = 2;
    my $lun_capacity_gb = 1;
    
    $lun_capacity_gb = 5 if ($self->platform_shim_test_case_is_functional());
    $self->platform_shim_test_case_add_to_cleanup_stack({'write_hole_cleanup' => {}});     

    # We do NOT disable load balancing since we want some parent raid groups NOT
    # on the CMI Active SP
    platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    platform_shim_device_disable_automatic_sparing($device);
    
    # Create the raid groups, different widths and drive types, parity
    my %rg_params = (id => '0');
    my @rgs = platform_shim_device_get_raid_group($self->{device});
    my $total_rgs = @rgs;
    $self->platform_shim_test_case_log_info("found $total_rgs raid groups");

    # Step 1: Set up test cases for each raid group
    $self->platform_shim_test_case_log_step("Create raid groups");
    my @rg_configs = @{write_hole_initialize_rg_configs($self, $luns_per_rg, $lun_capacity_gb)};

    # Step 2: Start io on luns
    platform_shim_device_initialize_rdgen($device);
    write_hole_start_io($self, \@rg_configs);
    
    write_hole_degrade_raid_groups($self, \@rg_configs);
      
    write_hole_break_raid_groups($self, \@rg_configs);

    write_hole_restore_raid_groups($self, \@rg_configs);

    write_hole_wait_for_rebuilds($self, \@rg_configs);
    
    $self->platform_shim_test_case_log_info("Stopping rdgen threads");
    platform_shim_device_rdgen_stop_io($self->{device}, {});
    $self->platform_shim_test_case_log_step("Checking background pattern on luns");
    test_util_io_check_background_pattern($self);

    test_util_event_check_for_unexpected_errors($self);

    test_util_general_log_test_time($self);
}

sub write_hole_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_gb) = @_;
   my @rgs;
   if ($self->platform_shim_test_case_get_parameter(name => 'clean_config'))
   {
       
       if ($self->platform_shim_test_case_is_functional())
       {
           @rgs = @{write_hole_create_all_parity_types_reserve($self)};
       }
       else
       {
           @rgs = @{test_util_configuration_create_all_parity_types_reserve($self)};
           #my $rg = test_util_configuration_create_random_raid_type_config($self, {raid_type => 'r1'});
           #push @rgs, $rg;
       }
       
   } 
   else 
   {
       @rgs = platform_shim_device_get_raid_group($self->{device}, {});
   }
   $self->platform_shim_test_case_log_step("Step: Initialize raid group configs from array of raid groups");
   my @rg_configs = ();
   my $rg_index = 0;
   my $tested_empty_rg_copy = 0;

   foreach my $rg (@rgs) {
      my $rg_id = platform_shim_raid_group_get_property($rg, 'id'); 
      my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
      my $width = platform_shim_raid_group_get_property($rg, 'raid_group_width'); 
      my @disks = platform_shim_raid_group_get_disk($rg);
      
      my $num_disks_to_remove = 0;
        
      # Setup the test case   
      my %rg_config = ();
      
      $rg_config{raid_group_id} = $rg_id;
      $rg_config{raid_group} = $rg;
      $rg_config{lun_per_rg} = $luns_per_rg;
      #$rg_config{raid_group_fbe_id} = platform_shim_raid_group_get_property($rg, 'fbe_id');
      $rg_config{width} = $width;
      $rg_config{raid_type} = $raid_type;
      my @disks_to_fault = (shift(@disks)); 

      if ($rg_config{raid_type} eq 'r6') {
         push @disks_to_fault , shift(@disks);
      }
      $rg_config{disks_to_fault} = \@disks_to_fault;

      my @disks_to_shutdown = (shift(@disks)); 
      $rg_config{disks_to_shutdown} = \@disks_to_shutdown;

      my @luns = ();
      if ($self->platform_shim_test_case_get_parameter(name => 'clean_config'))
      {
          # Bind `luns_per_rg' plus 1
          foreach (1..($luns_per_rg)) {
             my $lun = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
             push @luns, $lun;
          }
      }
      else 
      {
          @luns = platform_shim_raid_group_get_lun($rg);
      }

      $rg_config{luns} = \@luns;

      # Push the test case
      push @rg_configs, \%rg_config;
   }
   # Return the address of the raid groups test configs created
   return \@rg_configs;
}

sub write_hole_start_io
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};
    my $device = $self->{device};
    
    
    $self->platform_shim_test_case_log_step("start rdgen to all luns");
    my $DEFAULT_START_LBA = 0xF001;
    my $TEST_ELEMENT_SIZE = 0x80;
                
    platform_shim_device_initialize_rdgen($device);
    test_util_io_write_background_pattern($self);
    
    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        block_count => 0x80, 
                        #object_id    => $FBE_OBJECT_ID_INVALID,
                        #class_id     => $FBE_CLASS_ID_LUN,
                        use_block => 1,
                        start_lba => $DEFAULT_START_LBA,
                        minimum_lba => $DEFAULT_START_LBA,
                        maximum_lba => $DEFAULT_START_LBA * 2,
                        minimum_blocks => $TEST_ELEMENT_SIZE,
                        );
                        
    my @luns = @{$rg_configs[0]->{luns}};
    $rdgen_params{maximum_lba} = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
    foreach my $lun (@luns)
    {
        # when sending through cache we need to supply the lun id
        $rdgen_params{clar_lun} = 1;
        platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
    }
    
    
    #platform_shim_device_rdgen_start_io($device, \%rdgen_params);
}

sub write_hole_degrade_raid_groups
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};
    
    my $device = $self->{device};
    # Step 2: Fault a disk in the raid group and verify rl
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my @faults = ();
        my $disk_id;
        foreach my $disk (@{$rg_config->{disks_to_fault}}) {
            $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info("Remove disk ". $disk_id." in RAID group ".$rg_id);
            push @faults, platform_shim_disk_remove($disk);
        }
        $rg_config->{disk_faults} = \@faults;
    }
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        foreach my $disk (@{$rg_config->{disks_to_fault}}) {
            $self->platform_shim_test_case_log_info('Waiting for removed  for RG ' . $rg_id);
            my @temp = ($disk);
            platform_shim_disk_wait_for_disk_removed($device, \@temp);
        }
        
    }
    foreach my $rg_config (@rg_configs) {

	# Step: Verify rebuild logging starts for redundant raid types
	$self->platform_shim_test_case_log_step('Verify that rebuild logging starts for raid group '.$rg_config->{raid_group_id});
	platform_shim_event_verify_rebuild_logging_started({object => $device,
							    raid_group => $rg_config->{raid_group}}); 
      }
    
    $rg_config_ref = \@rg_configs;
}

sub write_hole_break_raid_groups
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    $self->platform_shim_test_case_log_info("Remove disks to break raid groups.");
    my $device = $self->{device};
    foreach my $rg_config (@rg_configs) {
       my $rg_id = $rg_config->{raid_group_id};
        my @faults = ();
        my $disk_id;
        foreach my $disk (@{$rg_config->{disks_to_shutdown}}) {
            $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info("Remove disk ".$disk_id." in RAID group ".$rg_id);
            push @faults, platform_shim_disk_remove($disk);
        }

        foreach my $disk (@{$rg_config->{disks_to_shutdown}}) {
           my $rg_id = $rg_config->{raid_group_id};
           $self->platform_shim_test_case_log_info('Waiting for removed state for RG ' . $rg_id);
           my @temp = ($disk);
           platform_shim_disk_wait_for_disk_removed($device, \@temp);
        }
        
        $rg_config->{shutdown_disks} = \@faults;
    }
    
    $rg_config_ref = \@rg_configs;
}

sub write_hole_restore_raid_groups
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    my $device = $self->{device};
    foreach my $rg_config (@rg_configs) {
       
       $self->platform_shim_test_case_log_step("Restore disks for RG: " . $rg_config->{raid_group_id});
        foreach my $disk_fault (@{$rg_config->{shutdown_disks}}) {
            $disk_fault->recover();
        }
        foreach my $disk_fault (@{$rg_config->{disk_faults}}) {
            $disk_fault->recover();
        }
    }
    $rg_config_ref = \@rg_configs;
}

sub write_hole_wait_for_rebuilds
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    my $device = $self->{device};
    foreach my $rg_config (@rg_configs) {
       
       $self->platform_shim_test_case_log_step("Wait for rebuild for RG: " . $rg_config->{raid_group_id});

       test_util_rebuild_wait_for_rebuild_complete($self, $rg_config->{raid_group}, 3600);
       foreach my $disk (@{$rg_config->{disks_to_fault}}) {
          my $disk_id = platform_shim_disk_get_property($disk, 'id');
          $self->platform_shim_test_case_log_info("Verify that rebuild completed on disk  ".$disk_id);
          platform_shim_event_verify_rebuild_completed({object => $device, disk => $disk});
       }
       $self->platform_shim_test_case_log_info("Verify that rebuild percent is 100%"); 
       my $rebuild_percent = platform_shim_raid_group_get_property($rg_config->{raid_group},'rebuilt_percent');
       $self->platform_shim_test_case_assert_true_msg($rebuild_percent == 100, "The rebuild percent has not reached 100: ".$rebuild_percent);
    }
    $rg_config_ref = \@rg_configs;
}

sub write_hole_create_all_parity_types_reserve
{
     my $tc = shift;
     my @raid_types = @mcr_test::test_util::test_util_configuration::g_parity_raid_types;
     push @raid_types, @mcr_test::test_util::test_util_configuration::g_parity_raid_types;
     
     my %params = ( min_raid_groups => scalar(@raid_types),
                    max_raid_groups => scalar(@raid_types),   
                    spares => 2,
                    raid_types => \@raid_types);
     return test_util_configuration_create_random_config($tc, \%params);
}

sub write_hole_cleanup
{
    my ($self) = @_;
    my $device = $self->{device};
    
    # Make sure rdgen is stopped since it opens an mcc volume.    
    platform_shim_device_stop_all_rdgen_io($device); 

    # Free rdgen memory
    platform_shim_device_cleanup_rdgen($device);

    # enable automatic sparing
    platform_shim_device_enable_automatic_sparing($device);
    
    # disable logical error injection
    platform_shim_device_disable_logical_error_injection($device);
    
    # restore load balancing
    platform_shim_device_enable_load_balancing($device);
}

1;
