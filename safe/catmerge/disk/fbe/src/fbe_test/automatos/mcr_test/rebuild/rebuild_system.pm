package mcr_test::rebuild::rebuild_system;

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
    
    my $lun_capacity_per_disk = 10;
    my $lun_capacity_gb;
    platform_shim_device_set_sparing_timer($device, 5*60);  
    if (platform_shim_device_is_simulator($self->{device}))
    {
        $lun_capacity_per_disk = 2;   
    }
    my $rg = test_util_configuration_create_raid_group_only_system_disks($self);
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
    $lun_capacity_gb = test_util_configuration_get_data_disks($self, $rg) * $lun_capacity_per_disk;
    my $lun1 = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
    my $lun2 = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
    my $lun3 = test_util_configuration_create_lun_size($self, $rg, $lun_capacity_gb."GB");
    
    my $psmrgid = platform_shim_device_get_psm_raid_group_id($device);
    my $psmlunid = platform_shim_device_get_psm_lun_id($device);
    my $vaultrgid = platform_shim_device_get_vault_raid_group_id($device);
    my $vaultlunid = platform_shim_device_get_vault_lun_id($device);
    my @psmdisks = ('0_0_0', '0_0_1','0_0_2');
    my $disk_id = $psmdisks[int(rand(scalar(@psmdisks)))];
    my ($disk) = platform_shim_device_get_disk($device, {id => $disk_id}); 
    
    my $fault = platform_shim_disk_remove($disk);
    $self->platform_shim_test_case_log_info("Waiting for removed state for disk $disk_id");
    platform_shim_disk_wait_for_disk_removed($device, [$disk]);
    
    my @lun_objects = platform_shim_raid_group_get_lun($rg);
    my $temp = join(', ',  map {platform_shim_lun_get_property($_, 'id')} @lun_objects);
    $self->platform_shim_test_case_log_step('Waiting for Faulted state for luns ' . $temp);
    platform_shim_device_wait_for_property_value($device, {objects => \@lun_objects, property => 'state', value => 'Faulted'});
    
    $self->platform_shim_test_case_log_step('Verify that rebuild logging starts for raid group '.$rg_id);                    
    platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                        raid_group => $rg}); 
                                                        
    foreach my $fbeid ($psmrgid, $vaultrgid) {
        $fbeid = sprintf("%#x", $fbeid) if ($fbeid !~ /x/i);
        $self->platform_shim_test_case_log_step("Verify rebuild logging started for system raid group $fbeid");
        platform_shim_event_verify_rebuild_logging_started({object => $device,
                                                            regex => '\(obj '.$fbeid.'\)'}); 
    }          
    
    $self->platform_shim_test_case_log_step("Recover disk faults");
    platform_shim_disk_reinsert($fault);
    
    # Wait for luns to be ready
        #my @lun_objects = platform_shim_raid_group_get_lun($test_case->{raid_group});
        #my $temp = join(', ',  map {platform_shim_lun_get_property($_, 'id')} @lun_objects);
    $self->platform_shim_test_case_log_info('Waiting for ready state for luns ' . $temp);
    platform_shim_device_wait_for_property_value($device, {objects => \@lun_objects, property => 'state', value => 'Bound'});
        
    # check that rebuild logging has stopped 
    $self->platform_shim_test_case_log_info("Verify that rebuild logging has stopped for raid group ".$rg_id);
    platform_shim_event_verify_rebuild_logging_stopped({object => $device,
                                                            raid_group => $rg});  
        
        

    $self->logStep("Verify that rebuild started on disk ".$disk_id);
    platform_shim_event_verify_rebuild_started({object => $device, disk => $disk});
    foreach my $fbeid ($psmrgid, $vaultrgid, $rg->getProperty("fbe_id")) {
        $fbeid = sprintf("%#x", $fbeid) if ($fbeid !~ /x/i);
        $self->logStep("Verify rebuild completed for raid group fbe_id $fbeid");
        platform_shim_event_verify_rebuild_completed({object => $device, 
                                                      disk => $disk,
                                                      regex => '\(obj '.$fbeid.'\)'}); 
    }                                              
                                                         
}


1;