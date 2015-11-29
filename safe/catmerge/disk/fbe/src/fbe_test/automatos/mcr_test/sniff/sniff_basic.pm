package mcr_test::sniff::sniff_basic;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_sniff;
use mcr_test::test_util::test_util_rebuild;
use mcr_test::test_util::test_util_dest;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use strict;
use warnings;
use Data::Dumper;



=head1 Test Case Sniff Verify

MCR_Test::Promotion::SV::SV_TC::SV_TC_Basic_Sniff_Verify


=cut

=for

Method: main
Performs all actions to be executed in the test case

Input:
*None*

Returns:
*None*

Exceptions:
*None*

Debugging Flags:
*None*

=cut


# Vairable to test if we need test on system drives
our $test_on_system_drives = 1;
my  $SKIP_REBOOTS = 0;
my  $g_use_lei = 0;   #Using DEST instead because LEI object injection since LEI doesn't seem to be working.   Maybe I just didn't set it up correctly. 

# DEBUGGING OPTIONS.  THE PROMOTED CODE SHOULD ALWAYS BE 0
#
my $DEBUG_CREATE_ONLY_ONE_RG = 0;
my $DEBUG_ENABLE_PVD_TRACING = 0;



# Check if we can run test on system drives
sub sv_check_system_drives($)
{
     my ($self) = @_;
    my $device = $self->{device};
    my @system_disks = platform_shim_disk_get_current_system_disks($self);
    my $num_disks = scalar(@system_disks);

    if ($num_disks < 4) {
        $self->platform_shim_test_case_log_warn("Number of system drives is $num_disks, will skip system drives test");
        $test_on_system_drives = 0;
        return;
    }
    for my $disk (@system_disks) {
        my $eol = platform_shim_disk_get_eol_state($disk);
        my $system_disk_min_gb = (platform_shim_disk_get_property($disk, "capacity")*512) / (1024*1024*1024);
        my $zero_percent = int(platform_shim_disk_get_property($disk, 'zero_percent'));

    	if ($zero_percent < 100) {
           $test_on_system_drives = 0;   
           $self->platform_shim_test_case_log_info("Zeroing in progress on system drives, will skip system drives test");
           return;
   		 }
        
         # If the system disk are 300GB or less then disable the use of system disks 
         if (!platform_shim_device_is_unified($device)  &&
             ($system_disk_min_gb <= 300)                 ) {
            $self->platform_shim_test_case_log_info("Rockies system disk capacity: $system_disk_min_gb GB too small for user rg");
            $test_on_system_drives = 0;
            return;
        }
        
        if ($eol != "0" ) {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_warn("Eol stat of $disk_id: '$eol', will skip system drives test");
            $test_on_system_drives = 0;
            return;
        }
        
        
    }
    
   
    $self->platform_shim_test_case_log_info("Will include system drive test");
}


sub sv_random_select($)
{
    my ($array_ref) = @_;
    my @array = @$array_ref;
    my $array_len = scalar(@array);

    if ($array_len <= 0) {
        return undef;
    }
    return $array[int(rand($array_len))];
}

sub sv_select_drive_for_sniff_from_array($$)
{
    my ($self, $disk_array_ref) = @_;
    my $disk = test_util_sniff_select_disk_for_sniff_from_array($self, $disk_array_ref);

    if (! $disk) {
        return undef;
    }
    my $zero_percent = int(platform_shim_disk_get_property($disk, 'zero_percent'));

    if ($zero_percent == 100) {
        return $disk;
    }
    if (platform_shim_test_case_is_promotion($self)) {
        # We can't afford the time for waiting zeroing in promotion test.
        my $disk_id = platform_shim_disk_get_property($disk, 'id');
        platform_shim_test_case_log_warn("Zero percent is $zero_percent for disk $disk_id, will skip the testcase");
        return undef;
    }
    return $disk;
}

sub sv_select_non_system_disk_for_sniff($)
{
    my $tc = shift;
    my $device = $tc->{device};
    my %disk_params = (vault => 0, technology => ['normal'], state =>"unused",);
    my @disks = platform_shim_device_get_disk($device, \%disk_params);

    my $chosen = sv_select_drive_for_sniff_from_array($tc, \@disks);

    return $chosen;
}

sub sv_select_system_disk_for_sniff($)
{
    my $tc = shift;
    my $device = $tc->{device};
    my %disk_params = (vault => 1, technology => ['normal']);
    my @disks = platform_shim_device_get_disk($device, \%disk_params);
    my $chosen = sv_select_drive_for_sniff_from_array($tc, \@disks);

    return $chosen;
}

# Create raid group for sniff verify test.
# Will 
#    1. Create all raid types on non-system drives
#    2. Create a r5 raid group on system drives if need test on system drives
# 
# @parameter:
#    $1: The test config
# @return:
#    The array of raid groups
# @version:
#    2014-08-13. Created. Jamin Kang
sub sv_create_raid_groups
{
    my ($self) = @_;
    my $device = $self->{device};
    my @rgs = ();

    if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {
        # 9/15/2014.  We need to use "unused" so the framework does not try to select removed drives.
        # this code seems to be buggy in the framework as we sometimes hang in the framework with removed drives.
        my @tech = ('normal', 'flash');
        my %disk_params = (vault => 0, state => "unused", technology => \@tech);
        my @disks = platform_shim_device_get_disk($device, \%disk_params);

        if (platform_shim_test_case_is_promotion($self)) {
            my $params = {
                min_raid_groups => 1,
                max_raid_groups => 1,
                raid_types => ['r5'],
                disks => \@disks,
                b_zeroed_only => 1,
            };
            @rgs = @{test_util_configuration_create_random_config($self, $params)};
        } else {
            # Pop out a disk so we have at least 1 unbound drive for test.
            pop(@disks);
            my $params = {
                disks => \@disks,
                b_zeroed_only => 1,
                spares => 0,
            };
            if ($DEBUG_CREATE_ONLY_ONE_RG){
                @rgs = @{test_util_configuration_create_one_random_raid_group($self, $params)};
            }
            else{
                @rgs = @{test_util_configuration_create_all_raid_types($self, $params)};
            }
        }

        if ($test_on_system_drives) {
            my @system_disks = platform_shim_device_get_system_disk($device);
            $self->platform_shim_test_case_log_info("Create system raid group with " .
                                                    scalar(@system_disks) . " drives");
            my $rg = test_util_configuration_create_raid_group($self,
                                                               {disks => \@system_disks, raid_type => 'r5'});
            push @rgs, $rg;
        }
    } else {
        my @current_rgs = platform_shim_device_get_raid_group($device, {});
        if ($test_on_system_drives) {
            @rgs = @current_rgs;
        } else {
            for my $rg (@current_rgs) {
                if (!sv_is_raid_group_on_system_drive($self, $rg)) {
                    push(@rgs, $rg);
                }
            }
        }
    }
    my $rg_num = scalar(@rgs);
    if ($rg_num <= 0) {
        platform_shim_test_case_throw_exception("No raid groups");
        return
    }
    $self->platform_shim_test_case_log_info(
        "Doing SV test on $rg_num raid groups, on system dirve: $test_on_system_drives");
    return @rgs
}

sub sv_is_raid_group_on_system_drive($$)
{
    my ($self, $rg_ref) = @_;
    my @disks = platform_shim_raid_group_get_disk($rg_ref);

    foreach my $disk (@disks) {
        my $id = platform_shim_disk_get_property($disk, 'id');

        if ($id =~ /0_0_[0-3]$/) {
            $self->platform_shim_test_case_log_info("This rg is system: ". Dumper($rg_ref));
            return 1;
        }
    }
    return 0;
}


sub sv_disable_automatic_sparing
{
    my ($self) = @_;
    my $device = $self->{device};

    $self->platform_shim_test_case_log_info("Disable automatic sparing");
    platform_shim_device_disable_automatic_sparing($device);
}

sub sv_enable_automatic_sparing
{
    my ($self) = @_;
    my $device = $self->{device};

    $self->platform_shim_test_case_log_info("Enable automatic sparing");
    platform_shim_device_enable_automatic_sparing($device);
}

sub sv_create_lun($$)
{
    my ($self, $rg) = @_;
    my $lun_cap_mb = 100;
    my $lun;
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
    $self->platform_shim_test_case_log_info("Creating LUN for Raid group $rg_id");
    $lun = test_util_configuration_create_lun_size($self, $rg, $lun_cap_mb . "MB");
    my $lun_id = platform_shim_lun_get_property($lun, 'id');
    $self->platform_shim_test_case_log_info("Created LUN $lun_id for Raid group $rg_id");
    return $lun;
}

sub sv_remove_lun($$)
{
    my ($self, $rg) = @_;
    my @luns = platform_shim_raid_group_get_lun($rg);
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
    my $lun_ids = join(', ', map {platform_shim_lun_get_property($_, 'id') } @luns);
    $self->platform_shim_test_case_log_info("Remove [$lun_ids] for raid $rg_id");

    foreach my $lun (@luns) {
        my $lun_id = platform_shim_lun_get_property($lun, 'id');
        $self->platform_shim_test_case_log_info("Removing LUN $lun_id");
        platform_shim_lun_unbind($lun);
    }
}

sub sv_get_system_lun_object_id($)
{
    my ($self) = @_;
    my $sp = platform_shim_device_get_target_sp($self->{device});
    my @luns_object_id = platform_shim_sp_discover_sep_objects($sp, "LUN");
    my @sys_luns_object_id = grep {platform_shim_sp_is_system_object_fast($sp, $_) } @luns_object_id;
    my $lun_number = scalar(@sys_luns_object_id);
    $self->platform_shim_test_case_log_info("Get $lun_number system Luns");

    return sort(@sys_luns_object_id);
}


=head1

    setup test cases for sniff verify
    @param:
        $1: test config

    @return:
        all test cases

=cut
sub setup_config
{
    my ($self) = @_;
    my @rgs = ();
    my @test_cases = ();

    sv_check_system_drives($self);
    @rgs = sv_create_raid_groups($self);
    foreach my $rg (@rgs) {
        my %test_case = ();
        my $lun;
        if ($self->platform_shim_test_case_get_parameter(name => "clean_config")) {
            if (! sv_is_raid_group_on_system_drive($self, $rg)) {
                $lun = sv_create_lun($self, $rg);
            }
        } else {
            my @rg_luns = platform_shim_raid_group_get_lun($rg);
            $lun = $rg_luns[0];
        }
        my @disks = platform_shim_raid_group_get_disk($rg);

        $test_case{raid_group} = $rg;
        $test_case{raid_group_id} = platform_shim_raid_group_get_property($rg, 'id');
        $test_case{raid_type} = platform_shim_raid_group_get_property($rg, 'raid_type');
        $test_case{lun} = $lun;        
        push(@{$test_case{disks}}, @disks);
        $test_case{fault_disks} = [];

        push @test_cases, \%test_case;
    }
    return \@test_cases;
}

sub sv_get_all_test_disks($$)
{
    my ($self, $test_cases_ref) = @_;
    my @all_disks;

    for my $test_case (@{$test_cases_ref}) {
        for my $disk (@{$test_case->{disks}}) {
            # SV is enabled only on spindle.
            push @all_disks, $disk
        }
    }
    return @all_disks;
}

sub sv_get_all_test_disks_for_fast_test($$)
{
    my ($self, $test_cases_ref) = @_;
    my @all_disks;

    for my $test_case (@{$test_cases_ref}) {
        for my $disk (@{$test_case->{disks}}) {
            # SV is enabled only on spindle.
            push(@all_disks, $disk);
            last;
        }
    }
    return @all_disks;
}

# Check if sniff verify is enabled.
# 
# @parameter:
#    $1: The test config
# @return:
#    None
#
# @version:
#    2014-08-13. Created. Jamin Kang
sub sv_check_enabled
{
    my ($self) = @_;
    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    my $active_name = platform_shim_sp_get_property($active_sp, "name");
    my $passive_name = platform_shim_sp_get_property($passive_sp, "name");

    $self->platform_shim_test_case_log_info("Check if SV enabled on both SPs. Active: $active_name, passive: $passive_name");
    my $is_active_enabled = platform_shim_sp_is_sniff_verify_enabled($active_sp);

    $self->platform_shim_test_case_assert_true($is_active_enabled, "SV isn't eanbled on SP: $active_name");
    my $is_passive_enabled = platform_shim_sp_is_sniff_verify_enabled($passive_sp);
    $self->platform_shim_test_case_assert_true($is_passive_enabled, "SV isn't eanbled on SP: $passive_name");
}

=head1

    Wait for all disks/luns to finish zeroing
    @param:
        $1: test config
        $2: test cases

    @return:
        None

=cut
sub sv_wait_for_zero
{
    my ($self, $test_cases_ref) = @_;
    my $timeout = 86400;

    for my $test_case (@{$test_cases_ref}) {
        for my $disk (@{$test_case->{disks}}) {
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $self->platform_shim_test_case_log_info("Verify zeroing completes on disk $disk_id");
            test_util_zeroing_wait_for_disk_zeroing_percent($self, $disk, 100, $timeout);
        }
        my $lun = $test_case->{lun};
        if (defined $lun) {
            my $lun_id = platform_shim_lun_get_property($lun, 'id');
            my $zeroed_timeout = 10 * 60;
            $self->platform_shim_test_case_log_info("Verify zeroing completes on LUN $lun_id");
            test_util_zeroing_wait_for_lun_zeroing_percent($self, $lun, 100, $zeroed_timeout);
        }
    }
}

sub sv_check_sniff_verify_update_on_drives
{
    my ($self, $drives_ref) = @_;
    my @all_disks = @$drives_ref;
    my @all_checkpoints;
    my $sv_timeout = 3 * 60;

    $self->platform_shim_test_case_log_info("Verify checkpoint update");
    for my $disk (@all_disks) {
        my $checkpoint = test_util_sniff_get_checkpoint($self, $disk);
        push @all_checkpoints, $checkpoint;
    }
    for (my $index = 0; $index < scalar(@all_disks); $index += 1) {
        test_util_sniff_wait_for_checkpoint_update($self,
                                                   $all_disks[$index],
                                                   $all_checkpoints[$index],
                                                   $sv_timeout);
    }
}

=head1

    Verify that sniff checkpoint is updated
    @param:
        $1: test config
        $2: test cases

    @return:
        None

=cut
sub sv_check_sniff_verify_update
{
    my ($self, $test_cases_ref) = @_;
    my @all_disks = sv_get_all_test_disks($self, $test_cases_ref);
    my @test_disks;

    # random select a disks to speed up test
    push(@test_disks, sv_random_select(\@all_disks));
    sv_check_sniff_verify_update_on_drives($self, \@test_disks);
}

sub sv_check_sniff_verify_stop
{
    my ($self, $test_cases_ref) = @_;

    my @all_disks = sv_get_all_test_disks_for_fast_test($self, $test_cases_ref);
    my @prev_checkpoints;
    my @cur_checkpoints;
    my $sv_timeout = 10;

    $self->platform_shim_test_case_log_info("Verify checkpoint stop");
    for my $disk (@all_disks) {
        my $checkpoint = test_util_sniff_get_checkpoint($self, $disk);
        push @prev_checkpoints, $checkpoint;
    }
    $self->platform_shim_test_case_log_info("Sleep $sv_timeout seconds before check");
    sleep($sv_timeout);
    $self->platform_shim_test_case_log_info("Get new checkpoint");
    for my $disk (@all_disks) {
        my $checkpoint = test_util_sniff_get_checkpoint($self, $disk);
        push @cur_checkpoints, $checkpoint;
    }
    for (my $i = 0; $i < scalar(@prev_checkpoints); $i += 1) {
        my $prev = $prev_checkpoints[$i];
        my $cur = $cur_checkpoints[$i];
        my $disk = $all_disks[$i];
        my $disk_id = platform_shim_disk_get_property($disk, 'id');

        $self->platform_shim_test_case_log_info("SV: Check for disk $disk_id");
        $self->platform_shim_test_case_assert_true_msg(
            $prev eq $cur,
            "SV: Unexpected checkpoint update from $prev to $cur, disk: $disk_id");
    }
}


sub sv_wait_for_rebuild_complete
{
    my ($self, $test_case_ref) = @_;

    $self->platform_shim_test_case_log_info("SV: Wait for All raid group to complete rebuild");
    for my $test_case (@{$test_case_ref}) {
        my $rg = $test_case->{raid_group};
        test_util_rebuild_wait_for_rebuild_complete($self, $rg, 3600);
    }
}


sub sv_wait_for_background_service
{
    my ($self, $test_case_ref) = @_;

    sv_wait_for_rebuild_complete($self, $test_case_ref);
    sv_wait_for_zero($self, $test_case_ref);
}


sub sv_fault_drives
{
    my ($self, $test_case, $fault_num) = @_;
    my $device = $self->{device};
    my $rg_id = platform_shim_raid_group_get_property($test_case->{raid_group}, "id");

    for (my $i = 0; $i < $fault_num; $i += 1) {
        my $rem_drive = pop(@{$test_case->{disks}});
        push(@{$test_case->{fault_disks}}, $rem_drive);

        my $disk_id = platform_shim_disk_get_property($rem_drive, "id");
        $self->platform_shim_test_case_log_info("Remove disk ".$disk_id." in RAID ".$rg_id);
        platform_shim_disk_remove($rem_drive);
        $self->platform_shim_test_case_log_info('Waiting for removed state for disk ' . $disk_id);
        platform_shim_disk_wait_for_disk_removed($device, [$rem_drive]);
    }
}

sub sv_reinsert_all_drives
{
    my ($self, $test_case_ref) = @_;
    my @all_rem_drives;

    foreach my $test_case (@{$test_case_ref}) {
        my @rem_drives = @{$test_case->{fault_disks}};
        push(@all_rem_drives, @rem_drives);
        push(@{$test_case->{disks}}, @rem_drives);
        $test_case->{fault_disks} = [];
    }
    foreach my $disk (@all_rem_drives) {
        my $disk_id = platform_shim_disk_get_property($disk, "id");
        $self->platform_shim_test_case_log_info("Reinsert disk ".$disk_id);
    }
    platform_shim_device_recover_from_faults($self->{device});
}


sub sv_recover_fault
{
    my ($self, $test_case_ref) = @_;

    sv_reinsert_all_drives($self, $test_case_ref);
    my @luns;
    foreach my $test_case (@{$test_case_ref}) {
        my $lun = $test_case->{lun};
        if (defined $lun) {
            push (@luns, $lun);
        }
    }
    my $lun_ids = join(', ', map {platform_shim_lun_get_property($_, "id")} @luns);
    $self->platform_shim_test_case_log_info("Waiting for ready state for luns " . $lun_ids);
    platform_shim_lun_wait_for_ready_state($self->{device}, \@luns);
    sv_wait_for_rebuild_complete($self, $test_case_ref);
}


sub _sv_find_test_cases($$$)
{
    my ($self, $test_case_ref, $is_on_system_drives) = @_;
    my @test_case_sys;
    my @test_case_user;

    foreach my $test_case (@$test_case_ref) {
        my $rg = $test_case->{raid_group};
        if (sv_is_raid_group_on_system_drive($self, $rg)) {
            push(@test_case_sys, $test_case);
        } else {
            push(@test_case_user, $test_case);
        }
    }
    my $tc_on_sys_number = scalar(@test_case_sys);
    my $tc_on_user_number = scalar(@test_case_user);

    $self->platform_shim_test_case_log_info("Find: user: $tc_on_user_number, sys: $tc_on_sys_number");
    if ($is_on_system_drives) {
        return @test_case_sys;
    } else {
        return @test_case_user;
    }
}

sub sv_find_test_case_with_luns($$$)
{
    my ($self, $test_case_ref, $is_on_system_drives) = @_;
    my @cur_test = _sv_find_test_cases($self, $test_case_ref, $is_on_system_drives);
    my @valid_tests;

    $self->platform_shim_test_case_assert_msg(
        scalar(@cur_test) > 0, "No valid test case!");
    foreach my $test_case (@cur_test) {
        if (defined $test_case->{lun}) {
            push(@valid_tests, $test_case);
        }
    }
    if (scalar(@valid_tests) > 0) {
        # Random select a test case
        my $valid_test_number = scalar(@valid_tests);
        return $valid_tests[int(rand($valid_test_number))];
    }
    #Random select a test case and bind a lun on it.
    my $cur_test_number = scalar(@cur_test);
    my $test_case = $cur_test[int(rand($cur_test_number))];
    $self->platform_shim_test_case_assert_msg(
        !defined $test_case->{lun}, "Internal error: Has a lun: ".Dumper($test_case));
    my $lun = sv_create_lun($self, $test_case->{raid_group});
    $test_case->{lun} = $lun;
    sv_wait_for_zero($self, [$test_case]);
    $self->platform_shim_test_case_log_info("Use test case: ".Dumper($test_case));
    return $test_case;
}


sub sv_find_test_case_without_luns($$$)
{
    my ($self, $test_case_ref, $is_on_system_drives) = @_;
    my @cur_test = _sv_find_test_cases($self, $test_case_ref, $is_on_system_drives);
    my @valid_tests;

    $self->platform_shim_test_case_assert_msg(
        scalar(@cur_test) > 0, "No valid test case!");
    foreach my $test_case (@cur_test) {
        if (!defined $test_case->{lun}) {
            push(@valid_tests, $test_case);
        }
    }
    if (scalar(@valid_tests) > 0) {
        # Random select a test case
        my $valid_test_number = scalar(@valid_tests);
        my $test_case = $valid_tests[int(rand($valid_test_number))];
        $self->platform_shim_test_case_log_info("Use test case: ".Dumper($test_case));
        return $test_case;
    }

    #Random select a test case and unbind all luns
    my $cur_test_number = scalar(@cur_test);
    my $test_case = $cur_test[int(rand($cur_test_number))];
    $self->platform_shim_test_case_assert_msg(
        defined $test_case->{lun}, "Internal error: no luns: ".Dumper($test_case));

    sv_remove_lun($self, $test_case->{raid_group});
    $test_case->{lun} = undef;
    $self->platform_shim_test_case_log_info("Use test case: ".Dumper($test_case));
    return $test_case;
}


sub sv_select_test_cases_for_degraded_test($$)
{
    my ($self, $test_case_ref) = @_;
    my @cur_test_cases;

    foreach my $test_case (@{$test_case_ref}) {
        my $rg = $test_case->{raid_group};
        if (sv_is_raid_group_on_system_drive($self, $rg)) {
            next;
        }
        my $rg_type = platform_shim_raid_group_get_property($rg, 'raid_type');
        if ($rg_type =~ /r0|r10/i) {
            my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
            $self->platform_shim_test_case_log_info("Raid $rg_id: RG of type $rg_type is not applicable");
            next;
        }
        push @cur_test_cases, $test_case
    }
    if (scalar(@cur_test_cases) <= 0) {
        return undef;
    }
    # for functional test, we test on all raid level
    if (!platform_shim_test_case_is_promotion($self)) {
        return @cur_test_cases;
    }
    #for promotion test, we test on R5 only
    my @select_test_cases;
    for my $test_case (@cur_test_cases) {
        my $rg = $test_case->{raid_group};
        my $rg_type = platform_shim_raid_group_get_property($rg, 'raid_type');
        if ($rg_type =~ /r5/i) {
            my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
            $self->platform_shim_test_case_log_info("Select Raid $rg_id  RG of type $rg_type for test");
            push(@select_test_cases, $test_case);
            last;
        }
    }
    if (scalar(@select_test_cases) <= 0) {
        push(@select_test_cases, $cur_test_cases[0]);
    }
    return @select_test_cases;
}

=head1

Check SV stop when raid group is degraded

=cut
sub sv_check_sniff_stop_on_degraded
{
    my ($self, $test_case_ref) = @_;
    my @cur_test_cases;

    $self->platform_shim_test_case_log_info("SV: Check stop on degraded raid group");
    @cur_test_cases = sv_select_test_cases_for_degraded_test($self, $test_case_ref);

    sv_wait_for_rebuild_complete($self, \@cur_test_cases);
    foreach my $test_case (@cur_test_cases) {
        sv_fault_drives($self, $test_case, 1);
    }

    sv_check_sniff_verify_stop($self, \@cur_test_cases);
    sv_recover_fault($self, \@cur_test_cases);
}


=head1

Check SV stop when raid group is broken

=cut
sub sv_check_sniff_stop_on_broken
{
    my ($self, $test_case_ref) = @_;
    my @cur_test_cases;

    $self->platform_shim_test_case_log_info("SV: Check stop on degraded raid group");
    @cur_test_cases = sv_select_test_cases_for_degraded_test($self, $test_case_ref);
    sv_wait_for_rebuild_complete($self, \@cur_test_cases);

    foreach my $test_case (@cur_test_cases) {
        my $rg = $test_case->{raid_group};
        my $rg_type = platform_shim_raid_group_get_property($rg, 'raid_type');
        my $cnt = 0;
        my $rg_id = platform_shim_raid_group_get_property($rg, 'id');

        if ($rg_type =~ /r3|r5/i) {
            $cnt = 2;
        } elsif ($rg_type =~ /r6/i) {
            $cnt = 3;
        } elsif ($rg_type =~ /r0/i) {
            $cnt = 1;
        } else {
            $self->platform_shim_test_case_log_info("Raid $rg_id: RG of type $rg_type in not applicable");
        }
        if ($cnt != 0) {
            $self->platform_shim_test_case_log_info("Fault $cnt drives for RAID $rg_id");
            sv_fault_drives($self, $test_case, $cnt);
        }
    }
    sv_check_sniff_verify_stop($self, \@cur_test_cases);
    sv_recover_fault($self, \@cur_test_cases);
}


sub sv_generate_media_error_lba($$)
{
    my ($self, $lba) = @_;
    my @lbas;
    my $error_counts;

    if (platform_shim_test_case_is_promotion($self)) {
        $error_counts = 1;
    } else {
        $error_counts = 3;
    }
    for (my $i = 0; $i < $error_counts; $i += 1) {
        $lba = hex($lba) + hex('0x8000');
        $lba = sprintf("%#x", $lba);
        push(@lbas, $lba);
    }
    return @lbas;
}


sub sv_test_media_error_on_drive($$$$)
{
    my ($self, $disk, $lba_ref, $reset_chkpt) = @_;
    my @lbas = @$lba_ref;
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');    

    $self->platform_shim_test_case_log_info("Test media error on disk: $disk_id, lba: @lbas");

    my $sp = test_util_disk_get_active_sp($self, $disk);
    my $timeout = 10 * 60;

    if ($DEBUG_ENABLE_PVD_TRACING)
    {
        platform_shim_disk_set_pvd_debug_tracing($disk, $sp);
    }

    #Step 0: Wait for zero
    my $zero_timeout = 86400;
    $self->platform_shim_test_case_log_info("Verify zeroing completes on disk $disk_id");
    test_util_zeroing_wait_for_disk_zeroing_percent($self, $disk, 100, $zero_timeout);

    #Step 1: Stop verify. Clear all error report and error injection.
    platform_shim_device_set_sniff_verify($self->{device}, {state => 0});
    platform_shim_disk_clear_verify_report($disk, $sp);
    if ($g_use_lei)
    {
        platform_shim_device_disable_logical_error_injection($self->{device});   
        platform_shim_device_disable_logical_error_injection_records($self->{device});   
    }
    else
    {
        test_util_dest_stop($self, $sp);
        test_util_dest_clear($self, $sp);
    }
    platform_shim_disk_clear_error_handle_health_info($disk, $sp);

    #Step 2: Inject errors
    my $error_count = scalar(@lbas);    
    foreach my $lba (@lbas) {        
        if ($g_use_lei)
        {            
            my %lei_params = (error_injection_type => 'FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR',  
                              error_mode           => 'FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED',
                              error_count          => 0, 
                              error_limit          => 1, 
                              error_bit_count      => 0,
                              lba_address          => $lba,                          
                              blocks               => 1);
    
            platform_shim_device_create_logical_error_injection_record_for_fbe_object_id($self->{device}, \%lei_params, $pvd_object_id);
        }
        else
        {
            test_util_sniff_inject_media_error($self, $disk, $sp, lba => $lba);
        }
    }
    $self->platform_shim_test_case_log_info("Set SV checkpoint to $reset_chkpt");

    #Step 3: Set sniff checkpoint and start verify
    platform_shim_disk_set_sniff_checkpoint($disk, $sp, $reset_chkpt);
    my $sp_bookmark = platform_shim_sp_bookmark_system_log($sp);
    my $sp_name = platform_shim_sp_get_short_name($sp);
    $self->platform_shim_test_case_log_info("System log sp$sp_name bookmark: $sp_bookmark");
    if ($g_use_lei)
    {
        platform_shim_device_enable_logical_error_injection_fbe_object_id($self->{device}, $pvd_object_id);
    }
    else
    {
        test_util_dest_start($self, $sp);
    }
    platform_shim_device_set_sniff_verify($self->{device}, {state => 1});

    #Step 4: Verify using error count and event log
    my $prev_lba = $lbas[0];
    foreach my $lba (@lbas)
    {        
        # If checkpoints are too far out, then bump up the verify checkpoint to 
        # speed up the test.
        if (hex($lba) - hex($prev_lba) > 0x10000)
        {
            my $chkpt = sprintf("%#x", hex($lba) - 0x1000);   # set it slightly before lba we are waiting for
            $self->platform_shim_test_case_log_info("Set SV checkpoint to $chkpt to speed up test");
            platform_shim_disk_set_sniff_checkpoint($disk, $sp, $chkpt);
        }
        test_util_sniff_wait_for_checkpoint($self, $disk, $lba, $timeout);
        $prev_lba = $lba;
    }    
    test_util_sniff_verify_error_count($self, $disk, $sp, $error_count, $g_use_lei);
    test_util_sniff_check_event_log($self, $sp, $sp_bookmark, $error_count);

    if ($DEBUG_ENABLE_PVD_TRACING)
    {
        # turn off tracing if it was enabled
        platform_shim_disk_clear_pvd_debug_tracing($disk, $sp);
    }
}


sub sv_test_media_error_on_drive_with_offset($$$)
{
    my ($self, $disk, $offset) = @_;
    my $lba = $offset;

    my @lbas = sv_generate_media_error_lba($self, $lba);
    sv_test_media_error_on_drive($self, $disk, \@lbas, $offset);
}


=head1

Inject media error on unbound non-system drive.
Verify using event log and error counts in BG verify report

=cut
sub sv_test_media_error_on_unbound_non_system_drive
{
    my ($self) = @_;

    my $disk = sv_select_non_system_disk_for_sniff($self);
    if (!$disk) {
        $self->platform_shim_test_case_log_warn("SV: Skip non-system unbound media error test");
        return;
    }
    my $lba = '0x18000';
    my @lbas = sv_generate_media_error_lba($self, $lba);
    my $reset_chkpt = '0x10000';

    sv_test_media_error_on_drive($self, $disk, \@lbas, $reset_chkpt);
}


=head1

Inject media error on unbound system drive.
Verify using event log and error counts in BG verify report

=cut
sub sv_test_media_error_on_system_drive
{
    my ($self) = @_;
    my $disk = sv_select_system_disk_for_sniff($self);

    if (!$disk) {
        $self->platform_shim_test_case_log_warn("SV: Skip system unbound media error test");
        return;
    }
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    $self->platform_shim_test_case_log_info("Error injection test on system drive $disk_id");
    my $offset = platform_shim_disk_get_pvd_default_offset($disk);
    $self->platform_shim_test_case_log_info("PVD Offset: " . Dumper($offset));
    sv_test_media_error_on_drive_with_offset($self, $disk, $offset);
}


=head1

Inject media error on bound region
Verify using event log and error counts

=cut
sub sv_test_media_error_on_bound_drive
{
    my ($self, $test_case_ref) = @_;
    my $test_case;

    $self->platform_shim_test_case_log_info("SV: Check media error on bound drive");
    $test_case = sv_find_test_case_with_luns($self, $test_case_ref, 0);

    #Step 0: Wait for background service to finish
    sv_wait_for_rebuild_complete($self, [$test_case]);

    #Setp 1: Select the frist disk for test
    $self->platform_shim_test_case_log_info("SV: Select " . Dumper($test_case));
    my @disks = platform_shim_raid_group_get_disk($test_case->{raid_group});
    my $disk = $disks[0];
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    $self->platform_shim_test_case_log_info("Select drive $disk_id for test");

    #Step 2: Calculate the lba range for test
    my $lun = $test_case->{lun};
    my $lun_id = platform_shim_lun_get_property($lun, 'id');
    my $lun_capacity = platform_shim_lun_get_capacity($test_case->{lun});
    my $capacity = sprintf("%#x", hex($lun_capacity) - hex("0x1"));

    $self->platform_shim_test_case_log_info("Map pba for lun $lun_id, LBA: 0");
    my $first_pba = platform_shim_lun_map_lba($lun, 0);
    $self->platform_shim_test_case_log_info("Map pba for lun $lun_id, LBA: $capacity");
    my $last_pba = platform_shim_lun_map_lba($lun, $capacity);
    $self->platform_shim_test_case_log_info("LUN $lun_id: [$first_pba, $last_pba], cap: $lun_capacity");

    my $lba = $first_pba;
    my @lbas = sv_generate_media_error_lba($self, $lba);

    #Step 3: Stop verify and clear LUN verify report
    platform_shim_device_set_sniff_verify($self->{device}, {state => 0});
    platform_shim_lun_clear_bv_report($lun);

    #Step 4: Do test
    my $reset_chkpt = $first_pba;
    sv_test_media_error_on_drive($self, $disk, \@lbas, $reset_chkpt);
}


=head1

Inject media error on system lun
Verify using event log and error counts

=cut
sub sv_test_media_error_on_system_lun
{
    my ($self) = @_;
    my %params = ( vault => 1,
                   id => '0_0_0',
                   technology => ['normal']
               );
    my @system_drives = platform_shim_device_get_disk($self->{device}, \%params);
    my @system_luns_object_id = sv_get_system_lun_object_id($self);
    $self->platform_shim_test_case_log_info("Get system lun: @system_luns_object_id");

    # Test on the first 5 luns to save time
    my $error_count = 5;
    if ($error_count > scalar(@system_luns_object_id)) {
        $error_count = scalar(@system_luns_object_id);
        $self->platform_shim_test_case_log_info("Test on $error_count LUNs");
    }

    my @test_luns;
    for (my $i = 0; $i < $error_count; $i += 1) {
        push(@test_luns, $system_luns_object_id[$i]);
    }

    my $sp = platform_shim_device_get_target_sp($self->{device});
    my @PBAs;

    foreach my $lun_obj_id (@test_luns) {
        my $pba = platform_shim_sp_map_lun_lba_by_object_id($sp, $lun_obj_id, 0);
        $self->platform_shim_test_case_log_info("    $lun_obj_id   =>    $pba");
        push(@PBAs, $pba);
    }

    foreach my $lun_obj_id (@test_luns) {
        $self->platform_shim_test_case_log_info("Clear verify report for LUN object $lun_obj_id");
        platform_shim_sp_clear_lun_bv_report_by_object_id($sp, $lun_obj_id);
    }
    $self->platform_shim_test_case_log_info("Disable verify...");
    platform_shim_device_set_sniff_verify($self->{device}, {state => 0});

    my @sorted_pbas = map {sprintf("%#x", $_) } sort { $a <=> $b } map {hex($_) } @PBAs;
    my $reset_chkpt = $sorted_pbas[0];
    my $disk = $system_drives[0];
    sv_test_media_error_on_drive($self, $disk, \@sorted_pbas, $reset_chkpt);
}


=head1

Inject media error on raid groups without lun

=cut
sub sv_test_media_error_on_raid_groups_without_lun($$)
{
    my ($self, $test_case) = @_;
    my $rg = $test_case->{raid_group};
    my @disks = platform_shim_raid_group_get_disk($rg);
    my $disk = $disks[0];
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my $offset = platform_shim_disk_get_pvd_default_offset($disk);
    $self->platform_shim_test_case_log_info("SV: Test on disk $disk_id, offset: $offset");
    sv_test_media_error_on_drive_with_offset($self, $disk, $offset);
}


=head1

Inject media error on raid groups without lun on non-system drives

=cut
sub sv_test_media_error_on_raid_groups_on_non_system_drive
{
    my ($self, $test_case_ref) = @_;
    my $tc_user = sv_find_test_case_without_luns($self, $test_case_ref, 0);

    sv_test_media_error_on_raid_groups_without_lun($self, $tc_user);
}

=head1

Inject media error on raid groups without lun on system drives

=cut
sub sv_test_media_error_on_raid_groups_on_system_drive
{
    my ($self, $test_case_ref) = @_;
    my $tc_sys = sv_find_test_case_without_luns($self, $test_case_ref, 1);

    sv_test_media_error_on_raid_groups_without_lun($self, $tc_sys);
}

=head1

Reboot one SP, check SV checkpoint is updated

=cut
sub sv_test_single_sp_reboot
{
    my ($self, $test_case_ref) = @_;

    if ($SKIP_REBOOTS)
    {
        $self->platform_shim_test_case_log_warn("Skipping Reboot Test");
        return;
    }

    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    my $active_name = platform_shim_sp_get_property($active_sp, 'name');


    # Skip on system drives as system raid group may do verify after reboot
    my @cur_tests = _sv_find_test_cases($self, $test_case_ref, 0);
    my @all_disks = sv_get_all_test_disks_for_fast_test($self, \@cur_tests);
    $self->platform_shim_test_case_log_info("SV: Single sp reboot test");
    my $disk_ids = join(', ',
                        map {platform_shim_disk_get_property($_, 'id')} @all_disks);
    $self->platform_shim_test_case_log_info("Run test on disks: [$disk_ids]");
    platform_shim_device_set_sniff_verify($device, {state => 0});
    $self->platform_shim_test_case_log_info("Rebooting SP $active_name");

    my $start = time;
    platform_shim_sp_reboot($active_sp);
    $self->platform_shim_test_case_log_info("wait for active sp to come back up");
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    my $elapse = time - $start;
    $self->platform_shim_test_case_log_info("SP $active_name came back in $elapse seconds");
    $self->platform_shim_test_case_log_info("Check if checkpoint update");
    platform_shim_device_set_sniff_verify($device, {state => 1});

    sv_check_sniff_verify_update_on_drives($self, \@all_disks);
}


=head1

Reboot dual SPs, check SV checkpoint is updated

=cut
sub sv_test_dual_sp_reboot
{
    my ($self, $test_case_ref) = @_;

    if ($SKIP_REBOOTS)
    {
        $self->platform_shim_test_case_log_warn("Skipping Reboot Test");
        return;
    }

    my $device = $self->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {});
    my $passive_sp = platform_shim_device_get_passive_sp($device, {});
    my $active_name = platform_shim_sp_get_property($active_sp, 'name');

    # Skip on system drives as system raid group may do verify after reboot
    my @cur_tests = _sv_find_test_cases($self, $test_case_ref, 0);
    my @all_disks = sv_get_all_test_disks_for_fast_test($self, \@cur_tests);
    $self->platform_shim_test_case_log_info("SV: dual sp reboot test");
    my $disk_ids = join(', ',
                        map {platform_shim_disk_get_property($_, 'id')} @all_disks);
    $self->platform_shim_test_case_log_info("Run test on disks: [$disk_ids]");

    $self->platform_shim_test_case_log_info("reboot both SPs");
    platform_shim_sp_reboot($active_sp);
    platform_shim_sp_reboot($passive_sp);

    $self->platform_shim_test_case_log_info("wait for both sps to come back up");
    platform_shim_sp_wait_for_agent($active_sp, 3600);
    platform_shim_sp_wait_for_agent($passive_sp, 3600);
    platform_shim_device_wait_for_ecom($device, 1800);
    $self->platform_shim_test_case_log_info("SV: Verify checkpoint update after SPs come back");

    test_util_general_set_target_sp($self, $active_sp);


    sv_check_sniff_verify_update_on_drives($self, \@all_disks);
}

=head1

Cleanup for sniff verify.
1. enable automatic sparing
2. clean DEST records
3. enable sniff verify

=cut
sub sniff_cleanup($)
{
    my ($self) = @_;
    my $device = $self->{device};

    $self->platform_shim_test_case_log_step("SV: doing basic cleanup");
    $self->platform_shim_test_case_log_info("SV: Enable automatic sparing");
    sv_enable_automatic_sparing($self);

    $self->platform_shim_test_case_log_info("SV: Clear DEST records");
    my @sps = platform_shim_device_get_sp($device);
    for my $sp (@sps) {
        test_util_general_set_target_sp($self, $sp);
        if ($g_use_lei)
        {
            platform_shim_device_disable_logical_error_injection($device);   # first cleanup active side from previous run
            platform_shim_device_disable_logical_error_injection_records($device);       
        }
        else
        {
            test_util_dest_stop($self, $sp);
            test_util_dest_clear($self, $sp);
        }

        if ($DEBUG_ENABLE_PVD_TRACING)
        {
            platform_shim_disk_clear_all_pvd_debug_tracing($sp);
        }
    }
    $self->platform_shim_test_case_log_info("SV: Enable sniff verify");
    platform_shim_device_set_sniff_verify($self->{device}, {state => 1});
}


sub main
{
    my ($self) = @_;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");

    $SKIP_REBOOTS = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($SKIP_REBOOTS == 1) {
       print "Skiping reboot during test since skip_reboot parameter set.\n";
    }

    $self->platform_shim_test_case_log_start();

    $self->platform_shim_test_case_add_to_cleanup_stack({'sniff_cleanup' => {}});

    $self->platform_shim_test_case_log_info("Waiting for disk zeroing");

    #test_util_sniff_wait_min_disks_zeroed($self, 21, 5400);   #removed so we can run with small configs.

    $self->platform_shim_test_case_log_info("Setup sniff configuration.");
    my $test_cases_ref = $self->setup_config();
    
    $self->platform_shim_test_case_log_info("Using test: " . Dumper($test_cases_ref));

    # For develop test only .... remove it when finish
    # Choose only 1 test case for quick check
    # {
    #     $test_cases_ref = [ $test_cases_ref->[3]];
    #     $self->platform_shim_test_case_log_info("Using test: " . Dumper($test_cases_ref));
    # }

    sv_disable_automatic_sparing($self);    

    $self->platform_shim_test_case_log_step("Check if SV enabled on both SPs.");
    sv_check_enabled($self);

    $self->platform_shim_test_case_log_step("Check if SV checkpoint updates");
    sv_check_sniff_verify_update($self, $test_cases_ref);

    $self->platform_shim_test_case_log_step("SV: Check stop on degraded raid group");

    sv_check_sniff_stop_on_degraded($self, $test_cases_ref);

    $self->platform_shim_test_case_log_step("SV: Check stop on broken raid group");
    sv_check_sniff_stop_on_broken($self, $test_cases_ref);

    $self->platform_shim_test_case_log_step("SV: Check media error on unbound non system drive");
    sv_test_media_error_on_unbound_non_system_drive($self);

    $self->platform_shim_test_case_log_step("SV: Check media error on bound drive");
    sv_test_media_error_on_bound_drive($self, $test_cases_ref);

    # Put functional test cases here
    if (platform_shim_test_case_is_functional($self)) {

        $self->platform_shim_test_case_log_step("SV: Check media error on raid group on non-system drives without lun");
        sv_test_media_error_on_raid_groups_on_non_system_drive($self, $test_cases_ref);

        if ($test_on_system_drives) {

            $self->platform_shim_test_case_log_step("SV: Check media error on system drive");
            sv_test_media_error_on_system_drive($self);

            $self->platform_shim_test_case_log_step("SV: Check media error on system lun");
            sv_test_media_error_on_system_lun($self);

            $self->platform_shim_test_case_log_step("SV: Check media error on raid group on system drives without lun");
            sv_test_media_error_on_raid_groups_on_system_drive($self, $test_cases_ref);
        }

        $self->platform_shim_test_case_log_step("SV: Check on single SP reboot");
        sv_test_single_sp_reboot($self, $test_cases_ref);

        $self->platform_shim_test_case_log_step("SV: Check on dual SP reboot");
        sv_test_dual_sp_reboot($self, $test_cases_ref);
    }

    return;
}


1;
