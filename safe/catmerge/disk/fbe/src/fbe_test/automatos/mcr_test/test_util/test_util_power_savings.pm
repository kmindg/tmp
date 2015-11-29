package mcr_test::test_util::test_util_power_savings;

use strict;
use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_disk;


###  System related ###
sub test_util_power_savings_set_system_state_enabled
{
    my ($device) = @_;
    platform_shim_device_power_saving_set_system_state_enabled($device);
}

sub test_util_power_savings_set_system_state_disabled
{
    my ($device) = @_;
    platform_shim_device_power_saving_set_system_state_disabled($device);
}

sub test_util_power_savings_is_system_state_enabled
{
    my ($device) = @_;
    return platform_shim_device_power_saving_is_system_state_enabled($device);
}

### Raid Group related ###

sub test_util_power_savings_set_rg_enabled
{
    my ($rg) = @_;
    platform_shim_raid_group_power_savings_set_state_enabled($rg);
}

sub test_util_power_savings_set_rg_disabled
{
    my ($rg) = @_;
    platform_shim_raid_group_power_savings_set_state_disable($rg);
}

sub test_util_power_savings_get_rg_max_latency_time
{
    my ($rg) = @_;
    return platform_shim_raid_group_power_savings_get_max_latency_time($rg);
}

sub test_util_power_savings_is_rg_saving
{
    my ($rg) = @_;
    return platform_shim_raid_group_power_savings_is_saving($rg);
}

sub test_util_power_savings_get_rg_idle_time
{
    my ($rg) = @_;
    return platform_shim_raid_group_power_savings_get_idle_time($rg);
}

sub test_util_power_savings_set_rg_idle_time
{
    my ($rg, $time) = @_;
    return platform_shim_raid_group_power_savings_set_idle_time($rg, $time);
}



#todo:  hib value doesn't look correct. test before using
sub test_util_power_savings_get_rg_hibernation_time
{
    my ($rg) = @_;
    return platform_shim_raid_group_power_savings_get_hibernation_time($rg);
}


### Disk related ####
sub test_util_power_savings_set_disk_state_enable
{
    my ($sp, $disk) = @_;
    platform_shim_disk_power_savings_set_state_enable($sp, $disk);
}

sub test_util_power_savings_set_disk_state_disable
{
    my ($sp, $disk) = @_;
    platform_shim_disk_power_savings_set_state_disable($sp, $disk);
}

sub test_util_power_savings_is_disk_enabled
{
    my ($sp, $disk) = @_;
    return platform_shim_disk_power_savings_is_enabled($sp, $disk);
}

sub test_util_power_savings_set_disk_idle_time
{
    my ($sp, $disk, $idle_time) = @_;
    platform_shim_disk_power_savings_set_idle_time($sp, $disk, $idle_time);
}

sub test_util_power_savings_is_disk_capable
{
    my ($sp, $disk) = @_;
    return platform_shim_disk_power_savings_is_capable($sp, $disk);
}

sub test_util_power_savings_is_disk_saving
{
    my ($sp, $disk) = @_;
    return platform_shim_disk_power_savings_is_saving($sp, $disk);
}






