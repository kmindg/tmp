package mcr_test::test_util::test_util_verify;

use strict;
use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_disk;

sub test_util_verify_wait_for_lun_rw_bv_complete
{
    my ($tc, $lun, $timeout) = @_;
    my $wait_until = time() + $timeout;
    my $lun_id = platform_shim_lun_get_property($lun, 'id');
    
    while (time() < $wait_until)
    {
        my $rw_verify_state = platform_shim_lun_get_rw_verify_state($lun);
        
        if ($rw_verify_state->{verify_percent} == 100)
        {
            $tc->platform_shim_test_case_log_info("RW verify completed for lun ".$lun_id);
            return;
        }
        sleep(2);
        
    }
    $tc->platform_shim_test_case_throw_exception("timed out waiting for rw verify to complete for lun $lun_id");
}

sub test_util_verify_wait_for_rw_verify_percent_increase
{
    my ($tc, $lun, $percent, $timeout) = @_;
    my $wait_until = time() + $timeout;
    my $lun_id = platform_shim_lun_get_property($lun, 'id');
    
    while (time() < $wait_until)
    {
        my $rw_verify_state = platform_shim_lun_get_rw_verify_state($lun);
        
        if ($rw_verify_state->{verify_percent} > $percent)
        {
            $tc->platform_shim_test_case_log_info("RW verify increased to ".
                        $rw_verify_state->{verify_percent}."% for lun ".$lun_id);
            return $rw_verify_state->{verify_percent};
        }
        elsif ($percent == 100 && $rw_verify_state->{verify_percent} == 100)
        {
            $tc->platform_shim_test_case_log_warn("Verify percent before: $percent after: ".$rw_verify_state->{verify_percent}." for lun ".$lun_id);
            return $rw_verify_state->{verify_percent};
        } 
        sleep(2);
        
    }
    $tc->platform_shim_test_case_throw_exception("timed out waiting for rw verify to complete for lun $lun_id");
    
}

sub test_util_verify_get_lun_total_bv_pass_count
{
    my ($tc, $lun) = @_;
    
    my $device = $tc->{device};
    my @sps = platform_shim_device_get_sp($device);
    my $pass_count = 0;
    foreach my $sp (@sps) 
    {
        $pass_count += platform_shim_lun_get_bv_pass_count($lun, $sp);
    }
    
    return $pass_count;
}

sub test_util_verify_wait_for_group_user_verify_checkpoint_reset
{
    my ($tc, $rg, $timeout) = @_;
    
    my $wait_until = time() + $timeout;
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');  
    my $sp = platform_shim_device_get_active_sp($tc->{device}, {object => $rg});
    
    my $rg_fbe_id;
    if (platform_shim_raid_group_get_property($rg, 'raid_type') eq 'r10')
    {
        my @mirrors = @{platform_shim_raid_group_get_downstream_mirrors($rg)};
        $rg_fbe_id = $mirrors[0];
    } 
    else 
    {
        $rg_fbe_id = platform_shim_raid_group_get_property($rg, 'fbe_id');
    }
    
    while (time() < $wait_until)
    {
        my $rg_info_hash = platform_shim_raid_group_get_rg_info($sp, $rg_fbe_id);
        
        if ($rg_info_hash->{rw_verify_checkpoint} =~ /^(0|0x0)$/i)
        {
            $tc->platform_shim_test_case_log_info("RW verify 0x0 for rg ".$rg_id);
            return;
        }
        sleep(2);
        
    }
    
    $tc->platform_shim_test_case_throw_exception("timed out waiting for rw verify to reset for rg $rg_id");
    
    
}

1;
