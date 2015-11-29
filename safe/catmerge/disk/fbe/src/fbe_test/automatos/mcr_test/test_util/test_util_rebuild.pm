package mcr_test::test_util::test_util_rebuild;

use strict;
use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;

sub test_util_rebuild_wait_for_rebuild_complete
{
    my ($tc, $raid_group, $timeout) = @_;
    
    my $wait_until = time() + $timeout;
    my $raid_group_id = platform_shim_raid_group_get_property($raid_group, 'id');
    while (time() < $wait_until)
    {
       my $pct = platform_shim_raid_group_get_property($raid_group, 'rebuilt_percent');
        if ($pct == 100) {
            $tc->platform_shim_test_case_log_info("Rebuild completed for raid ".$raid_group_id);
            return;
        }  
        $tc->platform_shim_test_case_log_info("Rebuild Pct $pct for RAID Group: " . $raid_group_id);
        sleep(2);
    }    
    
    platform_shim_test_case_throw_exception("timed out waiting for rebuild to complete for $raid_group_id");

}

sub test_util_rebuild_wait_for_rebuild_percent_increase
{
    my ($tc, $raid_group, $percent, $timeout) = @_;
    my $device = $tc->{device};
    my $wait_until = time() + $timeout;
    my $raid_group_id = platform_shim_raid_group_get_property($raid_group, 'id');
    
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $raid_group});
    platform_shim_device_set_target_sp($device, $active_sp);
    while (time() < $wait_until)
    {
        my $rb_percent = platform_shim_raid_group_get_property($raid_group, 'rebuilt_percent');
        if ($rb_percent > $percent) {
            $tc->platform_shim_test_case_log_info("Rebuild percent increase from $percent to $rb_percent for rg ".$raid_group_id);
            return $rb_percent;
        } 
        elsif ($percent == 100 && $rb_percent == 100)
        {
            $tc->platform_shim_test_case_log_warn("Rebuild percent before: $percent after: $rb_percent for rg ".$raid_group_id);
            return $rb_percent;
        } 
        sleep(2);
    }    
    
    platform_shim_test_case_throw_exception("timed out waiting for rebuild to complete for $raid_group_id");
    
}

sub test_util_rebuild_wait_for_rebuild_checkpoint_increase
{
    my ($tc, $raid_group, $checkpoint_str, $timeout, $active_sp) = @_;
    my $timeout_sec = time + $timeout;
    my $device = $tc->{device};
    
    my $wait_until = time() + $timeout;
    my $raid_group_id = platform_shim_raid_group_get_property($raid_group, 'id');
    
    while (!defined $active_sp) 
    {
        eval {
            $active_sp = platform_shim_device_get_active_sp($device, {object => $raid_group});
        };
        if ($@) {
            $tc->platform_shim_test_case_log_warn( "$@\n");
        }
    }

    platform_shim_device_set_target_sp($device, $active_sp);
    my $checkpoint = Math::BigInt->new($checkpoint_str);
    while (time() < $wait_until)
    {
        
        my $rb_checkpoint_str = platform_shim_raid_group_get_property($raid_group, 'rebuild_checkpoint');
        if ($rb_checkpoint_str =~ /0xFFFFFFFFFFFFFFFF/i)
        {
            $tc->platform_shim_test_case_log_warn("Rebuild checkpoint already $rb_checkpoint_str for rg ".$raid_group_id);
            return $rb_checkpoint_str;
        }
        
        my $rb_checkpoint = Math::BigInt->new($rb_checkpoint_str);
        if ($rb_checkpoint > $checkpoint) 
        {
            $tc->platform_shim_test_case_log_info("Rebuild checkpoint increase from $checkpoint_str to $rb_checkpoint_str for rg ".$raid_group_id);
            return $rb_checkpoint_str;
        } 
        sleep(2);
    }    
    
    platform_shim_test_case_throw_exception("Timed out waiting for rebuild to complete for $raid_group_id");
}


sub test_util_rebuild_set_speed
{
    my ($tc, $speed) = @_;
    
    my @sps = platform_shim_device_get_sp($tc->{device});
    foreach my $sp (@sps)
    {
        platform_shim_sp_set_background_op_speed($sp, 'RG_REB', $speed);
    }
}

sub test_util_rebuild_restore_speed
{
    my ($tc) = @_;
    test_util_rebuild_set_speed($tc, 120);
}

sub test_util_rebuild_is_ntmirror_complete
{
    my ($tc, $sp) = @_;
    
    if (platform_shim_device_is_unified($tc->{device})) {
        #22:40:12 root@BC-H1412-spb spb:~> sptool -mirror -rebuildstatus
        #MirrorIoctl: Device \Device\Harddisk0\DR0 opened w/name SUCCESS
        #MirrorIoctl: IOCTL_NTMIRROR_REBUILD_STATUS issued successfully...
        #MirrorIoctl:
        #    RebuildInProgress:      FALSE
        #    RebuildRequired:        FALSE
        my $output = platform_shim_sp_execute_command($sp, ['sptool', '-mirror', '-rebuildstatus'], undef);
        my @lines = split /\n/, $output;
        
        foreach my $line (@lines) {
            if ($line =~ /RebuildRequired\:\s+FALSE/i) {
                return 1;
            }
        }
    
    }
    else 
    {
    
    
        #C:\>spstate -ntmstatus
        #=======================================
        #Rebuild Status of Flare Boot Partition
        #
        #No Rebuild Required
        #
        #C:\>spstate -ntmstatus
        #
        #=======================================
        #Rebuild Status of Flare Boot Partition
        #
        #Rebuild is in progress on Disk 2
        #Rebuilt: 0%
        #Rebuild Checkpoint: 0b8fb6a2
        
        
        my $output = platform_shim_sp_execute_command($sp, ['spstate', '-ntmstatus'], undef);
        my @lines = split /\n/, $output;
        
        foreach my $line (@lines) {
            if ($line =~ /No Rebuild Required/i) {
                return 1;
            }
        }
    }
    
    return 0;
    
}

1;
