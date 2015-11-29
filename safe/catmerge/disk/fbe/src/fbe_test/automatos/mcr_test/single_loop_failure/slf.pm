package mcr_test::single_loop_failure::slf;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_configuration;
use mcr_test::test_util::test_util_zeroing;
use mcr_test::test_util::test_util_disk;
use mcr_test::test_util::test_util_general;
use mcr_test::platform_shim::platform_shim_host;
use mcr_test::platform_shim::platform_shim_event;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_enclosure;
use mcr_test::test_util::test_util_slf;
use mcr_test::test_util::test_util_host_io;
use mcr_test::test_util::test_util_io;
use mcr_test::test_util::test_util_performance_stats;
use mcr_test::test_util::test_util_sniff;
use mcr_test::test_util::test_util_zeroing;

use strict;
use warnings;

my $g_skip_reboots = 0;

#
#
# Method: pretest_steps
# This is a Classwide version of preTestCase used for all the tests in the ParentFeature
# Directory.
# Performs all actions required prior to the test case being executed.
# The test developer can list the actions that will happen in the pre test case.
#
# Input:
# *None*
#
# Returns:
# *None*
#
# Exceptions:
# *None*
#
# Debugging Flags:
# *None*
#

sub pretest_steps
{
    my ($self) = @_;

    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("Before Main test, the pre-requisite test steps of $name");

    # set cache as enabled so luns will be enabled by default when cache is turned on
    $self->set_cache('enable');
    
    return;
}
# ==================Single Loop Failure Test Cases===========================================
# * Create One RAID Group
# * Create LUNs.
# * Run a test with cache disabled where we cause SLF.
# * Run a test with cache enabled where we cause SLF.
# * Run a test where we cause SLF on the active side of a PVD and check that sniff continues on the peer.
# * Run a test where we cause SLF on the active side of a PVD and check that zeroing continues on the peer.
# 
# For Functional level test we should:
#    * use more than one Raid Group.
#    * Perform reboot of the SLF SP.
#    * Perform reboot of the SP that does not see SLF. 
# 
# ======================================================================================
sub main
{
    test_util_general_test_start();
    my ($self) = @_;
    my $purpose;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    my $device = $self->{device};
    my $luns_per_rg     = 1;
    my $lun_capacity_gb = 1;

    $g_skip_reboots = $self->platform_shim_test_case_get_parameter(name => "skip_reboot");
    if ($g_skip_reboots == 1) {
       print "Skiping reboot during test since skip_reboot parameter set.\n";
    }

    $self->platform_shim_test_case_add_to_cleanup_stack({'slf_cleanup' => {}});     
    
    $self->pretest_steps();
    my @rgs = platform_shim_device_get_raid_group($self->{device});
    my $pre_existing_rgs = @rgs;
    my @rg_configs = @{slf_initialize_rg_configs($self, $luns_per_rg, $lun_capacity_gb, \@rgs)};


    $purpose = "With Cache disabled (write through mode), verify that if IO is sent to a LUN via SP A, ".
               "and single backend loop failure is introduced on SP A side, ".
               "IO is redirected (by MCC) via SP B. Verify that when single ".
               "backened loop failure is removed, IO path via A is restored (by MCC).";
                              
    $self->platform_shim_test_case_print_purpose($purpose);
    induce_slf_with_write_cache($self, \@rg_configs, "clar_lun"); 


    if (platform_shim_test_case_is_functional($self))
    {
        $purpose = "Verify that if IO is sent to an fbe LUN via SP A, ".
                   "and single backend loop failure is introduced on SP A side, ".
                   "IO is not redirected (by MCR) via SP B at the lun level. Verify that when single ".
                   "backened loop failure is removed, IO path via A is restored.";
        
        $self->platform_shim_test_case_print_purpose($purpose);
        induce_slf_with_write_cache($self, \@rg_configs, "fbe_lun");
    }

    # the sniff tests only runs at functional level
    if (platform_shim_test_case_is_functional($self)) {	
        $purpose = 'Verify that once backend path failure is reported by PVD of '.
                   'the active object SP, the active object SP should be changed '.
                   'to the peer, and sniff verify operation continues on the peer.';
    
        $self->platform_shim_test_case_print_purpose($purpose);
        sniff_verify_pvd($self);
    }
   
    # The zeroing tests only runs at functional level.  
    # This test does not fit in the promotion test.
    if (platform_shim_test_case_is_functional($self)) {
        $purpose = 'Verify that once backend path failure is reported during zeroing by'.
                   'PVD on the active object SP side, the active object SP should be '.
                   'changed to the peer, and zeroing operation continues on the peer.';
  
        $self->platform_shim_test_case_print_purpose($purpose);
        slf_disk_zeroing($self);
    }
   
    platform_shim_device_rdgen_stop_io($self->{device}, {});

    test_util_general_log_test_time($self);
}

sub induce_slf_with_write_cache
{  
    my ($self, $rg_config_ref, $mode) = @_;
    my @rg_configs = @{$rg_config_ref};
    
    my $device = $self->{device};
    
    my $target_sp = platform_shim_device_get_target_sp($device);
    my $sp_name = platform_shim_sp_get_property($target_sp, 'name');

    $self->slf_write_background_pattern();

    # set cache as per the test case
    $self->set_cache('disable');

    my @all_luns = map {@{$_->{luns}}} @rg_configs;
    my @disks_to_fault = map {@{$_->{disks_to_fault}}} @rg_configs;
    
    # Cause SLF on random disk, and random SP.
    my @sps = platform_shim_device_get_sp($device);
    my $slf_sp;
    my $non_slf_sp;
    if (int(rand(2))) {
       $slf_sp = $sps[1];
       $non_slf_sp = $sps[0];
    } else{
       $slf_sp = $sps[0];
       $non_slf_sp = $sps[1];
    }
    
    test_util_io_wait_for_dirty_pages_flushed($self, 600);
    test_util_performance_stats_enable_sep_pp($self);
  
    test_util_general_set_target_sp($self, $target_sp);
    $self->platform_shim_test_case_log_step("Running IO from $sp_name");
	$self->slf_start_io(\@rg_configs, $mode);
	
    $self->platform_shim_test_case_log_info('Let IO run for 10 sec');
    sleep 10;

    test_util_io_verify_rdgen_io_progress($self, 1, 60);

    # check IO counters are incrementing on the owning SP 
    test_util_performance_stats_check_io_counters($self, {rdgen_sp => $target_sp, luns => \@all_luns});
    test_util_performance_stats_check_disk_io_counters($self, {rdgen_sp => $target_sp, disks => \@disks_to_fault});

    
    
    # start slf on disks
    $self->platform_shim_test_case_log_step("Induce SLF on ".uc($sp_name));
    test_util_slf_induce_disk_slf($self, \@disks_to_fault, $slf_sp);
    
    # check that IO is running
    test_util_general_set_target_sp($self, $target_sp);
    test_util_io_verify_rdgen_io_progress($self, 1, 60);

    if ($mode =~ /clar_lun/)
    {
        # check that IO counters are incrementing on the peer sp only
        test_util_performance_stats_check_io_counters($self, {slf_sp => $slf_sp,
                                                              luns => \@all_luns});
        test_util_performance_stats_check_disk_io_counters($self, {slf_sp => $slf_sp,
                                                                   disks => \@disks_to_fault}); 
    }
    else
    {
        # check that lun IO counters are incrementing on the rdgen sp only
        test_util_performance_stats_check_io_counters($self, {rdgen_sp => $target_sp,
                                                              luns => \@all_luns});
        # check that ios are being redirected to the non slf'd sp                                                             
        test_util_performance_stats_check_disk_io_counters($self, {slf_sp => $slf_sp,
                                                                   disks => \@disks_to_fault}); 
        
    }                                                         

    # let IO run for 10 sec
    $self->platform_shim_test_case_log_info('Let IO run for 10 sec');
    sleep 10;

    # check that IO is running
    test_util_general_set_target_sp($self, $target_sp);
    test_util_io_verify_rdgen_io_progress($self, 1, 60);
 
    # restore SLF
    platform_shim_device_recover_from_faults($device);
    test_util_general_set_target_sp($self, $slf_sp);
    test_util_slf_wait_for_slf_cleared($self, \@disks_to_fault);

    # check that IO is running
    test_util_general_set_target_sp($self, $target_sp);
    test_util_io_verify_rdgen_io_progress($self, 1, 60);

    # check IO counters are incrementing on the owning sp
    test_util_performance_stats_check_io_counters($self, {rdgen_sp => $target_sp});
    test_util_performance_stats_check_disk_io_counters($self, {rdgen_sp => $target_sp, disks => \@disks_to_fault});
    
    # stop IO
    $self->platform_shim_test_case_log_info("Stopping rdgen threads");
    test_util_general_set_target_sp($self, $target_sp);
    platform_shim_device_rdgen_stop_io($self->{device}, {});

    if ($self->platform_shim_test_case_is_functional())
    {
        # reboot tests

        if ($g_skip_reboots)
        {
            $self->platform_shim_test_case_log_warn("Skipping Reboot Test");
        }
        else
        {
            platform_shim_device_disable_automatic_sparing($device);
            platform_shim_device_set_sparing_timer($device, 10000); 
            test_util_slf_induce_disk_slf($self, \@disks_to_fault, $slf_sp);
            test_util_general_set_target_sp($self, $non_slf_sp);
            platform_shim_device_initialize_rdgen($device);
            $self->slf_start_io(\@rg_configs, $mode);
            
    
            # reboot the slf sp 
            $self->platform_shim_test_case_log_step("Rebooting SLF SP: ".platform_shim_sp_get_property($slf_sp, "name"));
            platform_shim_sp_reboot($slf_sp);
            test_util_general_set_target_sp($self, $non_slf_sp);
            # verify that counters are incrementing on the non slf sp still
            test_util_io_verify_rdgen_io_progress($self, 1, 60);
            # check IO counters are incrementing on the rdgen sp
            test_util_performance_stats_check_io_counters($self, {rdgen_sp => $non_slf_sp, single_sp => 1});
            test_util_performance_stats_check_disk_io_counters($self, {slf_sp => $slf_sp, disks => \@disks_to_fault});
            $self->platform_shim_test_case_log_info("Wait for slf sp to come up");
            platform_shim_sp_wait_for_agent($slf_sp, 3600);
                    
            
            test_util_general_set_target_sp($self, $slf_sp);
            platform_shim_device_initialize_rdgen($device);
            test_util_slf_induce_disk_slf($self, \@disks_to_fault, $slf_sp);
            test_util_general_set_target_sp($self, $slf_sp);
    
            $self->slf_start_io(\@rg_configs, $mode);
            # verify that counters are incrementing on the non slf sp still
            test_util_io_verify_rdgen_io_progress($self, 1, 60);
            test_util_performance_stats_enable_sep_pp($self);
            platform_shim_device_disable_automatic_sparing($device);
            platform_shim_device_set_sparing_timer($device, 10000); 
    
            #reboot non slf sp
            $self->platform_shim_test_case_log_step("Rebooting Non SLF SP: ".platform_shim_sp_get_property($non_slf_sp, "name"));
            platform_shim_sp_reboot($non_slf_sp);
            test_util_general_set_target_sp($self, $slf_sp);
            # verify io counters continue for appropriate luns
            my @r0_luns = grep {platform_shim_lun_get_property($_, 'raid_type') eq "r0"} @all_luns;
            my @redundant_luns = grep {platform_shim_lun_get_property($_, 'raid_type') ne "r0"} @all_luns;
            test_util_performance_stats_check_io_counters($self, {rdgen_sp => $slf_sp, single_sp => 1, luns => \@redundant_luns});
            test_util_performance_stats_check_io_counters_stopped($self, {sp => $slf_sp, luns => \@r0_luns});
            platform_shim_device_rdgen_stop_io($self->{device}, {});
            platform_shim_device_recover_from_faults($self->{device});
            $self->platform_shim_test_case_log_info("Wait for non slf sp to come up");
            platform_shim_sp_wait_for_agent($non_slf_sp, 3600);
            platform_shim_device_enable_automatic_sparing($device);
            platform_shim_device_set_sparing_timer($device, 300); 
        }
    }
      

    platform_shim_device_recover_from_faults($self->{device});
    test_util_general_set_target_sp($self, $slf_sp);
    test_util_slf_wait_for_slf_cleared($self, \@disks_to_fault);

}

sub sniff_verify_pvd
{
    my ($self) = @_;
    my @disks;
    
    # Enable sniff verify
    test_util_sniff_enable($self);

    # Get the unbound disks excluding disks from bus_encl 0_0
    $self->platform_shim_test_case_log_info('Gathering unused disk to use for sniff verify...');

    my $disk = test_util_sniff_select_disk_for_sniff($self);
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    
    $self->platform_shim_test_case_log_info('Selected drive ' . $disk_id . ' for Sniff Verify');

    # Pre SLF - get the active and passive SP for the drive
	my %hash_getsp = (object =>$disk);
    my $active_sp_pre_slf = platform_shim_device_get_active_sp($self->{device}, \%hash_getsp);
    my $passive_sp_pre_slf = platform_shim_device_get_passive_sp($self->{device},\%hash_getsp);

    # get PVD obj of the drive
    my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');
    my $msg1 = "Pre SLF - Active SP for the PVD object ($pvd_object_id)".
              " of drive $disk_id is ".
              uc(platform_shim_sp_get_property($active_sp_pre_slf, 'name'));
    $self->platform_shim_test_case_log_info($msg1);

    # this will set the sp on which the xtool cmd will get executed
    test_util_general_set_target_sp($self, $active_sp_pre_slf);

    # get sniff verfiy checkpoint
    my %disk_snv_stats;
    $disk_snv_stats{'checkpoint'} = platform_shim_disk_get_property($disk, 'verify_checkpoint');
    $disk_snv_stats{'percent'} = platform_shim_disk_get_property($disk, 'verify_percent');

    $msg1 = 'Sniff Verify Checkpoint is '.$disk_snv_stats{'checkpoint'} .
            ' and Sniff Verify Percent is '.$disk_snv_stats{'percent'} .'%';
    $self->platform_shim_test_case_log_info("For disk $disk_id, $msg1");

    $self->platform_shim_test_case_log_info('Checking the progress of sniff verify (timeout in 10 mins)');
    #test_util_sniff_wait_for_sniff_percent_increase($self, $disk, $disk_snv_stats{percent}, 3600);
    test_util_sniff_wait_for_checkpoint_update($self, $disk, $disk_snv_stats{'checkpoint'}, 300); 
    
    # Inducing SLF on active obj sp
	my %hash_slf = (disk => $disk, sp => $active_sp_pre_slf);
    #my %encl_params = test_util_slf_induce_slf($self, %hash_slf);
    my $disk_fault = platform_shim_disk_remove_on_sp($disk, $active_sp_pre_slf);
    test_util_slf_wait_for_slf($self, [$disk]);

    # POST SLF - get the active SP for the drive
    my $active_sp_post_slf = platform_shim_device_get_active_sp($self->{device}, {object => $disk});

    my $msg2 = "Active SP for the PVD object ($pvd_object_id) of drive $disk_id is ".
            uc(platform_shim_sp_get_property($active_sp_post_slf,'name'));

    if (platform_shim_sp_get_property($active_sp_post_slf, 'name') ne 
        platform_shim_sp_get_property($passive_sp_pre_slf, 'name')) {
        platform_shim_test_case_throw_exception("Active object SP has not changed after inducing SLF. $msg2");
    } 
    else {
        $self->platform_shim_test_case_log_info("After inducing SLF, $msg2");
    }

    $self->platform_shim_test_case_log_step("Verify sniff verify continues for disk $disk_id on ".
                    uc(platform_shim_sp_get_property($active_sp_post_slf, 'name')));

    test_util_general_set_target_sp($self, $active_sp_post_slf);
    $disk_snv_stats{'checkpoint'} = platform_shim_disk_get_property($disk, 'verify_checkpoint');
    $disk_snv_stats{'percent'} = platform_shim_disk_get_property($disk, 'verify_percent');

    $msg1 = 'Sniff Verify Checkpoint is '.$disk_snv_stats{'checkpoint'} .
            ' and Sniff Verify Percent is '.$disk_snv_stats{'percent'} .'%';
    $self->platform_shim_test_case_log_info("For disk $disk_id, $msg1");
    #test_util_sniff_wait_for_sniff_percent_increase($self, $disk, $disk_snv_stats{percent}, 3600);
    test_util_sniff_wait_for_checkpoint_update($self, $disk, $disk_snv_stats{'checkpoint'}, 300); 
    
    # restore SLF
    #test_util_slf_restore_slf($self, %encl_params);
    platform_shim_disk_reinsert($disk_fault);

    # POST SLF restore - get the active SP for the drive
    my $active_sp_post_slf_restore = platform_shim_device_get_active_sp($self->{device}, \%hash_getsp);

    $msg2 = "After restoring SLF, Active SP for the PVD object ($pvd_object_id) ".
           "of drive $disk_id is ". uc(platform_shim_sp_get_property($active_sp_post_slf_restore, 'name'));
    $self->platform_shim_test_case_log_info($msg2);

    $self->platform_shim_test_case_log_info("Verifying sniff verify continues for disk $disk_id on ".
                   uc(platform_shim_sp_get_property($active_sp_post_slf_restore, 'name')));

    test_util_general_set_target_sp($self, $active_sp_post_slf_restore);
    #test_util_sniff_wait_for_sniff_percent_increase($self, $disk, $disk_snv_stats{percent}, 3600);
    test_util_sniff_wait_for_checkpoint_update($self, $disk, $disk_snv_stats{'checkpoint'}, 300); 
}


sub slf_disk_zeroing
{
    my ($self) = @_;
    $self->platform_shim_test_case_log_info('Gathering unused disk to use for zeroing...');
    
    # Get the Parameter names and values
    my %params = $self->platform_shim_test_case_get_parameter();

    my @disks = @{test_util_configuration_get_fastest_disks($self, {num_disks => 1, unzeroed_preferred => 1})};
    my $disk = shift(@disks);
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    
    $self->platform_shim_test_case_log_info ('Selected drive ' . $disk_id . ' for disk zeroing');

    # get PVD obj of the drive
    my $pvd_obj_id = platform_shim_disk_get_property($disk, 'fbe_id');	

    # Pre SLF - get the active and passive SP for the drive
    my %hash_getsp = (object =>$disk);
    my $active_sp_pre_slf = platform_shim_device_get_active_sp($self->{device}, \%hash_getsp);
    my $passive_sp_pre_slf = platform_shim_device_get_passive_sp($self->{device},\%hash_getsp);
	
    my $msg = "Pre SLF - Active SP for the PVD object ($pvd_obj_id) ".
              "of drive $disk_id is ".
              uc(platform_shim_sp_get_property($active_sp_pre_slf, 'name'));
    $self->platform_shim_test_case_log_info($msg);

    # initiate disk zeroing
    platform_shim_disk_start_background_zero($disk);	

    # this will set the sp on which the FBE API XTool cmd will get executed
    test_util_general_set_target_sp($self, $active_sp_pre_slf);	

    my $disk_zero_pct = 0;
    my $disk_zero_chkpt = 0;

    $disk_zero_pct = platform_shim_disk_get_property($disk, 'zero_percent');
    # Takes too much time to wait for 2%, so we simply skip this in promotion.
    if (!platform_shim_test_case_is_promotion($self)) {
       $disk_zero_pct = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zero_pct, 1800);
    }
	
    my %hash_slf = (disk => $disk, sp => $active_sp_pre_slf);
    my $disk_fault = platform_shim_disk_remove_on_sp($disk, $active_sp_pre_slf);	
    test_util_slf_wait_for_slf($self, [$disk]);

    # POST SLF - get the active SP for the drive
    my $active_sp_post_slf = platform_shim_device_get_active_sp($self->{device}, \%hash_getsp);	

    $msg = "Active SP for the PVD object ($pvd_obj_id) of drive $disk_id is ".
            uc(platform_shim_sp_get_property($active_sp_post_slf, 'name'));

    if (platform_shim_sp_get_property($active_sp_post_slf, 'name') ne 
        platform_shim_sp_get_property($passive_sp_pre_slf,'name')) {
        platform_shim_test_case_throw_exception(
            "Active object SP has not changed after inducing SLF. $msg");
    } 
    else {
        $self->platform_shim_test_case_log_info("After inducing SLF, $msg");
    }

    $self->platform_shim_test_case_log_info("Verify zeroing continues for disk $disk_id on ".
                    uc(platform_shim_sp_get_property($active_sp_post_slf, 'name')));

    test_util_general_set_target_sp($self, $active_sp_post_slf);
    $disk_zero_pct = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zero_pct, 1800);
    
    # restore SLF
    platform_shim_disk_reinsert($disk_fault);	

    # POST SLF restore - get the active SP for the drive
    my $active_sp_post_slf_restore = platform_shim_device_get_active_sp($self->{device}, \%hash_getsp);

    $msg = "After restoring SLF, Active SP for the PVD object ($pvd_obj_id) ".
           "of drive $disk_id is ". uc(platform_shim_sp_get_property($active_sp_post_slf_restore, 'name'));
    $self->platform_shim_test_case_log_info($msg);

    $self->platform_shim_test_case_log_step("Verify zeroing continues for disk $disk_id on ".
                    uc(platform_shim_sp_get_property($active_sp_post_slf_restore, 'name')));
    test_util_general_set_target_sp($self, $active_sp_post_slf_restore);
    $disk_zero_pct = test_util_zeroing_wait_for_disk_zeroing_percent_increase($self, $disk, $disk_zero_pct, 1800);

}

sub set_cache
{
    my ($self, $set_sp_status) = @_;

	if ($set_sp_status eq "enable")
	{
		$self->platform_shim_test_case_log_info('Enabling Cache');
		platform_shim_device_enable_sp_cache($self->{device});		
	}else{
		$self->platform_shim_test_case_log_info('Disabling Cache');
		platform_shim_device_disable_sp_cache($self->{device});				
	}
	
    my %cache_state = platform_shim_device_get_sp_cache_info($self->{device});

    if (( $cache_state{write_state} ==  0 ) && ($set_sp_status eq "disable")) {
        $self->platform_shim_test_case_log_info('SP cache Disabled');
    }elsif (( $cache_state{write_state} ==  1 ) && ($set_sp_status eq "enable")) {
		$self->platform_shim_test_case_log_info('SP cache Enabled');
	}else{
        platform_shim_test_case_throw_exception('SP cache could not be ${set_sp_status}d!!');
    }
    
    foreach my $sp (platform_shim_device_get_sp($self->{device}))
    {
        platform_shim_sp_cache_clear_clean_pages($sp);  
    }

}


sub slf_initialize_rg_configs
{
   my ($self, $luns_per_rg, $lun_capacity_gb, $rgs_ref) = @_;
   my @rgs = @{$rgs_ref};
   my $existing_rgs = @rgs;
   if ($existing_rgs > 0) {
      $self->platform_shim_test_case_log_info("found $existing_rgs raid groups.  Not recreating config.");
   } elsif ($self->platform_shim_test_case_is_functional()) {
      #@rgs = @{test_util_configuration_create_all_raid_types($self)};
      @rgs = @{test_util_configuration_create_all_raid_types($self, {b_zeroed_only => 1})};
   } else {
       #@rgs = @{test_util_configuration_create_all_raid_types($self)};
       @rgs = @{test_util_configuration_create_one_random_raid_group($self, {b_zeroed_only => 1})};
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
      #$rg_config{disk_to_fault_index} = int(rand(scalar(@disks)));
      $rg_config{disks_to_fault} = [$disks[0]];#[$disks[$rg_config{disk_to_fault_index}]];


      my @luns = ();
      if ($existing_rgs > 0) {
         @luns = platform_shim_raid_group_get_lun($rg);
      } else {
         # Bind `luns_per_rg' plus 1
         foreach (1..($luns_per_rg)) {
            my $lun = test_util_configuration_create_lun($self, $rg, {size => $lun_capacity_gb."GB"});
            push @luns, $lun;
         }
      }
      my $num_luns = @luns;

      $rg_config{luns} = \@luns;

      # Push the test case
      push @rg_configs, \%rg_config;
   }
   # Return the address of the raid groups test configs created
   return \@rg_configs;
}

sub slf_start_io
{
    my ($self, $rg_config_ref, $mode) = @_;
    
    
    
    if ($mode =~ /fbe_lun/)
    {
        $self->slf_start_fbe_lun_io($rg_config_ref);
    }
    else
    {
        $self->slf_start_clar_lun_io($rg_config_ref);
    }
   

}

sub slf_start_clar_lun_io
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};
    my $device = $self->{device};
    
    
    $self->platform_shim_test_case_log_step("start rdgen to all luns");
    my $DEFAULT_START_LBA = 0xF001;
    my $TEST_ELEMENT_SIZE = 0x80;

    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        block_count => 0x80, 
                        use_block => 1,
                        start_lba => $DEFAULT_START_LBA,
                        minimum_lba => $DEFAULT_START_LBA,
                        minimum_blocks => $TEST_ELEMENT_SIZE,
                        );
    foreach my $rg_config (@rg_configs)
    {                    
        my @luns = @{$rg_config->{luns}};
        $rdgen_params{maximum_lba} = platform_shim_lun_get_property($luns[0], 'capacity_blocks');
        foreach my $lun (@luns)
        {
            # when sending through cache we need to supply the lun id
            $rdgen_params{clar_lun} = 1;
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
        }
    }

}

sub slf_start_fbe_lun_io
{
    my ($self, $rg_config_ref) = @_;
    my @rg_configs = @{$rg_config_ref};
    my $device = $self->{device};
    
    
    $self->platform_shim_test_case_log_step("start rdgen to all luns");
    my $DEFAULT_START_LBA = 0xF001;
    my $TEST_ELEMENT_SIZE = 0x80;
    
    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        block_count => 0x80, 
                        use_block => 1,
                        start_lba => $DEFAULT_START_LBA,
                        minimum_lba => $DEFAULT_START_LBA,
                        minimum_blocks => $TEST_ELEMENT_SIZE,
                        );
                        
   platform_shim_device_rdgen_start_io($device, \%rdgen_params);

}

sub slf_write_background_pattern
{
    my ($self) = @_;
    my $device = $self->{device};
    
    my $target_sp = platform_shim_device_get_target_sp($device);
    foreach my $sp (platform_shim_device_get_sp($device))
    {            
        test_util_general_set_target_sp($self, $sp);
        platform_shim_device_initialize_rdgen($device);
    }
    test_util_general_set_target_sp($self, $target_sp);
    
    test_util_io_write_background_pattern($self);
}

sub slf_cleanup
{
    my ($self) = @_;
    my $device = $self->{device};
    
    # Make sure rdgen is stopped since it opens an mcc volume.    
    platform_shim_device_stop_all_rdgen_io($device); 

    # Free rdgen memory
    platform_shim_device_cleanup_rdgen($device);
    
    # Disable performance statistics
    test_util_performance_stats_disable_sep_pp($self);    
}

1;
