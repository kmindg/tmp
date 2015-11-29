package mcr_test::test_util::test_util_disk;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_disk;
use strict;

sub test_util_disk_simulate_new_disk
{
	my ($tc, $disk_ref) = @_;
	
	my $device = $tc->{device};
	my @disks = @{$disk_ref};
	
	my $threshold = 60; #seconds
	
	my @sps = platform_shim_device_get_sp($device);
	platform_shim_device_disable_automatic_sparing($device);
	platform_shim_device_set_system_time_threshold($device, {threshold => $threshold.'S'});
	
	sleep(5);
	
	foreach my $disk (@disks)
	{
		platform_shim_disk_send_debug_control($sps[0], $disk, 0);
		platform_shim_disk_send_debug_control($sps[1], $disk, 0);
	}
	
	if (platform_shim_device_is_unified($tc->{device})) {
	    # the disk just disappears from uemcli and pvdinfo
	    # perhaps a property from c4admintool can be used. but easier to wait for AR556834
	} 
	else
	{
	    platform_shim_disk_wait_for_disk_removed($device, $disk_ref);
	}
	
	sleep($threshold);

	foreach my $disk (@disks)
	{
		platform_shim_disk_send_debug_control($sps[0], $disk, 1);
		platform_shim_disk_send_debug_control($sps[1], $disk, 1);
	}
	
	platform_shim_disk_wait_for_disk_ready($device, $disk_ref);
	platform_shim_device_enable_automatic_sparing($device);
	platform_shim_device_set_system_time_threshold($device, {});
	                   
}

sub test_util_disk_is_spindle
{
    my ($disk) = @_;
    my $technology = platform_shim_disk_get_property($disk, "technology");
    
    if ($technology =~ /normal/i) {
        return 1;
    }
    return 0;
}


sub test_util_disk_get_active_sp
{
    my ($self, $disk) = @_;

    my $active_sp = platform_shim_device_get_active_sp($self->{device}, {object => $disk});

    return $active_sp;
}

sub test_util_disk_is_psm_disk
{
    my ($tc, $disk) = @_;
    
    return (platform_shim_disk_get_property($disk, 'id') =~ /^(0_0_0|0_0_1|0_0_2)$/);
}

sub test_util_disk_is_vault_disk
{
    my ($tc, $disk) = @_;
    
    return (platform_shim_disk_get_property($disk, 'id') =~ /^(0_0_0|0_0_1|0_0_2|0_0_3)$/);
    
}

sub test_util_disk_get_pvd_info_hash
{
    my ($tc) = @_;
    my $device = $tc->{device};
    
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "pvdinfo -list", \%hash)};
    my $string = $ret_hash{stdout};
    my @lines = split /\n/, $string;
    my %disk_hash = ();
    
    foreach my $line (@lines)
    {
        if ($line =~ /(\w+)\s+(\d+\_\d+\_\d+)\s+(\w+)\s+(\w+)\s+(NONE|THIS SP|PEER SP|BOTH SPs)\s+(\w+)\s+(\d+)/) 
        {
            my $pvd_id = $1;
            my $bed = $2;
            my $drive_type = $3;
            my $eol = $4;
            my $slf = $5;
            my $zero_checkpoint = $6;
            my $zero_percent = $7;  

            my ($b, $e, $d) = split /\_/, $bed;
            $b = sprintf("%u", $b);
            $e = sprintf("%u", $e);
            $d = sprintf("%u", $d);
            
            $bed = $b."_".$e."_".$d;
            
            $disk_hash{$bed}{fbe_id} = $pvd_id;
            $disk_hash{$bed}{drive_type} = $drive_type;
            $disk_hash{$bed}{eol} = $eol; # can parse into 0 or 1
            $disk_hash{$bed}{slf} = $slf; # can parse further
            $disk_hash{$bed}{zero_checkpoint} = $zero_checkpoint;
            $disk_hash{$bed}{zero_percent} = $zero_percent;
        }
    }
    
    return \%disk_hash;
}

sub test_util_disk_is_spindown_qual
{
    my ($disk) = @_;
    
    my $device = $disk->getDevice(); 
    my $sp = platform_shim_device_get_target_sp($device);
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my $is_spindown_qual = 0;

    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "di $disk_id", \%hash)};
    my $string = $ret_hash{stdout};
    my @lines = split /\n/, $string;

    foreach my $line (@lines)
    {
        if ($line =~ /Spindown.*:(\w+)/) 
        {
            if ($1 eq "YES")
            {
                $is_spindown_qual = 1;
            }
            last;
        }
    }
    
    return $is_spindown_qual;
    
#    return (platform_shim_disk_get_property($disk, 'part_number') =~ /PWR/);    
}

1;
