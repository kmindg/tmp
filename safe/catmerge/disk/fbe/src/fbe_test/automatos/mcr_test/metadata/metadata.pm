package mcr_test::metadata::metadata;

use base qw(mcr_test::platform_shim::platform_shim_test_case);

use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_sniff;
use mcr_test::test_util::test_util_io;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::test_util::test_util_verify;
use Data::Dumper;
use strict;
use warnings;

#todo: try not to use any direct calls into frame.  Replace with shim functions.
 
#todo: remove debug before checkin
my $DEBUG = 0;
my $CONTINUE_ON_FAILURE = 0;   #todo: do not check in.

#todo: now that fbeapix is fixed,  try using parallel again to speed up test.
my $VERIFY_IN_PARALLEL = 0;
my $SKIP_REBOOTS = 0;
my $CORRUPTION_TYPE_ZEROS = 0;
my $CORRUPTION_TYPE_COH_ERRORS = 1;


# ==================Metadata Test Cases===========================================
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

    $SKIP_REBOOTS = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($SKIP_REBOOTS == 1) {
       print "Skiping reboot during test since skip_reboot parameter set.\n";
    }

    $self->platform_shim_test_case_add_to_cleanup_stack({'metadata_cleanup' => {}});                     

    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    my $rdgen_target_sp = $active_sp;

    if ($self->platform_shim_test_case_get_parameter(name => 'clean_config') == '1')
    {
        # make sure we are always starting with a sane config.  e.g. no lei records left over from last test.
        metadata_cleanup($self);  
    }

    # Enable logical error injection
    platform_shim_device_enable_logical_error_injection($self->{device});
    test_util_general_set_target_sp($self, $passive_sp);
    platform_shim_device_enable_logical_error_injection($self->{device});
    test_util_general_set_target_sp($self, $active_sp);
    #metadata_verify_no_lei_records($self);



    # Step 1: Based on the platform under test configure the following:
    #           o $luns_per_rg
    #           o $lun_capacity_gb
    #           o $lun_io_capacity_percent
    #           o $lun_io_max_lba
    $self->platform_shim_test_case_log_step("Configure test parameters (luns per rg, lun io percent, etc) based on platform");
    metadata_configure_params_based_on_platform($self, \$luns_per_rg, \$lun_capacity_gb);


    # Set up test cases for each raid group
    $self->platform_shim_test_case_log_step("Create raid groups");
    my @rg_configs = @{metadata_initialize_rg_configs($self, $luns_per_rg, $lun_capacity_gb)};
   

    # Start test
    #     
    metadata_bind_luns($self, \@rg_configs);

    if ($DEBUG)
    {
        debugtest($self, \@rg_configs);
        return;
    }
    
    metadata_wait_for_zeroing($self, \@rg_configs);  
          
    if ($VERIFY_IN_PARALLEL)
    {
        metadata_create_pvd_paged_errors($self, \@rg_configs); 
        metadata_create_raid_paged_errors($self, \@rg_configs); 
        metadata_run_verify_on_pvd_paged_errors_in_parallel($self, \@rg_configs);        
        #metadata_run_verify_on_raid_paged_errors_in_parallel($self, \@rg_configs);
    }
    else
    {
        
        metadata_run_verify_on_pvd_paged_errors_in_serial($self, \@rg_configs);        
        metadata_run_verify_on_raid_paged_errors_in_serial($self, \@rg_configs);
    }

    test_util_general_set_target_sp($self, $rdgen_target_sp);
    metadata_verify_zeroed_data($self, \@rg_configs);


    if ($self->platform_shim_test_case_is_functional())
    {        
        # 1. write background pattern
        # 2. run reboot test case
        #    2a. corrupt paged metadata
        #    2b. reboot
        #    2c. wait for verify
        #    2d. verify data pattern.
        # 3. run rebuild test case.
        #    3a.  same as above.
        # 4. run verify test case.
        #    4a.  same as above.
        
        platform_shim_device_initialize_rdgen($device);
        metadata_write_pattern($self);
        
        metadata_run_reboot_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS);
        metadata_run_rebuild_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS); 
        metadata_run_verify_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS);  
    }

    test_util_general_log_test_time($self);
}

sub debugtest
{
    my ($self, $rg_configs_ref) = @_;
    my @rg_configs = @{$rg_configs_ref}; 
    my $device = $self->{device};

    $self->platform_shim_test_case_log_step("******   Debug Test  *********");
    
    #metadata_run_verify_on_raid_paged_errors_in_serial($self, \@rg_configs);
    my $rdgen_target_sp = platform_shim_device_get_active_sp($self->{device}, {});   
    platform_shim_device_initialize_rdgen($device);
    metadata_write_pattern($self);

    metadata_run_reboot_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS);
    metadata_run_rebuild_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS); 
    metadata_run_verify_test($self, \@rg_configs, $rdgen_target_sp, $CORRUPTION_TYPE_COH_ERRORS);  
}


sub metadata_configure_params_based_on_platform
{
    my ($self, $luns_per_rg_ref, $lun_capacity_gb_ref) = @_;
     
    my $device = $self->{device};

    if (platform_shim_device_is_simulator($device)) {
        # Simulator with maximum drive capacity of 10GB
        $$luns_per_rg_ref     = 1;                  
        $$lun_capacity_gb_ref = 1;
    } else { # Hardware
        if ($self->platform_shim_test_case_is_promotion())
        {
            $$luns_per_rg_ref     = 1;                  
            $$lun_capacity_gb_ref = 1;
        }
        elsif ($self->platform_shim_test_case_is_functional())
        {
            #todo
            #$$luns_per_rg_ref     = 1;                  
            #$$lun_capacity_gb_ref = 100;
        }
        elsif ($self->platform_shim_test_case_is_bughunt())
        {
        }
    }
}


sub metadata_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_gb) = @_;
   my @rgs;

   my $active_sp = platform_shim_device_get_active_sp($self->{device}, {});


   if ($self->platform_shim_test_case_get_parameter(name => 'clean_config') == '1')
   {
       #@rgs = @{test_util_configuration_create_one_random_raid_group($self, {})};
       $self->platform_shim_test_case_log_step("Step: Initialize raid group configs from array of raid groups");
       #@rgs = @{test_util_configuration_create_all_raid_types_reserve($self)};  # I don't need spares.
       $self->platform_shim_test_case_log_info("Waiting for minimum disks to be zeroed before creating RGs.");

       # To support small array configs, do not wait for minimum disks to be zeroed.  
       #test_util_zeroing_wait_min_disks_zeroed($self, 3600);
        my $params = {
            spares => 0,   # no spares needed for this test.
            b_zeroed_only => 1,
        };
       @rgs = @{test_util_configuration_create_all_raid_types($self, $params)};       
   }
   else
   {
       $self->platform_shim_test_case_log_step("Step: Using existing raid group configs");
       @rgs = platform_shim_device_get_raid_group($self->{device}, {});       
   }   

   if (scalar(@rgs) == 0)
   {
        $self->platform_shim_test_case_log_error("No exising RGs to use. ");
   }
   
   my @rg_configs = ();
   my $rg_index = 0;
   my $tested_empty_rg_copy = 0;   
   my $pvd_info_hash_ref = test_util_disk_get_pvd_info_hash($self);  
   $self->platform_shim_test_case_log_info("Pvd Info List: ". Dumper($pvd_info_hash_ref) );

   foreach my $rg (@rgs) {      
      my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
      my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
      my $width = platform_shim_raid_group_get_property($rg, 'raid_group_width'); 
      my @disks = platform_shim_raid_group_get_disk($rg);
      
      my $position_to_error = int(rand($width));  
      #my $position_to_error = int(0);

      # Setup the test case   
      my %rg_config = ();
      
      $rg_config{raid_group_id} = $rg_id;      
      $rg_config{raid_group} = $rg;
      $rg_config{luns_per_rg} = $luns_per_rg;
      $rg_config{lun_capacity_gb} = $lun_capacity_gb;
      $rg_config{raid_group_fbe_id} = platform_shim_raid_group_get_property($rg, 'fbe_id');
      $rg_config{width} = $width;
      $rg_config{raid_type} = $raid_type;     
      $rg_config{disks} = \@disks;
      $rg_config{disk_for_pvd_paged_errors} = $disks[$position_to_error];
      $rg_config{technology} = platform_shim_disk_get_property($disks[0], 'technology');
      #$rg_config{disk_for_pvd_paged_errors} = metadata_wait_for_any_disk_zeroed($self, \@disks, $pvd_info_hash_ref, 600);

      my %rg_info = %{platform_shim_raid_group_get_rg_info($active_sp, $rg_config{raid_group_fbe_id})};
      $rg_config{paged_metadata_start_lba} = $rg_info{paged_metadata_start_lba};
      $rg_config{paged_metadata_size} = $rg_info{paged_metadata_size};
                  
      # Push the test case
      push @rg_configs, \%rg_config;
   }
   # Return the address of the raid groups test configs created
   return \@rg_configs;
}

sub metadata_bind_luns
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};
    
    $self->platform_shim_test_case_log_step("Step: Binding Luns");

    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $rg = $rg_config->{raid_group};
        my @luns = ();

        if ($self->platform_shim_test_case_get_parameter(name => 'clean_config') == '1')
        {            
            $self->platform_shim_test_case_log_step("Binding Luns on RAID ID " . $rg_id);
            foreach (1..($rg_config->{luns_per_rg})) {
                my $lun = test_util_configuration_create_lun($self, $rg, {size => $rg_config->{lun_capacity_gb}."GB", initial_verify => 0});
                push @luns, $lun;
            }            
        }
        else 
        {
            $self->platform_shim_test_case_log_step("Using existing Luns. RAID ID " . $rg_id);
            @luns = platform_shim_raid_group_get_lun($rg);
        }

        if (scalar(@luns) == 0)
        {
            $self->platform_shim_test_case_log_error("No exising luns to use.  RAID ID: ". $rg_id);
        }

        $rg_config->{luns} = \@luns;
    }
}


sub metadata_wait_for_zeroing
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    
    my $timeout = 600;  

    $self->platform_shim_test_case_log_step("Waiting for Lun Zeroing to complete");

    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $disk = $rg_config->{disk_for_pvd_paged_errors};  
        my $disk_id = platform_shim_disk_get_property($disk, 'id');        
        
        my @luns = @{$rg_config->{luns}};
        foreach my $lun (@luns) {
            my $lun_id = platform_shim_lun_get_property($lun, 'id');
            $self->platform_shim_test_case_log_info("Verify zeroing completes on lun $lun_id [RG $rg_id]");
            test_util_zeroing_wait_for_lun_zeroing_percent($self, $lun, 100, $timeout);  
        }
    }    
}


sub metadata_verify_no_lei_records
{
    my ($self) = @_;
   
    $self->platform_shim_test_case_log_step("Verifying no existing LEI records");  

    my @sps = platform_shim_device_get_sp($self->{device});
    foreach my $sp (@sps) { 
        my %lei_stats = platform_shim_device_get_logical_error_injection_statistics($self->{device}, $sp);
        $self->platform_shim_test_case_log_info("LEI status [" . $sp->getProperty('name') . "] " . Dumper(\%lei_stats) );
        my %lei_records = platform_shim_device_get_logical_error_records($self->{device}, $sp);
        $self->platform_shim_test_case_log_info("LEI records [" . $sp->getProperty('name') . "] " . Dumper(\%lei_records) );

        foreach my $key (keys %lei_records)
        {
            # There is a bug where we always get at least one record with a key 'error_bit_count'. So only look for keys that
            # are integers.
            if ($key =~ m/^\d+$/)
            {
                $self->platform_shim_test_case_log_error("Record $key found.  Removing.");
                platform_shim_device_delete_logical_error_injection_record(int($key), $sp);
            }
        }
    }
}

sub metadata_create_pvd_paged_errors
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    $self->platform_shim_test_case_log_step("Setting up PVD paged metadata error records");

    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $pvd_object_id = platform_shim_disk_get_property($rg_config->{disk_for_pvd_paged_errors}, 'fbe_id');
        
        # Setup error injection record
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $rg_config->{disk_for_pvd_paged_errors}});        
        my $pvd_metadata_info = platform_shim_disk_get_pvd_metadata_info($rg_config->{disk_for_pvd_paged_errors});
        $self->platform_shim_test_case_log_info("Creating record on SP:".$active_sp->getProperty('name')." for PVD ". sprintf("0x%x",$pvd_object_id). 
                                                " RG [$rg_id], PVD paged metadata start lba " . $pvd_metadata_info->{paged_metadata_start_lba} .
                                                " size " . $pvd_metadata_info->{paged_metadata_size});

        metadata_create_lei_object_record($self, $active_sp, $pvd_metadata_info->{paged_metadata_start_lba}, $pvd_object_id);
    }
}

sub metadata_create_raid_paged_errors
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    

    $self->platform_shim_test_case_log_step("Setting up RAID paged metadata error records");

    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
        my $rg = $rg_config->{raid_group};
        my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba};        
        my $paged_metadata_size = $rg_config->{paged_metadata_size};

        $self->platform_shim_test_case_log_info("Inject errors on RAID (ID/obj) (" . $rg_id . "/" . sprintf("0x%x",$rg_fbe_id) . ") paged metadata." .
                                                " start lba: " . $paged_metadata_start_lba . 
                                                " size: " . $paged_metadata_size);     
        
        my %map = platform_shim_raid_group_map_lba($rg, $paged_metadata_start_lba);

        $self->platform_shim_test_case_log_info("mapLba: " . $paged_metadata_start_lba . 
                                                " pba: " . $map{pba} .
                                                " data_pos: " . $map{data_pos} .
                                                " parity_pos: " . $map{parity_pos} .
                                                " chunk_index: " . $map{chunk_index} .
                                                " metadata_lba: " . $map{metadata_lba} .
                                                " original_lba: " . $map{original_lba} .
                                                " offset: " . $map{offset} );     
            
        
        # Setup error injection

        my @disks = @{$rg_config->{disks}};
        my $rg_str = "RG: $rg_id -";
        foreach my $disk (@disks)
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');   
            $rg_str = $rg_str . " $disk_id";         
        }
        $self->platform_shim_test_case_log_info($rg_str);
       
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disks[$map{data_pos}]});
        $self->platform_shim_test_case_log_info("Set up logical error injection on SP:".$active_sp->getProperty('name').
                                                "for ".platform_shim_disk_get_property($disks[$map{data_pos}], 'id'));

        my $pvd_object_id = platform_shim_disk_get_property($disks[$map{data_pos}], 'fbe_id');
                
        my $start_lba = Math::BigInt->from_hex($map{pba})-Math::BigInt->from_hex($map{offset});
        metadata_create_lei_object_record($self, $active_sp, $start_lba->as_hex(), $pvd_object_id);                    
    }
}


sub metadata_create_lei_global_record
{
    my ($self, $active_sp, $lba_start_str) = @_;
    my %lei_params;

    $self->platform_shim_test_case_log_info("Creating record on SP:".$active_sp->getProperty('name').
                                            ", paged metadata start lba $lba_start_str");

    %lei_params = (active_sp            => $active_sp,
                      error_injection_type => 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR',  
                      error_mode           => 'FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED',
                      error_count          => 0, 
                      error_limit          => 0x15, 
                      error_bit_count      => 0,
                      lba_address          => $lba_start_str, 
                      blocks               => 1);


    platform_shim_device_create_logical_error_injection_record($self->{device}, \%lei_params);

}

sub metadata_create_lei_object_record
{
    my ($self, $active_sp, $lba_start_str, $pvd_object_id) = @_;
    my %lei_params;


    $self->platform_shim_test_case_log_info("Creating record on SP:".$active_sp->getProperty('name')." for PVD ". sprintf("0x%x",$pvd_object_id). 
                                            " paged metadata start lba $lba_start_str.");

        %lei_params = (active_sp            => $active_sp,
                          error_injection_type => 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR',  
                          error_mode           => 'FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED',
                          error_count          => 0, 
                          error_limit          => 0x15, 
                          error_bit_count      => 0,
                          lba_address          => $lba_start_str,                          
                          blocks               => 1);

    platform_shim_device_create_logical_error_injection_record_for_fbe_object_id($self->{device}, \%lei_params, $pvd_object_id); 
}


sub metadata_run_verify_on_paged_errors
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    
    my $device = $self->{device};  

    $self->platform_shim_test_case_log_step("Run Verify on paged errors");

    foreach my $rg_config (@rg_configs) {        
        my $rg_id = $rg_config->{raid_group_id};
        my $disk = $rg_config->{disk_for_pvd_paged_errors};
        my $pvd_metadata_info = platform_shim_disk_get_pvd_metadata_info($disk);
        my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disk});
        my %lei_stats;
        my %lei_records;
        my $sniff_passed = 0;
 
    }

    #TBD
}

sub metadata_run_verify_on_pvd_paged_errors_in_serial
{
    my ($self, $rg_configs_ref) = @_;
    my @rg_configs = @{$rg_configs_ref};    
    my $device = $self->{device};  
    my %pvds;

    $self->platform_shim_test_case_log_step("Run Verify against PVD metadata and wait for completion");

    # Verify PVD's paged metadata errors
    foreach my $rg_config (@rg_configs) {        
        my $rg_id = $rg_config->{raid_group_id};
        my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
        my $disk = $rg_config->{disk_for_pvd_paged_errors};
        my $pvd_metadata_info = platform_shim_disk_get_pvd_metadata_info($disk);    #takes 5 seconds to make following calls into framework.
        my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');             
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disk});
                

        $self->test_util_general_set_target_sp($active_sp);

        test_util_sniff_clear_verify_report($self, $disk);       

        metadata_create_lei_global_record($self, $active_sp, $pvd_metadata_info->{paged_metadata_start_lba});

        # enabling object just before causing error injecting becuase there are multiple pvds that need injection.   If
        # we enabled them all at once then most likely the injection would occur for the wrong pvd.   Currently no way
        # to associate an error record with a given pvd. 
        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id); 

        $self->platform_shim_test_case_log_info("Injecting errors on PVD " . sprintf("0x%x",$pvd_object_id) . 
                                                " paged metadata [RG $rg_id] on sp".platform_shim_sp_get_short_name($active_sp));            
       
        # move sniff to injected area of PVD paged region.  occasionally error injection doesn't hit cp.  I think there is 
        # a race somewhere when set_checkpoint is called that it increments to next chunk.   Therefore setting to previous
        # chunk to be sure we don't miss it.
        #
        my $checkpoint = Math::BigInt->from_hex($pvd_metadata_info->{paged_metadata_start_lba})-0x800;
        test_util_sniff_set_checkpoint($self, $disk, $checkpoint->as_hex()); 
        #test_util_sniff_set_checkpoint($self, $disk, $pvd_metadata_info->{paged_metadata_start_lba}); 

        $pvds{$pvd_object_id} = {'completed' => 0, 'sp' => $active_sp};

        my %lei_stats;
        my $is_injected = 0;
        #todo: change to use time base timeout.
        foreach (1..10)
        {
            %lei_stats = platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id($self->{device}, $active_sp, $pvd_object_id);
            if (int($lei_stats{error_injected_count}) > 0) 
            {
                $is_injected = 1;
                last;
            }
            $self->platform_shim_test_case_log_info("Waiting for sniff injection of PVD page" ); 

            $self->platform_shim_test_case_log_info("LEI object stats: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_stats) );

            my %lei_global_stats = platform_shim_device_get_logical_error_injection_statistics($device, $active_sp);
            $self->platform_shim_test_case_log_info("LEI global stats: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_global_stats) );

            my %lei_records = platform_shim_device_get_logical_error_records($device, $active_sp);
            $self->platform_shim_test_case_log_info("LEI records: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_records) );

            my %hash = ();
            my %pvdinfo_hash = %{platform_shim_sp_execute_fbecli_command($active_sp, "pvdinfo -object_id ".sprintf("0x%x",$pvd_object_id), \%hash)};
            $self->platform_shim_test_case_log_info("pvdinfo :" . sprintf("0x%x",$pvd_object_id) . " -> ". Dumper(\%pvdinfo_hash) );

            my %hash2 = ();
            my %rginfo_hash = %{platform_shim_sp_execute_fbecli_command($active_sp, "rginfo -d -object_id ".sprintf("0x%x",$rg_fbe_id), \%hash2)};
            $self->platform_shim_test_case_log_info("rginfo :" . sprintf("0x%x",$rg_fbe_id) . " -> ". Dumper(\%rginfo_hash) );

            sleep 1;
        }

        if (! $is_injected)
        {
            platform_shim_test_case_throw_exception("Failed To Inject on Sniff to PVD paged.");
        }

        # cleanup error records so they don't collide, then re-enable
        #
        platform_shim_device_disable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  
        platform_shim_device_disable_logical_error_injection($device);   
        platform_shim_device_disable_logical_error_injection_records($device);   
        platform_shim_device_enable_logical_error_injection($self->{device});
    }
}



sub metadata_get_pvd_lba_for_raid_metadata_start
{
    my ($self, $rg_config) = @_;

    my $device = $self->{device};
    #my $rg_config = %{$rg_config_ref};
    my $rg_id = $rg_config->{raid_group_id};
    my $rg_type = $rg_config->{raid_type};
    my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
    my $rg = $rg_config->{raid_group};
    my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba}; 
    my $paged_metadata_size = $rg_config->{paged_metadata_size};
    my $disk;
    my $pvd_object_id;
    my $pvd_lba;
    my %pvd_info;
    my $position;
          
    # If R10 then inject errors into mirrors below, since the metadata for striper above is not
    # interesting.

    if ($rg_type eq 'r10')
    {
        $self->platform_shim_test_case_log_info("RG is R10. Getting start lba for underlying mirror");                     

        my @mirror_fbe_ids = @{platform_shim_raid_group_get_downstream_mirrors($rg)};   
        $rg_fbe_id = hex($mirror_fbe_ids[0]);  

        my $default_sp = platform_shim_device_get_active_sp($device, {});
        my %r0_info = %{platform_shim_raid_group_get_rg_info($default_sp, $rg_config->{raid_group_fbe_id})};
        $paged_metadata_start_lba = $r0_info{paged_metadata_start_lba};        

        my @vd_ids = @{platform_shim_device_get_downstream_objects_for_fbe_object_id($self->{device}, $mirror_fbe_ids[0])};            
        my @pvd_ids = @{platform_shim_device_get_downstream_objects_for_fbe_object_id($self->{device}, $vd_ids[0])};        
        $pvd_object_id = hex($pvd_ids[0]);
        $position = 0;
        

        $disk = metadata_get_disk_from_pvd_object_id($self->{device}, $pvd_ids[0], $rg_config->{disks});

        # Since this is a mirror,  I don't need to call maplba,  since I RG metadata start lba is the same LBA at PVD level.

        #todo: Need to verify that this is correct. Use rginfo -chunk_info and -
        $pvd_lba = Math::BigInt->from_hex($paged_metadata_start_lba) + 0x10000;
    } 
    else
    {
        my %map = platform_shim_raid_group_map_lba($rg, $paged_metadata_start_lba);
        $self->platform_shim_test_case_log_info("mapLba: " . $paged_metadata_start_lba . 
                                                " pba: " . $map{pba} .
                                                " data_pos: " . $map{data_pos} .
                                                " parity_pos: " . $map{parity_pos} .
                                                " chunk_index: " . $map{chunk_index} .
                                                " metadata_lba: " . $map{metadata_lba} .
                                                " original_lba: " . $map{original_lba} .
                                                " offset: " . $map{offset} );         
        #$pvd_lba = Math::BigInt->from_hex($map{pba}) - Math::BigInt->from_hex($map{offset});
        $pvd_lba = Math::BigInt->from_hex($map{pba});
        my @disks = @{$rg_config->{disks}};
        $position = $map{data_pos};
        $disk = $disks[$position];
        $pvd_object_id = platform_shim_disk_get_pvd_object_id($disk);
    }

    $self->platform_shim_test_case_log_info("Get RAID metadata start LBA. ID:$rg_id obj:" . sprintf("0x%x",$rg_fbe_id) . " type:$rg_type " .
                                            " start lba:" . $paged_metadata_start_lba . " pvd lba: " . $pvd_lba->as_hex());     
    $pvd_info{disk} = $disk;
    $pvd_info{lba} = $pvd_lba;
    $pvd_info{object_id} = $pvd_object_id;
    $pvd_info{position} = $position;

    return \%pvd_info;
}


sub metadata_get_pvd_lba_for_raid_metadata_end
{
    my ($self, $rg_config) = @_;

    my $device = $self->{device};
    #my $rg_config = %{$rg_config_ref};
    my $rg_id = $rg_config->{raid_group_id};
    my $rg_type = $rg_config->{raid_type};
    my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
    my $rg = $rg_config->{raid_group};
    my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba}; 
    my $paged_metadata_size = $rg_config->{paged_metadata_size};
    my $end_lba =  Math::BigInt->from_hex($paged_metadata_start_lba) + Math::BigInt->from_hex($paged_metadata_size) - 1;
    my $disk;
    my $pvd_object_id;
    my $pvd_lba;
    my %pvd_info;
    my $position;
    
          
    # If R10 then inject errors into mirrors below, since the metadata for striper above is not
    # interesting.

    if ($rg_type eq 'r10')
    {
        $self->platform_shim_test_case_log_info("RG is R10. Getting end lba for underlying mirror");                   

        my @mirror_fbe_ids = @{platform_shim_raid_group_get_downstream_mirrors($rg)};   
        $rg_fbe_id = hex($mirror_fbe_ids[0]);  

        my $default_sp = platform_shim_device_get_active_sp($device, {});
        my %r0_info = %{platform_shim_raid_group_get_rg_info($default_sp, $rg_config->{raid_group_fbe_id})};
        $paged_metadata_start_lba = $r0_info{paged_metadata_start_lba};   
        $paged_metadata_size =      $r0_info{paged_metadata_size}; 
        $end_lba =  Math::BigInt->from_hex($paged_metadata_start_lba) + Math::BigInt->from_hex($paged_metadata_size) - 1;

        my @vd_ids = @{platform_shim_device_get_downstream_objects_for_fbe_object_id($self->{device}, $mirror_fbe_ids[0])};            
        my @pvd_ids = @{platform_shim_device_get_downstream_objects_for_fbe_object_id($self->{device}, $vd_ids[0])};
        $pvd_object_id = hex($pvd_ids[0]);
        $position = 0;

        $disk = metadata_get_disk_from_pvd_object_id($self->{device}, $pvd_ids[0], $rg_config->{disks});

        # Since this is a mirror,  We don't need to call maplba,  since the RG metadata start lba is the same LBA at PVD level.
        $pvd_lba = $end_lba + 0x10000;
    } 
    else
    {
        my %map = platform_shim_raid_group_map_lba($rg, $end_lba->as_hex());
        $self->platform_shim_test_case_log_info("mapLba: " . $end_lba->as_hex() . 
                                                " pba: " . $map{pba} .
                                                " data_pos: " . $map{data_pos} .
                                                " parity_pos: " . $map{parity_pos} .
                                                " chunk_index: " . $map{chunk_index} .
                                                " metadata_lba: " . $map{metadata_lba} .
                                                " original_lba: " . $map{original_lba} .
                                                " offset: " . $map{offset} );  
        #$pvd_lba = Math::BigInt->from_hex($map{pba}) - Math::BigInt->from_hex($map{offset});
        $pvd_lba = Math::BigInt->from_hex($map{pba});
        my @disks = @{$rg_config->{disks}};
        $position = $map{data_pos};
        $disk = $disks[$position];
        $pvd_object_id = platform_shim_disk_get_pvd_object_id($disk);
    }

    $self->platform_shim_test_case_log_info("Get RAID metadata end LBA. ID:$rg_id obj:" . sprintf("0x%x",$rg_fbe_id) . " type:$rg_type " .
                                            " end lba:" . $end_lba->as_hex() . " pvd lba: " . $pvd_lba->as_hex());     

    $pvd_info{disk} = $disk;
    $pvd_info{lba} = $pvd_lba;
    $pvd_info{object_id} = $pvd_object_id;
    $pvd_info{position} = $position;

    return \%pvd_info;
}


sub metadata_run_verify_on_raid_paged_errors_in_serial
{
    my ($self, $rg_configs_ref) = @_;
    my @rg_configs = @{$rg_configs_ref};    
    my $device = $self->{device};  

    $self->platform_shim_test_case_log_step("Run Verify against RAID metadata and wait for completion");

    # Verify RAID's paged metadata errors
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $rg_type = $rg_config->{raid_type};
        my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
        my $rg = $rg_config->{raid_group};
        my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba};       
        my $paged_metadata_size = $rg_config->{paged_metadata_size};  
        my %lei_stats;
        my %lei_records;
        my $sniff_passed = 0;
        my @mirror_fbe_ids;                       

        my $pvd_info_ref = metadata_get_pvd_lba_for_raid_metadata_start($self, $rg_config);
        my $disk = $pvd_info_ref->{disk};
        my $pvd_object_id = $pvd_info_ref->{object_id};
        my $pvd_lba = $pvd_info_ref->{lba};

               
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disk});                      
        
        $self->test_util_general_set_target_sp($active_sp);

        test_util_sniff_clear_verify_report($self, $disk);       

        # Note:  Only injecting on PVD active side.  If RAID is active on other side then RAID verify is not injected, so there is a 50% chance 
        # of hitting it.   This actually is results in covering more code paths. Path 1) error on Sniff, pass on RAID verify.  Path 2) error
        # on Sniff, error on RAID verify.
        #
        $self->platform_shim_test_case_log_info("Creating record on SP:".$active_sp->getProperty('name')." for PVD ". sprintf("0x%x",$pvd_object_id). 
                                                " RG [$rg_id], RAID paged metadata start lba " . $pvd_lba->as_hex());
        
        metadata_create_lei_global_record($self, $active_sp, $pvd_lba->as_hex());

        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  
=head1
        #DEBUG_TEST
        my $active_rg_sp = platform_shim_device_get_active_sp($self->{device}, {object => $rg});  
        if ($active_sp->getProperty('name') ne $active_rg_sp->getProperty('name'))
        {
            $self->platform_shim_test_case_log_info("RAID is active on alternate SP");
            metadata_create_lei_global_record($self, $active_rg_sp, $pvd_lba->as_hex());   
            $self->test_util_general_set_target_sp($active_rg_sp);
            platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id); 

            #option 2: inject into RG.

            metadata_create_lei_global_record($self, $active_rg_sp, $pvd_lba->as_hex());   
            metadata_create_lei_global_record($self, $active_rg_sp, ($pvd_lba-0x10000)->as_hex());   #todo: try both for now.
            $self->test_util_general_set_target_sp($active_rg_sp);
            platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $rg_fbe_id); 

            # change back target sp
            $self->test_util_general_set_target_sp($active_sp);         

        }
        #DEBUG_TEST END
=cut        

        # move sniff to injected area of RAID paged region. Occasionally error injection doesn't hit cp.  I think there is 
        # a race somewhere when set_checkpoint is called that it increments to next chunk.   Therefore setting to previous
        # chunk to be sure we don't miss it.
        #
        my $checkpoint = $pvd_lba-0x800;
        test_util_sniff_set_checkpoint($self, $disk, $checkpoint->as_hex()); 
        #test_util_sniff_set_checkpoint($self, $disk, $pvd_lba->as_hex());


        my $is_injected = 0;
        foreach (1..10)
        {
            %lei_stats = platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id($self->{device}, $active_sp, $pvd_object_id);
            if (int($lei_stats{error_injected_count}) > 0) 
            {
                $is_injected = 1;
                last;
            }

            $self->platform_shim_test_case_log_info("Waiting for sniff injection of RAID page" ); 

            $self->platform_shim_test_case_log_info("LEI object stats: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_stats) );            

            my %lei_global_stats = platform_shim_device_get_logical_error_injection_statistics($device, $active_sp);
            $self->platform_shim_test_case_log_info("LEI global stats: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_global_stats) );

            my %lei_records = platform_shim_device_get_logical_error_records($device, $active_sp);
            $self->platform_shim_test_case_log_info("LEI records: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_records) );

            my %hash = ();
            my %pvdinfo_hash = %{platform_shim_sp_execute_fbecli_command($active_sp, "pvdinfo -object_id ".sprintf("0x%x",$pvd_object_id), \%hash)};
            $self->platform_shim_test_case_log_info("pvdinfo :" . sprintf("0x%x",$pvd_object_id) . " -> ". Dumper(\%pvdinfo_hash) );

            my %hash2 = ();
            my %rginfo_hash = %{platform_shim_sp_execute_fbecli_command($active_sp, "rginfo -d -object_id ".sprintf("0x%x",$rg_fbe_id), \%hash2)};
            $self->platform_shim_test_case_log_info("rginfo :" . sprintf("0x%x",$rg_fbe_id) . " -> ". Dumper(\%rginfo_hash) );

            sleep 1;
        }

        if (! $is_injected)
        {
            platform_shim_test_case_throw_exception("Failed To Inject on Sniff to RAID paged.");
        }
=head1
        #DEBUG_TEST        
        if ($active_sp->getProperty('name') ne $active_rg_sp->getProperty('name'))
        {
            $is_injected = 0;
            foreach (1..100)
            {
                my %lei_stats2 = platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id($self->{device}, $active_sp, $pvd_object_id);
                $self->platform_shim_test_case_log_info("LEI status RAID: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_stats2) );
                if ($lei_stats{write_verify_block_remapped_count} > 0) 
                {
                    $is_injected = 1;
                    last;
                }
                sleep 1;
            }

            if (! $is_injected)
            {

                platform_shim_test_case_throw_exception("Failed To Inject on RAID Verify");
            }

        }
        #DEBUG_TEST END
=cut
        

        # cleanup error records so they don't collide, then re-enable
        #
        platform_shim_device_disable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  
        platform_shim_device_disable_logical_error_injection($device);   
        platform_shim_device_disable_logical_error_injection_records($device);   
        platform_shim_device_enable_logical_error_injection($self->{device});
    }

}


sub metadata_run_verify_on_pvd_paged_errors_in_parallel
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    
    my $device = $self->{device};  
    my %pvds;

    $self->platform_shim_test_case_log_step("Run Verify against PVD metadata and wait for completion");

    # Verify PVD's paged metadata errors
    foreach my $rg_config (@rg_configs) {        
        my $rg_id = $rg_config->{raid_group_id};
        my $disk = $rg_config->{disk_for_pvd_paged_errors};
        my $pvd_metadata_info = platform_shim_disk_get_pvd_metadata_info($disk);    #takes 5 seconds to make following calls into framework.
        my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');     
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disk});
                

        $self->test_util_general_set_target_sp($active_sp);

        test_util_sniff_clear_verify_report($self, $disk);

        # enabling object just before causing error injecting becuase there are multiple pvds that need injection.   If
        # we enabled them all at once then most likely the injection would occur for the wrong pvd.   Currently no way
        # to associate an error record with a given pvd. 
        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id); 

        $self->platform_shim_test_case_log_info("Injecting errors on PVD " . sprintf("0x%x",$pvd_object_id) . 
                                                " paged metadata [RG $rg_id]");            

        # move sniff to injected area of PVD paged region.
        test_util_sniff_set_checkpoint($self, $disk, $pvd_metadata_info->{paged_metadata_start_lba}); 

        $pvds{$pvd_object_id} = {'completed' => 0, 'sp' => $active_sp};
    }


    # Verify PVD's paged metadata errors.   Try for a few minutes.
    my $finished;
    foreach (1..4)  # takes about 30 seconds to do one pass to check all pvds. 
    {
        $finished = 1;
        foreach my $pvd_object_id (keys %pvds) {        
            my $active_sp = $pvds{$pvd_object_id}->{sp};
            my $is_complete = $pvds{$pvd_object_id}->{completed};
                        
            if (! $is_complete)
            {                   
                my %lei_stats = platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id($self->{device}, $active_sp, $pvd_object_id);
                $self->platform_shim_test_case_log_info("LEI status: [" . $active_sp->getProperty('name') . "] " . Dumper(\%lei_stats) );
        
                if (int($lei_stats{error_injected_count}) > 0)   
                {
                    $pvds{$pvd_object_id}->{completed} = 1;
                    $self->platform_shim_test_case_log_info("PVD " . sprintf("0x%x",$pvd_object_id) . " injected successfully");
                }
                else
                {
                    $finished = 0;
                }
            }
        }

        if ($finished)
        {
            last;
        }        

        $self->platform_shim_test_case_log_info("Still waiting for Error Injection");

        #sleep 1;
    }
    
    my @sps = platform_shim_device_get_sp($self->{device});   
    foreach my $sp (@sps) { 
        my %lei_stats = platform_shim_device_get_logical_error_injection_statistics($self->{device}, $sp);
        $self->platform_shim_test_case_log_info("LEI status (global): [" . $sp->getProperty('name') . "] " . Dumper(\%lei_stats) );
        my %lei_records = platform_shim_device_get_logical_error_records($self->{device}, $sp);
        $self->platform_shim_test_case_log_info("LEI records: [" . $sp->getProperty('name') . "] " . Dumper(\%lei_records) );
    }
       

    if (! $finished)
    {
        $self->platform_shim_test_case_log_error("Failed sniff injection on some number of PVDs");
    }
    
    #platform_shim_device_disable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);                
    
}

=head1
sub metadata_run_verify_on_raid_paged_errors_in_parallel
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    
    my $device = $self->{device};  

    $self->platform_shim_test_case_log_step("Run Verify against RAID metadata and wait for completion");

    # Verify RAID's paged metadata errors
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
        my $rg = $rg_config->{raid_group};
        my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba};       
        my $paged_metadata_size = $rg_config->{paged_metadata_size};  

        $self->platform_shim_test_case_log_info("Inject errors on RAID (ID/obj) (" . $rg_id . "/" . sprintf("0x%x",$rg_fbe_id) . ") paged metadata." .
                                                " start lba: " . $paged_metadata_start_lba . " size: " . $paged_metadata_size);     
        
        my %map = platform_shim_raid_group_map_lba($rg, $paged_metadata_start_lba);
        my @disks = @{$rg_config->{disks}};
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disks[$map{data_pos}]});        
        my $pvd_object_id = platform_shim_disk_get_property($disks[$map{data_pos}], 'fbe_id');
        
        $self->test_util_general_set_target_sp($active_sp);
        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  

        # move sniff to injected area of RAID paged region.
        test_util_sniff_set_checkpoint($self, $disks[$map{data_pos}], hex($map{pba})-hex($map{offset}));

        $pvds{$pvd_object_id} = {'completed' => 0, 'sp' => $active_sp};
    }


    # Verify RAID's paged metadata errors
    foreach my $rg_config (@rg_configs) {
        my $rg_id = $rg_config->{raid_group_id};
        my $rg_fbe_id = $rg_config->{raid_group_fbe_id};
        my $rg = $rg_config->{raid_group};
        my $paged_metadata_start_lba = $rg_config->{paged_metadata_start_lba};       
        my $paged_metadata_size = $rg_config->{paged_metadata_size};  
        my %lei_stats;
        my %lei_records;
        my $sniff_passed = 0;

        $self->platform_shim_test_case_log_info("Inject errors on RAID (ID/obj) (" . $rg_id . "/" . sprintf("0x%x",$rg_fbe_id) . ") paged metadata." .
                                                " start lba: " . $paged_metadata_start_lba . " size: " . $paged_metadata_size);     

        my %map = platform_shim_raid_group_map_lba($rg, $paged_metadata_start_lba);
        my @disks = @{$rg_config->{disks}};
        my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disks[$map{data_pos}]});        
        my $pvd_object_id = platform_shim_disk_get_property($disks[$map{data_pos}], 'fbe_id');

        $self->test_util_general_set_target_sp($active_sp);
        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  


        # move sniff to injected area of RAID paged region.
        test_util_sniff_set_checkpoint($self, $disks[$map{data_pos}], hex($map{pba})-hex($map{offset}));

        sleep 3;

        # wait until sniff passes injected area.   
        foreach (1..100)  # try for 100 seconds.
        {
            sleep 1;

            %lei_stats = platform_shim_device_get_logical_error_injection_statistics($self->{device}, $active_sp);
            $self->platform_shim_test_case_log_info("LEI status: " . Dumper(\%lei_stats) );
            %lei_records = platform_shim_device_get_logical_error_records($self->{device}, $active_sp);
            $self->platform_shim_test_case_log_info("LEI records: " . Dumper(\%lei_records) );

            #TODO: verify injection occurred.

            $sniff_passed = 1;
            last;
        }        

        if (! $sniff_passed)
        {
            $self->platform_shim_test_case_log_error("Failed to inject on sniff @ raid paged metadata lba:" . 
                                                     "$paged_metadata_start_lba" .
                                                     "pvd lba: ". sprintf("0x%x",hex($map{pba})-hex($map{offset})));
        }

        #platform_shim_device_disable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);  
    }

}
=cut


sub metadata_verify_zeroed_data($$)
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};    
    my $device = $self->{device};  

    $self->platform_shim_test_case_log_step("Verify all luns are zeroed");

    platform_shim_device_initialize_rdgen($device);

    test_util_io_check_background_pattern($self, {pattern_zeros => 1});      
    platform_shim_device_cleanup_rdgen($device);
}


# search a given disk array for a given pvd object id
sub metadata_get_disk_from_pvd_object_id($$$)
{
    my ($device, $pvd_object_id, $disk_array_ref) = @_;

    my @disks = @{$disk_array_ref};

    my @returned_disks = ();
    foreach my $d (@disks)
    {
        my $fbe_id = platform_shim_disk_get_property($d, 'fbe_id');
        if ($fbe_id == hex($pvd_object_id))
        {
            return $d;            
        }
    }

    Automatos::Exception::Base->throw("No disk found for pvd object ID: ".$pvd_object_id);
    return;
}

##########################################################################
# sub metadata_run_reboot_test
# 
# Description:  Test will corrupt the entire metadata page for one drive in each raid group.
#               It will then cause a dual reboot.  When SPs come back up the RAID groups will run
#               metadata verify and reconstruct the corrupted pages during Activate state.
#               The RAID group will then transition to Ready.
#
sub metadata_run_reboot_test
{
    my ($self, $rg_configs_ref, $rdgen_target_sp, $corruption_type) = @_;
    my @rg_configs = @{$rg_configs_ref};  

    if ($SKIP_REBOOTS)
    {
        $self->platform_shim_test_case_log_warn("Skipping Reboot Test");
        return;
    }

    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});


    $self->platform_shim_test_case_log_step("Reboot Test");

    #corrupt metadata
    #    
    foreach my $rg_config (@rg_configs)
    {
        metadata_corrupt_raid_paged_for_one_position($self, $active_sp, $rg_config, $corruption_type);
    }    
    
    $self->platform_shim_test_case_log_info("Rebooting both SPs");
    platform_shim_sp_reboot($active_sp);   
    platform_shim_sp_reboot($passive_sp);       

    $self->platform_shim_test_case_log_info("Waiting for reboot sp".platform_shim_sp_get_short_name($active_sp)); 
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    $self->platform_shim_test_case_log_info("Waiting for reboot sp".platform_shim_sp_get_short_name($passive_sp)); 
    platform_shim_sp_wait_for_agent($passive_sp, 3600);
    $self->platform_shim_test_case_log_info("SPs are back"); 

    
    # Wait for metadata verify.  When RG goes Ready we know it's done.
    foreach my $rg_config (@rg_configs)
    {
        my $rg = $rg_config->{raid_group};
        $self->platform_shim_test_case_log_info("Waiting for RG " . sprintf("0x%x",$rg->getProperty('fbe_id')) . " to come Ready"); 
        platform_shim_raid_group_wait_for_ready_state($device, [$rg]);  
    }

    # verify pattern
    #
    # always make sure RDGEN verifys on same SP it wrote pattern.   
    test_util_general_set_target_sp($self, $rdgen_target_sp);    

    platform_shim_device_initialize_rdgen($device);
    metadata_verify_pattern($self);
}

##########################################################################
# sub metadata_run_verify_test
# 
# Description:  Test will corrupt the entire metadata page for one drive in each raid group.
#               It will then cause a rebuild by removing another drive in the raid group.
#               The exception is that for R0 we simply check for RG going failed and then
#               back to Ready after its metadata is rebuilt.
#
sub metadata_run_rebuild_test
{
    my ($self, $rg_configs_ref, $rdgen_target_sp, $corruption_type) = @_;
    my @rg_configs = @{$rg_configs_ref};  

    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    my $corrupted_position = -1;
    my $fail_pos = -1;

    $self->platform_shim_test_case_log_step("Rebuild Test");

    #corrupt metadata
    #    
    foreach my $rg_config (@rg_configs)
    {
        my $rg = $rg_config->{raid_group};
        my $rg_width = $rg->getProperty('raid_group_width');

        $corrupted_position = metadata_corrupt_raid_paged_for_one_position($self, $active_sp, $rg_config, $corruption_type);
        
        foreach my $pos (0..$rg_width-1)
        {
            if ($pos != $corrupted_position)
            {
                $fail_pos = $pos;
                last;
            }
        }

        #initiate a rebuild       
        $rg_config->{removed_disk} = $rg_config->{disks}[$fail_pos];
        $rg_config->{disk_fault} = platform_shim_disk_remove($rg_config->{disks}[$fail_pos]);

        $self->platform_shim_test_case_log_info("Removing drive to Initiating rebuild logging for RG " . sprintf("0x%x",$rg->getProperty('fbe_id')) .
                                                ". width:$rg_width, corrupt_pos:$corrupted_position, fail_pos:$fail_pos"); 
    }    
    
    # Wait for rebuild logging to kick off. Then re-insert drive 
    foreach my $rg_config (@rg_configs)
    {
        my $rg = $rg_config->{raid_group};
        my $rg_type = $rg_config->{raid_type};        

        platform_shim_disk_wait_for_disk_removed($device, [$rg_config->{removed_disk}]);

        # skip r0 since there's no rebuild logging
        if ($rg_type eq 'r0')
        {
            $self->platform_shim_test_case_log_info("Skipping since type is R0.  Verifying RG is Failed ");
            platform_shim_raid_group_wait_for_failed_state($device, [$rg]);
        }
        else
        {                       
            $self->platform_shim_test_case_log_info('Verify that rebuild logging starts for raid group ' . $rg_config->{raid_group_id});                    
            platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                               raid_group => $rg_config->{raid_group}});
        }
    }
        
    #restore faults.
    foreach my $rg_config (@rg_configs)
    {
        my $disk_fault = $rg_config->{disk_fault};
        $disk_fault->recover();
    }

    
    # Wait for Rebuild.
    foreach my $rg_config (@rg_configs)
    {        
        my $rg = $rg_config->{raid_group};
        my $rg_type = $rg_config->{raid_type}; 

        $self->platform_shim_test_case_log_info("Waiting for rebuild of RG " . $rg_config->{raid_group_id} . 
                                                " (" . sprintf("0x%x",$rg->getProperty('fbe_id')) . ")" ); 

        # skip r0 since there's no rebuild logging
        if ($rg_type eq 'r0')
        {
            $self->platform_shim_test_case_log_info("Skipping since type is R0.  Waiting for RG to go Ready ");
            platform_shim_raid_group_wait_for_ready_state($device,[$rg]);  
        }
        else
        {
            $self->platform_shim_test_case_log_info("Verify that rebuild started");
            platform_shim_event_verify_rebuild_started({object => $device, 
                                                       raid_group => $rg_config->{raid_group}});
    
            $self->platform_shim_test_case_log_info("Recover disk faults");
            platform_shim_device_recover_from_faults($device);
    
            $self->platform_shim_test_case_log_info("Wait for rebuild to complete ");
            test_util_rebuild_wait_for_rebuild_complete($self, $rg_config->{raid_group}, 3600);
    
            $self->platform_shim_test_case_log_info("Rebuild is complete.");
        }

    }

    # verify pattern
    #
    # always make sure RDGEN verifys on same SP it wrote pattern.   
    test_util_general_set_target_sp($self, $rdgen_target_sp);    

    platform_shim_device_initialize_rdgen($device);
    metadata_verify_pattern($self);
}


##########################################################################
# sub metadata_run_verify_test
# 
# Description:  Test will corrupt the entire metadata page for one drive in each raid group.
#               It will then issue a background verify.  The verify will detect the corrupted
#               metadata pages and reconstruct them.
#
sub metadata_run_verify_test
{    
    my ($self, $rg_configs_ref, $rdgen_target_sp, $corruption_type) = @_;

    my @rg_configs = @{$rg_configs_ref};  
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});

    $self->platform_shim_test_case_log_step("Background Verify Test");


    foreach my $rg_config (@rg_configs)
    {
        my $rg = $rg_config->{raid_group};
        my $rg_width = $rg->getProperty('raid_group_width');

        #corrupt paged metadata
        my $corrupted_position = metadata_corrupt_raid_paged_for_one_position($self, $active_sp, $rg_config, $corruption_type);

        $self->platform_shim_test_case_log_info("Initiating Background Verify for RG " . sprintf("0x%x",$rg->getProperty('fbe_id')));   

        # Initiate a background verify
        foreach my $lun (@{$rg_config->{luns}})
        {
            platform_shim_lun_clear_bv_report($lun);
            platform_shim_lun_initiate_rw_background_verify($lun);
        }
    }   
     
        
    foreach my $rg_config (@rg_configs)
    {
        my $rg = $rg_config->{raid_group};
        
        foreach my $lun (@{$rg_config->{luns}})
        {
            $self->platform_shim_test_case_log_info("Waiting for Background Verify for RG " . sprintf("0x%x",$rg->getProperty('fbe_id')) .
                                                    "lun: " . $lun->getProperty('id'));   
            test_util_verify_wait_for_lun_rw_bv_complete($self, $lun, 3600);
        }
    }


    # verify pattern
    #
    # always make sure RDGEN verifys on same SP it wrote pattern.   
    test_util_general_set_target_sp($self, $rdgen_target_sp);    

    platform_shim_device_initialize_rdgen($device);
    metadata_verify_pattern($self);
}



#corrupts the paged area for one drive.
sub metadata_corrupt_raid_paged_for_one_position
{
    my ($self, $active_sp, $rg_config, $corruption_type) = @_;

    my $pvd_info_start_ref = metadata_get_pvd_lba_for_raid_metadata_start($self, $rg_config);
    my $pvd_object_id = $pvd_info_start_ref->{object_id};
    my $pvd_start_lba = $pvd_info_start_ref->{lba};
    my $position = $pvd_info_start_ref->{position};

    my $pvd_info_end_ref = metadata_get_pvd_lba_for_raid_metadata_end($self, $rg_config);
    my $pvd_last_lba = $pvd_info_end_ref->{lba};

    my $blocks = $pvd_last_lba - $pvd_start_lba;

    # Zero out page, including checksum, to corrupt it and cause multibit crc. 
    # todo: Other patterns?
    #        

    my $cmd;
    if ($corruption_type == $CORRUPTION_TYPE_ZEROS)
    {
        #todo:  add zeros and coh pattern to shim/utils
        $cmd = "rdgen -object_id " . sprintf("0x%x",$pvd_object_id) .  
                  " -package_id sep -start_lba " . $pvd_start_lba->as_hex() .  
                  " -min_lba ". $pvd_start_lba->as_hex() . 
                  " -max_lba ". $pvd_last_lba->as_hex() .
                  " -constant -seq -zeros -pass_count 1 w * 1 80";
    }
    elsif($corruption_type == $CORRUPTION_TYPE_COH_ERRORS)
    {
        $cmd = "rdgen -object_id " . sprintf("0x%x",$pvd_object_id) .  
                  " -package_id sep -start_lba " . $pvd_start_lba->as_hex() .  
                  " -min_lba ". $pvd_start_lba->as_hex() . 
                  " -max_lba ". $pvd_last_lba->as_hex() .
                  " -constant -seq -pass_count 1 o * 1 80";
    }
    else
    {
        platform_shim_test_case_throw_exception("Unsupported Corruption Type: $corruption_type");
    }

    my %hash;
    platform_shim_sp_execute_fbecli_command($active_sp, $cmd, \%hash);        

    return $position;
}


sub metadata_write_pattern
{
    my ($self) = @_;
    test_util_io_write_background_pattern($self);
}

sub metadata_verify_pattern
{
    my ($self) = @_;
    test_util_io_check_background_pattern($self);
}

sub metadata_cleanup
{
    my $self = shift;
    my $device = $self->{device};
  
    $self->platform_shim_test_case_log_step(__FUNCTION__());

    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});

    # disable logical error injection
    platform_shim_device_disable_logical_error_injection($device);   # first cleanup active side from previous run
    platform_shim_device_disable_logical_error_injection_records($device);   
    test_util_general_set_target_sp($self, $passive_sp);
    platform_shim_device_disable_logical_error_injection($device);   # next cleanup passive side from previous run
    platform_shim_device_disable_logical_error_injection_records($device);       
    test_util_general_set_target_sp($self, $active_sp);

    # enable automatic sparing
    platform_shim_device_enable_automatic_sparing($device);

    # restore load balancing
    platform_shim_device_enable_load_balancing($device);    
}

sub metadata_wait_for_any_disk_zeroed
{
    my ($self, $disk_array_ref, $pvd_info_hash_ref, $timeout) = @_;

    my @disks = @{$disk_array_ref};
    my %pvd_info = %{$pvd_info_hash_ref};
    
    my $wait_until = time() + $timeout;

    my @beds =();
    while (time() < $wait_until)
    {
        foreach my $disk (@disks)
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            if (not exists $pvd_info{$disk_id} )
            {
                platform_shim_test_case_throw_exception("Disk $disk_id doesn't exist in pvd info. Probably bug in parsing code");
            }
            if ($pvd_info{$disk_id}{zero_percent} == 100)
            {
                $self->platform_shim_test_case_log_info("Disk $disk_id finished zeroing.");
                return $disk;
            }
        }
        sleep(10); # sleep for a long time, zeroing takes awhile. 
    }
    
    platform_shim_test_case_throw_exception("No disks in set finished zeroing within $timeout seconds");    
}


sub __FUNCTION__
{
    return (caller(1))[3];
}
1;


