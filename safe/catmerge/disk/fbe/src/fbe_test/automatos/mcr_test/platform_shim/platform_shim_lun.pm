package mcr_test::platform_shim::platform_shim_lun;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_test_case;
use strict;

sub platform_shim_lun_get_property
{
	my ($lun, $property) =  @_;
	
	if (platform_shim_device_is_unified($lun->getDevice()))
	{
	    $property = "bind_offset" if ($property eq "address_offset");
	}

	return $lun->getProperty($property); 	
}

sub platform_shim_lun_get_capacity
{
    my ($lun) = @_;
    my $capacity = platform_shim_lun_get_property($lun, 'capacity_blocks');
    # The capacity return is DEC. convert to hex
    return sprintf("%#x", int($capacity));
}

sub platform_shim_lun_bind
{
	my ($raid_group, $params_ref) = @_;	
	my %params = %$params_ref;
	my $device = $raid_group->getDevice();
	if (platform_shim_device_is_unified($device))
	{
	    my @luns = ($raid_group->createBackendLun(%params));
	    return @luns;
	}
	
	my @luns =  $raid_group->createLun(%params);
	return @luns;
}

sub platform_shim_lun_unbind
{
	my ($lun) = @_;
	
	$lun->remove();
}

sub platform_shim_lun_get_zeroing_state
{
	my ($lun) = @_;
	
	my $device = $lun->getDevice();	
	my %params = (object => $lun);
	my $sp = platform_shim_device_get_active_sp($device, \%params);
	
    my $ref_hash;
    if (platform_shim_device_is_unified($lun->getDevice()))
    {
        ($ref_hash) = $sp->dispatch ('backendLunShowZero', %params);  
    }
    else
    {
        ($ref_hash) = $sp->dispatch ('lunShowZero', %params);  
    }
	
	my %zeroing_state;
	$zeroing_state{zero_checkpoint} = $ref_hash->{parsed}->{$params{object}->getProperty('fbe_id')}->{zero_checkpoint};
	$zeroing_state{zero_percent} = $ref_hash->{parsed}->{$params{object}->getProperty('fbe_id')}->{zero_percent};
	
	return \%zeroing_state;
}

sub platform_shim_lun_rdgen_start_io
{
	my ($lun, $params_ref) = @_;
	my %params = %$params_ref;
	if (platform_shim_device_is_unified($lun->getDevice()))
	{
	    if ($params{clar_lun}) {
	        $params{for_mcc} = delete $params{clar_lun} 
	    }
	}
	$lun->startRdGen(%params);
}

sub platform_shim_lun_rdgen_stop_io 
{
	my ($lun) = @_;
    $lun->stopRdGen();
}

sub platform_shim_lun_rdt
{
	my ($lun, $params_ref) = @_;
	my %params = %$params_ref;
	return $lun->raidDriverTester(%params);
}

sub platform_shim_lun_ndb
{
	my ($raid_group, $params_ref) = @_;
	
	my $device = $raid_group->getDevice();
	if (platform_shim_device_is_unified($device))
	{
	   return $raid_group->ndbBackendLun(%$params_ref);
	}
	
	my $ndbPassword = '1234';
	
	my %lunParams = (size                   => $params_ref->{size},
              		 lun_count              => 1,
              		 non_destructive_bind   => $ndbPassword,
              		 address_offset         => $params_ref->{address_offset},
                     poll                   => 1,
                     raid_type              => platform_shim_raid_group_get_property($raid_group, 'raid_type'));
	
    my @luns = platform_shim_lun_bind($raid_group, \%lunParams);	
    if (scalar(@luns) == 0) 
    {
	    platform_shim_test_case_throw_exception("NDB failed");
	}
    
	platform_shim_lun_mark_dirty($luns[0]);
    return $luns[0];
    
}

sub platform_shim_lun_mark_dirty
{
	my ($lun) = @_;
	$lun->markDirty();
}

sub platform_shim_lun_initiate_rw_background_verify
{
    my ($lun) = @_;
    my $device = $lun->getDevice();
    my %params = (object => $lun);
    my $sp = platform_shim_device_get_active_sp($device, \%params);
    platform_shim_device_set_target_sp($device, $sp);
    $lun->setBackgroundVerify(initiate_verify => 1);
    
}
sub platform_shim_lun_get_rw_verify_state
{
    my ($lun) = @_;
    my $device = $lun->getDevice();
    my %params = (object => $lun);
    my $sp = platform_shim_device_get_active_sp($device, \%params);
    platform_shim_device_set_target_sp($device, $sp);
    
    $params{type} = 1; # rw verify

    my $ref_hash;
    if (platform_shim_device_is_unified($device)) 
    {
        ($ref_hash) = $sp->dispatch ('backendLunShowBackgroundVerify', %params);   
    }
    else
    {
       ($ref_hash) = $sp->dispatch ('lunShowBackgroundVerify', %params); 
    }
    
    my %rw_verify_state;
    $rw_verify_state{verify_checkpoint} = $ref_hash->{parsed}->{$params{object}->getProperty('fbe_id')}->{background_verify_chkpt};
    $rw_verify_state{verify_percent} = $ref_hash->{parsed}->{$params{object}->getProperty('fbe_id')}->{background_verify_pct};
    
    return \%rw_verify_state;
}

sub platform_shim_lun_get_bv_pass_count
{
    my ($lun, $sp) = @_;    
    
    my $lun_id = platform_shim_lun_get_property($lun, "fbe_id");
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my %hash;

    my @cmd = ('fbe_api_object_interface_cli.exe', '-k', '-'.$sp_name,
                     '-sep', '-lun', 'getVerifyRep', $lun_id); 
                     
    platform_shim_sp_execute_command($sp, \@cmd, \%hash);
                                 
    return hex($hash{'Pass Count'});  
}

sub platform_shim_lun_get_bv_report
{
    my ($lun) = @_;   
    my $device = $lun->getDevice();
    my $raid_group = platform_shim_lun_get_raid_group($lun);
    my %params = (object => $raid_group);
    my $sp = platform_shim_device_get_active_sp($device, \%params);
    platform_shim_device_set_target_sp($device, $sp);
    
    my %report = $lun->getBackgroundVerify();
    my $lun_id = platform_shim_lun_get_property($lun, 'id');
    
    # normalize data since recent_report is a navi key and previous_report is fbecli
    $report{$lun_id}{recent_report} = $report{$lun_id}{previous_report} if ($report{$lun_id}{previous_report});
    # singlebitcrc_errors and multibitcrc_errors are checksum errors in navi - normalize for unity
    if (!(defined $report{$lun_id}{recent_report}{checksum_errors}))
    {
        $report{$lun_id}{recent_report}{checksum_errors}{correctable} = $report{$lun_id}{recent_report}{singlebitcrc_errors}{correctable} + $report{$lun_id}{recent_report}{multibitcrc_errors}{correctable};
        $report{$lun_id}{recent_report}{checksum_errors}{uncorrectable} = $report{$lun_id}{recent_report}{singlebitcrc_errors}{uncorrectable} + $report{$lun_id}{recent_report}{multibitcrc_errors}{uncorrectable};
        $report{$lun_id}{total_report}{checksum_errors}{correctable} = $report{$lun_id}{total_report}{singlebitcrc_errors}{correctable} + $report{$lun_id}{total_report}{multibitcrc_errors}{correctable};
        $report{$lun_id}{total_report}{checksum_errors}{uncorrectable} = $report{$lun_id}{total_report}{singlebitcrc_errors}{uncorrectable} + $report{$lun_id}{total_report}{multibitcrc_errors}{uncorrectable};
        $report{$lun_id}{previous_report}{checksum_errors}{correctable} = $report{$lun_id}{previous_report}{singlebitcrc_errors}{correctable} + $report{$lun_id}{previous_report}{multibitcrc_errors}{correctable};
        $report{$lun_id}{previous_report}{checksum_errors}{uncorrectable} = $report{$lun_id}{previous_report}{singlebitcrc_errors}{uncorrectable} + $report{$lun_id}{previous_report}{multibitcrc_errors}{uncorrectable};
        $report{$lun_id}{current_report}{checksum_errors}{correctable} = $report{$lun_id}{current_report}{singlebitcrc_errors}{correctable} + $report{$lun_id}{current_report}{multibitcrc_errors}{correctable};
        $report{$lun_id}{current_report}{checksum_errors}{uncorrectable} = $report{$lun_id}{current_report}{singlebitcrc_errors}{uncorrectable} + $report{$lun_id}{current_report}{multibitcrc_errors}{uncorrectable};
    }

    return %report;
}

sub platform_shim_lun_clear_bv_report($)
{
    my ($lun) = @_;
    my $device = $lun->getDevice();
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $lun});
    my $lun_objid = platform_shim_lun_get_property($lun, 'fbe_id');
    my $sp_name = platform_shim_sp_get_short_name($active_sp);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe', '-k', '-' . $sp_name,
        '-sep', '-lun', 'clearVerifyRep', $lun_objid);
    platform_shim_sp_execute_command($active_sp, \@cmd);
}

sub platform_shim_lun_get_lun_status
{
	my ($lun, $hash_ref) = @_;
	my %hash_data = %$hash_ref;
	return $lun->getLunStatus(%hash_data);
}

sub platform_shim_device_get_performance_statistics_summed
{
	my ($device, $hash_ref) = @_;
	my %data_hash = %$hash_ref;
	return $device->getPerformanceStatisticsSummed(%data_hash);
}

sub platform_shim_lun_map_lba($$)
{
    my ($lun, $lba) = @_;
    my $device = $lun->getDevice();
    my $object_id = platform_shim_lun_get_property($lun, 'fbe_id');
    my %params = (
        object => $lun,
        lba => $lba
    );
    
    my ($ret_hash);
    if (platform_shim_device_is_unified($device)) 
    {
        ($ret_hash) = $device->dispatch('backendLunMapLba', %params);
    }
    else
    {
        ($ret_hash) = $device->dispatch('lunMapLbaShow', %params);
    }
    return $ret_hash->{parsed}->{pba};
}

sub platform_shim_lun_wait_for_faulted_state
{
    my ($device, $lun_ref) = @_;
    
    if (platform_shim_device_is_unified($device)) 
    {
        return;
    }
    
    platform_shim_device_wait_for_property_value($device, {objects => $lun_ref, property => 'state', value => 'Faulted'});
}

sub platform_shim_lun_wait_for_ready_state
{
    my ($device, $lun_ref) = @_;
    
    if (platform_shim_device_is_unified($device)) 
    {
        platform_shim_device_wait_for_property_value($device, {objects => $lun_ref, property => 'state', value => 'READY'});
        return;
    }
    platform_shim_device_wait_for_property_value($device, {objects => $lun_ref, property => 'state', value => 'Bound'});
}

sub platform_shim_lun_get_raid_group
{
    my ($lun) = @_;
    return $lun->getRaidGroup();
    
}



1;
