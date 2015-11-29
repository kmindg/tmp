package mcr_test::power_savings::power_savings;

use base qw(mcr_test::platform_shim::platform_shim_test_case);

use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_power_savings;
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


# ==================Power Savings Test Cases===========================================
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

    $self->platform_shim_test_case_add_to_cleanup_stack({'power_savings_cleanup' => {}});                     

    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    test_util_general_set_target_sp($self, $active_sp);
    my $rdgen_target_sp = $active_sp;

    if ($self->platform_shim_test_case_get_parameter(name => 'clean_config') == '1')
    {
        # make sure we are always starting with a sane config.  e.g. no lei records left over from last test.
        power_savings_cleanup($self);  
    }

    # Step 1: Based on the platform under test configure the following:
    #           o $luns_per_rg
    #           o $lun_capacity_gb
    #           o $lun_io_capacity_percent
    #           o $lun_io_max_lba
    $self->platform_shim_test_case_log_step("Configure test parameters (luns per rg, lun io percent, etc) based on platform");
    power_savings_configure_params_based_on_platform($self, \$luns_per_rg, \$lun_capacity_gb);


    # Set up test cases for each raid group
    $self->platform_shim_test_case_log_step("Create raid groups");
    my @rg_configs = @{power_savings_initialize_rg_configs($self, $luns_per_rg, $lun_capacity_gb)};
    power_savings_bind_luns($self, \@rg_configs);


    # Make sure test is initialized with default of PS being disabled.
    test_util_power_savings_set_system_state_disabled($device);

    # initialize rdgen
    platform_shim_device_initialize_rdgen($device);

    # Test power savings on spares.
    power_savings_test_spare_disks($self, $device, $active_sp, $passive_sp);

    # Test power savings on RGs.
    #   
    power_savings_test_raid_groups($self, $device, $active_sp, $passive_sp, \@rg_configs);

    if ($self->platform_shim_test_case_is_functional())
    {            
        #Currently functional doesn't do anything different than promotional, 
        # other than limits number of RGs
    }

    platform_shim_device_cleanup_rdgen($device);

    test_util_general_log_test_time($self);
}

sub power_savings_send_one_read_io
{
    my ($sp, $object_id) = @_;

    #rdt -d -object_id 0x13c -package_id sep r 0x15 0 1
    my $cmd = "rdt -object_id " . sprintf("0x%x",$object_id) .  
              " -package_id sep r 0x15 0 1";

    my %hash;
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}

sub power_savings_test_spare_disks
{
    my ($self, $device, $active_sp, $passive_sp) = @_;

    $self->platform_shim_test_case_log_step("Verify spare drives powersavings");    
    my $disk_is_spindown_verified = 0;
    my @tech = ('normal');
    my %disk_params = (vault => 0, state => "unused", technology => \@tech);
    my @disks = platform_shim_device_get_disk($device, \%disk_params);
    foreach my $disk (@disks)
    {
        if (test_util_power_savings_is_disk_capable($active_sp, $disk))
        {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');   
            my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');
            $self->platform_shim_test_case_log_info("Verifying DISK:$disk_id");

            $self->platform_shim_test_case_log_info("Sending IO to reset idle timer. Disk:$disk_id pvd:".sprintf("0x%x",$pvd_object_id));
            power_savings_send_one_read_io($active_sp, $pvd_object_id);
            test_util_power_savings_set_system_state_enabled($device);

            #first verify it's not saving.
            if (test_util_power_savings_is_disk_saving($active_sp, $disk))
            {
                platform_shim_test_case_throw_exception("Failure: DISK:$disk_id is saving power before being enabled. ".
                                                        "Most likely system PS was not disabled before test started");
            }

            # Enable on both sides.  This is the default state, but do it just to be sure.
            test_util_power_savings_set_disk_state_enable($active_sp, $disk);
            test_util_power_savings_set_disk_state_enable($passive_sp, $disk);
            platform_shim_disk_power_savings_set_idle_time($active_sp, $disk, "30 S");
            platform_shim_disk_power_savings_set_idle_time($passive_sp, $disk, "30 S");

            sleep(45);

            #verify hibernating
            if (test_util_power_savings_is_disk_saving($active_sp, $disk))
            {
                $self->platform_shim_test_case_log_info("Verified DISK:$disk_id is in power saving mode");
            }
            else
            {
                platform_shim_test_case_throw_exception("Failure: DISK:$disk_id is not saving power.");
            }

            $disk_is_spindown_verified = 1;
            test_util_power_savings_set_system_state_disabled($device);
            last;
        }
    }
}

sub power_savings_test_raid_groups
{
    my ($self, $device, $active_sp, $passive_sp, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};

    $self->platform_shim_test_case_log_step("Verify Raid Group powersavings");    

    test_util_power_savings_set_system_state_enabled($device);

    foreach my $rg_config (@rg_configs) {        
        my $rg_id = $rg_config->{raid_group_id};
        my $rg = $rg_config->{raid_group};
        my @disks = @{$rg_config->{disks}};

        $rg_config->{is_ps_capable} = 1;

        $self->platform_shim_test_case_log_info("Testing RG:$rg_id");

        foreach my $disk (@disks)
        {            
            if (test_util_power_savings_is_disk_capable($active_sp, $disk))
            {
                # Enable on both sides.  This is the default state, but do it just to be sure.
                test_util_power_savings_set_disk_state_enable($active_sp, $disk);
                test_util_power_savings_set_disk_state_enable($passive_sp, $disk);
                platform_shim_disk_power_savings_set_idle_time($active_sp, $disk, "30 S");
                platform_shim_disk_power_savings_set_idle_time($passive_sp, $disk, "30 S");
            }
            else
            {
                $rg_config->{is_ps_capable} = 0;
                my $disk_id = platform_shim_disk_get_property($disk, 'id');   
                $self->platform_shim_test_case_log_info("Disk not PS capable. RG:$rg_id Disk:$disk_id");
                last;
            }
        }

        if ($rg_config->{is_ps_capable})
        {
=head1
            my @luns = @{$rg_config->{luns}};
            my $lun_object_id = platform_shim_lun_get_property($luns[0], 'fbe_id');
            $self->platform_shim_test_case_log_info("Sending IO to reset idle timer. RG:$rg_id lun:".sprintf("0x%x",$lun_object_id));
            power_savings_send_one_read_io($active_sp, $lun_object_id);

            # first verify it's not already saving
            if (platform_shim_raid_group_power_savings_is_saving($rg))
            {
                platform_shim_test_case_throw_exception("Failure: RG:$rg_id is already saving power. ".
                                                        "Most likey PS wasn't disabled before test started");                
            }
=cut
        }
        else
        {
            #todo: add ps option to RG create utility
            $self->platform_shim_test_case_log_info("RG not PS capable because one or more drives not capable. Skipping RG:$rg_id");
            next;
        }

        test_util_power_savings_set_rg_enabled($rg);
        test_util_power_savings_set_rg_idle_time($rg, "30 S");
    }

    $self->platform_shim_test_case_log_info("DEBUG: rg_configs:" . Dumper(\@rg_configs) );

    sleep(45);

    # verify RGs are hibernating
    #
    foreach my $rg_config (@rg_configs) {        
        my $rg_id = $rg_config->{raid_group_id};
        my $rg = $rg_config->{raid_group};        

        if ($rg_config->{is_ps_capable} == 1)
        {        
            if (platform_shim_raid_group_power_savings_is_saving($rg))
            {
                $self->platform_shim_test_case_log_info("Verified RG:$rg_id is in power saving mode");
            }
            else
            {
                platform_shim_test_case_throw_exception("Failure: RG:$rg_id is not saving power");
            }
        }
        elsif ($rg_config->{is_ps_capable} != 0)
        {
            platform_shim_test_case_throw_exception("Failure: is_ps_capable flag not defined");
        }
    }
    test_util_power_savings_set_system_state_disabled($device);
}

sub power_savings_configure_params_based_on_platform
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




sub power_savings_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_gb) = @_;
   my @rgs;
   my $device = $self->{device};

   
   if ($self->platform_shim_test_case_get_parameter(name => 'clean_config') == '1')
   {
       $self->platform_shim_test_case_log_step("Step: Initialize raid group configs from array of raid groups");

       if ($self->platform_shim_test_case_is_promotion()) {          
           @rgs = @{test_util_configuration_create_one_random_raid_group($self, {b_spindown_capable_only => 1})};
       } else {   #functional           
           @rgs = @{test_util_configuration_create_all_raid_types($self, {b_spindown_capable_only => 1})};
       }
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


   $self->platform_shim_test_case_log_info("Initialize raid group configs from array of raid groups");
   my @rg_configs = ();
   my $rg_index = 0;
   my $tested_empty_rg_copy = 0;

   foreach my $rg (@rgs) {
      my $rg_id = platform_shim_raid_group_get_property($rg, 'id'); 
      my $raid_type = platform_shim_raid_group_get_property($rg, 'raid_type');
      my $width = platform_shim_raid_group_get_property($rg, 'raid_group_width');
      my @disks = platform_shim_raid_group_get_disk($rg);
            
      # Setup the test case   
      my %rg_config = ();
      $rg_config{disks} = \@disks;
      $rg_config{raid_group_id} = $rg_id;
      $rg_config{raid_group} = $rg;
      $rg_config{luns_per_rg} = $luns_per_rg;
      $rg_config{lun_capacity_gb} = $lun_capacity_gb;
      $rg_config{width} = $width;
      $rg_config{raid_type} = $raid_type;

      $self->platform_shim_test_case_log_info('Raid Group ' . $rg_config{raid_group_id} . 
                                              ' Raid Type ' . $rg_config{raid_type} . 
                                              ' Width ' . $rg_config{width});
      # Push the test case
      push @rg_configs, \%rg_config;
   }
   return \@rg_configs;
}

sub power_savings_bind_luns
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
            $self->platform_shim_test_case_log_error("No existing luns to use.  RAID ID: ". $rg_id);
        }

        $rg_config->{luns} = \@luns;
    }
}


sub power_savings_cleanup
{
    my $self = shift;
    my $device = $self->{device};
  
    $self->platform_shim_test_case_log_step(__FUNCTION__());

    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});

    test_util_power_savings_set_system_state_disabled($device);
}


sub __FUNCTION__
{
    return (caller(1))[3];
}
1;

