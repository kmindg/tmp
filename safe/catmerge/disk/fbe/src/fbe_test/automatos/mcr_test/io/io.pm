package mcr_test::io::io;

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

my $total_test_cases = 0;

# ==================IO Test Cases===========================================
# * Create RAID groups of type R5, R3, R6, R10, R1, R0, ID
# * Create LUNs.
# * Run I/O of different flavors
# * Degrade raid group.
# * Degrade R6
# * Stop I/O
# * Restore Drives
# * Let Rebuild occur 
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
    my $luns_per_rg     = 1;
    my $lun_capacity_gb = 1;
    $total_test_cases = 0;

    $lun_capacity_gb = 5 if ($self->platform_shim_test_case_is_functional());
    $self->platform_shim_test_case_add_to_cleanup_stack({'io_cleanup' => {}});     

    # We do NOT disable load balancing since we want some parent raid groups NOT
    # on the CMI Active SP
    platform_shim_device_disable_load_balancing($device);
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    platform_shim_device_disable_automatic_sparing($device);
    
    # Create the raid groups, different widths and drive types, parity
    my %rg_params = (id => '0');
    my @rgs = platform_shim_device_get_raid_group($self->{device}, {});     
    my $total_rgs = @rgs;
    $self->platform_shim_test_case_log_info("found $total_rgs raid groups");

    # Step 1: Set up test cases for each raid group
    $self->platform_shim_test_case_log_step("Create raid groups");
    my @rg_configs = @{io_initialize_rg_configs($self, $luns_per_rg, $lun_capacity_gb)};

    # Step 2: Start io on luns
    platform_shim_device_initialize_rdgen($device);

    $self->platform_shim_test_case_log_info("Run I/O tests Non-degraded.");
    io_run_io_tests($self, \@rg_configs, 1, "non_degraded");
    $self->platform_shim_test_case_log_info("Non-degraded I/O Complete.  Total test cases: " . $total_test_cases);
    
    io_degrade_raid_groups($self, \@rg_configs, 0);

    $self->platform_shim_test_case_log_info("Run I/O tests degraded.");
    io_run_io_tests($self, \@rg_configs, "single_degraded");
    $self->platform_shim_test_case_log_info("Degraded I/O Complete.  Total test cases: " . $total_test_cases);

    $self->platform_shim_test_case_log_info("Double degrade R6");
    io_degrade_raid_groups($self, \@rg_configs, 1);

    $self->platform_shim_test_case_log_info("Run I/O tests double degraded.");
    io_run_io_tests($self, \@rg_configs, "double_degraded");
    $self->platform_shim_test_case_log_info("Double Degraded I/O Complete.  Total test cases: " . $total_test_cases);
       
    io_restore_raid_groups($self, \@rg_configs);

    io_wait_for_rebuilds($self, \@rg_configs);
       
    test_util_event_check_for_unexpected_errors($self);
    
    test_util_general_log_test_time($self);
    platform_shim_device_enable_automatic_sparing($device);
}
sub io_create_all_raid_types
{
     my $tc = shift;
     my @raid_types = qw(disk r0 r1 r3 r5 r6 r10);
     
     my %params = ( min_raid_groups => scalar(@raid_types),
                    max_raid_groups => scalar(@raid_types),   
                    spares => 2,
                    raid_types => \@raid_types);
     return test_util_configuration_create_random_config($tc, \%params);
}
sub io_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_gb) = @_;
   my @rgs;
   if ($self->platform_shim_test_case_get_parameter(name => 'clean_config'))
   {
       
       if ($self->platform_shim_test_case_is_functional()) {
          @rgs = @{io_create_all_raid_types($self)};
       } else {
           @rgs = @{test_util_configuration_create_limited_raid_types_reserve($self)};
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
      my @disk_faults = ();
      $rg_config{disk_faults} = \@disk_faults;

      my @disks_to_shutdown = (shift(@disks)); 
      $rg_config{disks_to_shutdown} = \@disks_to_shutdown;
      
      my $data_disks = test_util_configuration_get_data_disks($self, $rg);
      $rg_config{data_disks} = $data_disks;

      my @luns = ();
      if ($self->platform_shim_test_case_get_parameter(name => 'clean_config'))
      {
         $lun_capacity_gb = test_util_configuration_get_per_lun_capacity($self, \%rg_config, $lun_capacity_gb, $luns_per_rg);

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

my @test_cases_short = ({op => "read", block_spec => "stripe", block_count => 0, lba_spec => 'sequential', threads => 1}, );

my @test_cases = ({op => "read", block_spec => "stripe", block_count => 0, lba_spec => 'sequential', threads => 1},
	          {op => "write", block_spec => "stripe", block_count => 0, lba_spec => 'sequential', threads => 1},
	          {op => "write_check", block_spec => "stripe", block_count => 0, lba_spec => 'sequential', threads => 1},

                  # only 0xC0 less than a full stripe.
		  {op => "read", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'sequential', inc_lba => 0x40, threads => 1},
	          {op => "write", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'sequential', inc_lba => 0x40, threads => 1},
	          {op => "write_check", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'sequential', inc_lba => 0x40, threads => 1},

		  {op => "read", block_spec => "constant", block_count => 0x40, lba_spec => 'sequential', inc_lba => 0x200,threads => 1},
	          {op => "write", block_spec => "constant", block_count => 0x40, lba_spec => 'sequential', inc_lba => 0x200, threads => 1},
	          {op => "write_check", block_spec => "constant", block_count => 0x40, lba_spec => 'sequential', inc_lba => 0x200, threads => 1},

                  # Random Light
                  {op => "read", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 4},
	          {op => "write", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 4},
	          {op => "write_check", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 4},

		  {op => "read", block_spec => "sub_stripe_random", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 4},
	          {op => "write", block_spec => "sub_stripe_random", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 4},
	          {op => "write_check", block_spec => "sub_stripe_random", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 4},

		  {op => "read", block_spec => "random", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200,threads => 4},
	          {op => "write", block_spec => "random", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200, threads => 4},
	          {op => "write_check", block_spec => "random", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200, threads => 4},

                  # Random Heavy
                  {op => "read", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 32},
	          {op => "write", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 32},
	          {op => "write_check", block_spec => "stripe", block_count => 0, lba_spec => 'random', threads => 32},

		  {op => "read", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 32},
	          {op => "write", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 32},
	          {op => "write_check", block_spec => "sub_stripe", block_count => 0xc0, lba_spec => 'random', inc_lba => 0x40, threads => 32},

		  {op => "read", block_spec => "constant", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200,threads => 32},
	          {op => "write", block_spec => "constant", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200, threads => 32},
	          {op => "write_check", block_spec => "constant", block_count => 0x40, lba_spec => 'random', inc_lba => 0x200, threads => 32},
                   );
sub test_util_io_get_rdgen_params
{
   my ($self, $rg_config, $case) = @_;
   my $device = $self->{device};
   my $rg = $rg_config->{raid_group};
   my $width = $rg_config->{width};
   my $data_disks = $rg_config->{data_disks};
   my $element_size = 0x80;
   my $stripe_size = $data_disks * $element_size;
   my %io_params = ();
   
   if ($case->{op} eq "read") {
      $io_params{read_only} = 1;
   } elsif ($case->{op} eq "write") {
      $io_params{write_only} = 1;
   } elsif ($case->{op} eq "write_check") {
      $io_params{write_read_check} = 1;
   } else {
      # default to read.
      $io_params{read_only} = 1;
   }

   if ($case->{block_spec} eq "constant") {
      $io_params{block_count} = $case->{block_count};
      $io_params{use_block} = 1;
   }elsif ($case->{block_spec} eq "random") {
      $io_params{block_count} = $case->{block_count};
      $io_params{use_block} = 0;
   } elsif ($case->{block_spec} eq "stripe") {
      $io_params{block_count} = $stripe_size;
      $io_params{use_block} = 1;
   } elsif ($case->{block_spec} eq "sub_stripe") {
      # constant size of less than a full stripe by the input block_count
      if ($case->{block_count} >= $stripe_size) {
         $io_params{block_count} = $element_size / 2;
      } else {
         $io_params{block_count} = $stripe_size - $case->{block_count};
      }
      $io_params{use_block} = 1;
   } elsif ($case->{block_spec} eq "sub_stripe_random") {
      # Random size of less than a full stripe by the input block_count
      if ($case->{block_count} >= $stripe_size) {
         $io_params{block_count} = $element_size / 2;
      } else {
         $io_params{block_count} = $stripe_size - $case->{block_count};
      }
      $io_params{use_block} = 0;
   } elsif ($case->{block_spec} eq "element") {
      $io_params{block_count} = $element_size;
      $io_params{use_block} = 1;
   } elsif ($case->{block_spec} eq "element_random") {
      $io_params{block_count} = $element_size;
      $io_params{use_block} = 0;
   } elsif ($case->{block_spec} eq "half_element") {
      $io_params{block_count} = $element_size / 2;
      $io_params{use_block} = 1;
   } elsif ($case->{block_spec} eq "half_element_random") {
      $io_params{block_count} = $element_size / 2;
      $io_params{use_block} = 0;
   }
   if ($case->{lba_spec} eq "random") {
   } elsif ($case->{block_spec} eq "sequential") {
      sequential_increasing_lba => 1;
      if (defined($case->{inc_lba})) {
         $io_params{increment_lba} = $io_params{block_count} + $case->{inc_lba}
      }
   }
   if ($io_params{block_count} <= 0) {
      platform_shim_test_case_throw_exception("block count is <= 0 blocks: " . $io_params{block_count});
   }
   $self->platform_shim_test_case_log_step("RG: " . $rg_config->{raid_group_id} . " type: " . $rg_config->{raid_type} . 
                                           " data disks " . $data_disks .
                                           " width " . $width .
                                           " stripe_size " . $stripe_size .
                                           " element_size " . $element_size);
   $io_params{thread_count} = $case->{threads};
   return %io_params;
}
sub io_run_io_tests {
   my ($self, $rg_configs_ref, $mode) = @_;
     
   my @rg_configs = @{$rg_configs_ref};
   my $rg_count = scalar(@rg_configs);
   my $device = $self->{device};

   my $io_run_seconds;
    
   if ($self->platform_shim_test_case_is_functional()){
      if ($mode ne "double_degraded") {
         # We run less time since there are so many more LUNs to start on
         # by the time we finish starting, we have been running I/O for minutes.
         $io_run_seconds = 10;
      } else {
         $io_run_seconds = 30;
      }
   } else {
      $io_run_seconds = 1;
   }
   
   platform_shim_device_rdgen_reset_stats($device);
   foreach my $io_case (@test_cases) {
       
      $self->platform_shim_test_case_log_step("Start I/O on all raid groups");
      $self->platform_shim_test_case_log_step($io_case->{op} . " " . $io_case->{lba_spec} . " " . 
                                              $io_case->{block_spec} . " threads: " . $io_case->{threads});

      foreach my $rg_config (@rg_configs) {

         if ( ($mode eq "single_degraded") && 
              (($rg_config->{raid_type} eq "disk") || ($rg_config->{raid_type} eq "r0")) ) {
             next;
         } elsif (($mode eq "double_degraded") && 
             ($rg_config->{raid_type} ne "r6")) {
             next;
         }
         my $luns_ref = $rg_config->{luns};
         my @luns = @$luns_ref;
         
         my %rdgen_params = test_util_io_get_rdgen_params($self, $rg_config, $io_case);
         #$self->platform_shim_test_case_log_step("Start I/O for RG: " . $rg_config->{raid_group_id});

         $self->platform_shim_test_case_log_step($io_case->{op} . " lba: " . $io_case->{lba_spec} . 
                                                 " blocks " . $io_case->{block_spec} . " blocks: " . $io_case->{block_count} . 
                                                 " threads: " . $io_case->{threads});
         $self->platform_shim_test_case_log_step("use_block:  " . $rdgen_params{use_block} . " block_count: " . $rdgen_params{block_count});
         foreach my $lun (@luns) {
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
            $total_test_cases = $total_test_cases + 1;
         }
       }

      $self->platform_shim_test_case_log_info("wait $io_run_seconds seconds for I/O to run.");
      sleep $io_run_seconds;

      #test_util_io_display_rdgen_stats($self);
      $self->platform_shim_test_case_log_info("Stopping rdgen threads");
      platform_shim_device_rdgen_stop_io($self->{device}, {});

      $self->platform_shim_test_case_log_step("Wait for rdgen to stop. ");
      test_util_io_wait_rdgen_thread_complete($self, 3600);
    }
}
sub io_degrade_raid_groups
{
    my ($self, $rg_config_ref, $start_index) = @_;
    my @rg_configs = @{$rg_config_ref};
    
    my $device = $self->{device};
    # Step 2: Fault a disk in the raid group and verify rl
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $raid_type = $rg_config->{raid_type};
        my @faults = ();
        my $disk_id;
        if ($raid_type eq 'r0' || $raid_type eq 'disk') {
           next;
        }
        if ($raid_type ne 'r6' && $start_index != 0) {
           next;
        }
        my @disks = @{$rg_config->{disks_to_fault}};
        my $disk = $disks[$start_index];
        $disk_id = platform_shim_disk_get_property($disk, 'id');
        $self->platform_shim_test_case_log_info("Remove disk ". $disk_id." in RAID group ".$rg_id);
        @faults = @{$rg_config->{disk_faults}};
        push @faults, platform_shim_disk_remove($disk);
        $rg_config->{disk_faults} = \@faults;
    }
    # Cannot reliably wait for a disk to be removed due to framwork bugs.
    # The below code just hangs if the drive is already removed.

    if (0) {
       foreach my $rg_config (@rg_configs) {
           my $rg_id = $rg_config->{raid_group_id};
           my @disks = @{$rg_config->{disks_to_fault}};
           my $disk = $disks[$start_index];
           my $raid_type = $rg_config->{raid_type};
           if ($raid_type eq 'r0' || $raid_type eq 'disk') {
              next;
           }
           if ($raid_type ne 'r6' && $start_index != 0) {
              next;
           }
           #my @temp = ($disk);
           #platform_shim_disk_wait_for_disk_state($device, \@temp, 'Removed');
           $self->platform_shim_test_case_log_info('Waiting for removed state for RG ' . $rg_id . " disk " . $start_index);
           platform_shim_disk_wait_for_disk_removed($device, [$disk]);
       }
    }
    
    $rg_config_ref = \@rg_configs;
}

sub io_restore_raid_groups
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

sub io_wait_for_rebuilds
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    my $device = $self->{device};
    foreach my $rg_config (@rg_configs) {
       
       $self->platform_shim_test_case_log_step("Wait for rebuild for RG: " . $rg_config->{raid_group_id});

       test_util_rebuild_wait_for_rebuild_complete($self, $rg_config->{raid_group}, 3600);
       $self->platform_shim_test_case_log_info("Verify that rebuild percent is 100%"); 
       my $rebuild_percent = platform_shim_raid_group_get_property($rg_config->{raid_group},'rebuilt_percent');
       $self->platform_shim_test_case_assert_true_msg($rebuild_percent == 100, "The rebuild percent has not reached 100: ".$rebuild_percent);
    }
    $rg_config_ref = \@rg_configs;
}

sub io_cleanup
{
    my ($self) = @_;
    my $device = $self->{device};
    
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
