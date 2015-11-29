package mcr_test::experimental::pvd_block_size;

use base qw(mcr_test::platform_shim::platform_shim_test_case);
use mcr_test::test_util::test_util_general;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_sp;
use strict;
use warnings;


sub main
{
    my $self = shift;
    my $name = $self->platform_shim_test_case_get_name();
    $self->platform_shim_test_case_log_info("In the main of test case $name");
    my $device = $self->{device};
    
    my %disk_params =(state => "unused",
                      vault => 0);
    
    my @disks = platform_shim_device_get_disk($device, \%disk_params);
    my $num_4k_disks = int(scalar(@disks)/3);
    my @chosen_disks = test_util_general_n_choose_k_array(\@disks, $num_4k_disks);
    
    my $sp = platform_shim_device_get_active_sp($device, {});
    foreach my $disk (@chosen_disks)
    {
        # pvdsrvc -bes 0_0_10 -set_block_size 4160
        my %hash;
        my $cmd = "pvd_srvc -bes ".platform_shim_disk_get_property($disk, 'id')." -set_block_size 4160";
        platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
    }
    
}

1;