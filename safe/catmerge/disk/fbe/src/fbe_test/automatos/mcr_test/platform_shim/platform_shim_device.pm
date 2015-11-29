package mcr_test::platform_shim::platform_shim_device;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use strict;
use mcr_test::platform_shim::platform_shim_sp;

# id => { type => SCALAR, optional => 1 },
# state => { type => SCALAR, optional => 1 },
# buses => { type => ARRAYREF, optional => 1 },
# not_buses => { type => ARRAYREF, optional => 1 },
# bus_enclosures => { type => ARRAYREF, optional => 1 },
# not_bus_enclosures => { type => ARRAYREF, optional => 1 },
# kind => { type => ARRAYREF, optional => 1 },
# technology => { type => ARRAYREF, optional => 1 },
# disk_count => { type => SCALAR, optional => 1 },
# power_saving_capable => { type => BOOLEAN, optional => 1 },
# vault => {type => BOOLEAN, optional => 1},
# self_encrypting  => {type => BOOLEAN, optional => 1 },
# same_type => {type => BOOLEAN, default => 0 },
# same_capacity => {type => BOOLEAN, default => 0 },
sub platform_shim_device_get_disk
{
	my ($device, $params_ref) = @_;
	my %params = %$params_ref;
	
	if (platform_shim_device_is_unified($device))
	{
	    $params{'b_e_s'} = delete $params{'id'} if ($params{'id'});
	}

	return $device->getDisk(%params);
}

sub platform_shim_device_get_system_disk
{
    my ($device) = @_;
    return $device->getVaultDisk(); 
}

sub platform_shim_device_get_raid_group
{
    my ($device, $params_ref) = @_;
    if (defined $params_ref) 
    {
        my %params = %$params_ref;
        return $device->getRaidGroup(%params);
    } 
    else 
    {
        return $device->getRaidGroup();
    }
}

sub platform_shim_device_get_system_raid_groups($$)
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    $params{is_private} = 1;
    return $device->find(type => 'RaidGroup', criteria => \%params);
}

sub platform_shim_device_get_system_luns($$)
{
    # TODO: this code will return undef because we don't have
    # LUN objects for system luns.
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    $params{is_private} = 1;
    return $device->find(type => 'Lun', criteria => \%params);
}

sub platform_shim_device_get_sp {
	my ($device) = @_;
	
	my @sps = $device->getSp();	
	return @sps;
}

sub platform_shim_device_get_active_sp
{
	my ($device, $params_ref) = @_;
	my %params = %$params_ref;
	return $device->getActiveSp(%params);
}

sub platform_shim_device_get_passive_sp
{
	my ($device, $params_ref) = @_;
	my %params = %$params_ref;
	return $device->getPassiveSp(%params);
}

sub platform_shim_device_get_target_sp
{
    my ($device) = @_;
    if (platform_shim_device_is_unified($device))
    {
        return ($device->getDispatchSystem())->getComponentObject();   
    } 
    else 
    {
        return $device->getPrimarySp();
    }
}

sub platform_shim_device_set_target_sp
{
    my ($device, $sp) = @_;

    if (platform_shim_device_is_unified($device))
    {
        return $device->setDispatchSystem({sp => $sp});   
    } 
    else 
    {
        return $device->setPrimarySp($sp);
    }
}

# 
# thread_count => { type => SCALAR },
# block_count => { type => SCALAR, optional => 1 },
# io_count => { type => SCALAR, optional => 1 },
# duration => {
#     callbacks => { 'time string' => timeUnit() },
#     optional => 1,
# },
# object =>
#  {
#      isa      => 'Automatos::Component::Lun::Emc::Vnx',
#      optional => 1
#  },
# object_fbe_index    => { type => SCALAR, optional => 1 },
# sequential_increasing_lba => { type => BOOLEAN, optional => 1 },
# sequential_decreasing_lba => { type => BOOLEAN, optional => 1 },
# repeat_lba          => { type => BOOLEAN, optional => 1 },
# sequential_increasing_caterpillar => { type => BOOLEAN, optional => 1 },
# abort               => { type => BOOLEAN, optional => 1 },
# abort_time  => {
#      callbacks => { 'time string' => timeUnit() },
#      optional => 1,
# },
# available_for_writes=> { type => BOOLEAN, optional => 1 },
# available_for_reads => { type => BOOLEAN, optional => 1 },
# use_block           => { type => BOOLEAN, optional => 1 },
# read_only           => { type => BOOLEAN, optional => 1 },
# write_only          => { type => BOOLEAN, optional => 1 },
# write_read_check    => { type => BOOLEAN, optional => 1 },
# read_check          => { type => BOOLEAN, optional => 1 },
# zero_fill           => { type => BOOLEAN, optional => 1 },
# zero_fill_read_check=> { type => BOOLEAN, optional => 1 },
# verify           => { type => BOOLEAN, optional => 1 },
# increment_lba     => { type => SCALAR, optional => 1 },
# increment_blocks => { type => SCALAR, optional => 1 },
# minimum_blocks   => { type => SCALAR, optional => 1 },
# minimum_lba       => { type => SCALAR, optional => 1 },
# maximum_lba       => { type => SCALAR, optional => 1 },
# start_lba         => { type => SCALAR, optional => 1 },
# align             => { type => BOOLEAN, optional => 1 },
# performance       => { type => BOOLEAN, optional => 1 },
# affinity_spread   => { type => BOOLEAN, optional => 1 },
# affine            => { type => SCALAR, optional => 1 },
# peer_options     => { type => SCALAR, optional => 1 },
# pass_count        => { type => SCALAR, optional => 1 },
# non_cached => { type => BOOLEAN, optional => 1 },
# clar_lun => { type => BOOLEAN, optional => 1 },
#pattern_zeros =>  { type => BOOLEAN, optional => 1},   
#pattern_clear =>  { type => BOOLEAN, optional => 1},

sub platform_shim_device_rdgen_start_io
{
	my ($device, $params_ref) = @_;
	my %params = %$params_ref;
	if (platform_shim_device_is_unified($device)) 
	{
	    $params{clariion_lun} = delete $params{clar_lun} if ($params{clar_lun});
	    $device->dispatch('backendLunRdGenStart', %params);   
	}
	else
	{
	    $device->dispatch('lunRdGenStart', %params);   
	}
		
}

sub platform_shim_device_rdgen_stop_io
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    if (platform_shim_device_is_unified($device)) 
    {
        $device->dispatch('backendLunRdGenStop', %params); 
    }
    else
    {
        $device->dispatch('lunRdGenStop', %params);   
    } 
}

sub platform_shim_device_stop_all_rdgen_io
{
    my ($device) = @_;
	my @sps = platform_shim_device_get_sp($device);
	foreach my $sp (@sps) { 
		my %params = (state => 1);
		$sp->dispatch('deviceRdGenStop');
	}
}

sub platform_shim_device_enable_automatic_sparing
{
	my ($device) = @_;

	my @sps = platform_shim_device_get_sp($device);
	foreach my $sp (@sps) { 
		my %params = (state => 1);
		$sp->dispatch('deviceAutomaticHotSpareSet', %params);
	}
}

sub platform_shim_device_disable_automatic_sparing
{
	my ($device) = @_;

	my @sps = platform_shim_device_get_sp($device);
	foreach my $sp (@sps) { 
		my %params = (state => 0);
		$sp->dispatch('deviceAutomaticHotSpareSet', %params);
	}
}


sub platform_shim_device_check_error_trace_counters
{
	my ($device) = @_;
	
	# reenable after the mega merge: set check errors back to 1
	return $device->getErrorTraceCounters(check_for_errors => 0);
}

sub platform_shim_device_clear_error_trace_counters
{
    my ($device) = @_;

    my %hash;
    my $cmd = "error_trace_counters -clear";
    foreach my $sp (platform_shim_device_get_sp($device))
    {
        platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
    }
}

sub platform_shim_device_set_system_time_threshold
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
   
    $device->dispatch('deviceSetSystemTimeThreshold', %params);
}

sub platform_shim_device_enable_load_balancing
{
    my ($device) = @_;
    my %params = (state => 1);
    my @sps = platform_shim_device_get_sp($device);
    
    map {$_->dispatch('deviceSetLoadBalance', %params)} @sps;
}

sub platform_shim_device_disable_load_balancing
{
    my ($device) = @_;
    my %params = (state => 0);
    my @sps = platform_shim_device_get_sp($device);
   
    map {$_->dispatch('deviceSetLoadBalance', %params)} @sps;
}

sub platform_shim_device_enable_neit
{
    my ($device) = @_;
    $device->enableNeit();
}

sub platform_shim_device_enable_logical_error_injection
{
    my ($device) = @_;
    my %params = (state => 1);
   
    $device->setLogicalErrorInjection(%params);
    
}

sub platform_shim_device_disable_logical_error_injection
{
    my ($device) = @_;
    my %params = (state => 0);
   
    $device->setLogicalErrorInjection(%params);
    
}

sub platform_shim_device_disable_logical_error_injection_records
{
    my ($device) = @_;
    
    my $max_records = 256;
    $device->disableLogicalErrorInjectionRecords( clear_record_count => $max_records);

}

sub platform_shim_device_create_logical_error_injection_record
{
    my ($device, $params_ref) = @_;
    $device->createLogicalErrorInjectionRecord(%$params_ref);
}

sub platform_shim_device_create_logical_error_injection_record_for_fbe_object_id
{
    my ($device, $params_ref, $object_fbe_id) = @_;
    $params_ref->{object_fbe_id} = $object_fbe_id;
    $device->dispatch('deviceCreateLogicalErrorInjectionRecords', %$params_ref);
}

sub platform_shim_device_create_logical_error_injection_record_for_object
{
    my ($device, $params_ref, $object) = @_;
    $params_ref->{object} = $object;
    $device->createLogicalErrorInjectionRecord(%$params_ref);
}

sub platform_shim_device_delete_logical_error_injection_record
{
    my ($device, $params_ref) = @_;
    deleteLogicalErrorInjectionRecord(%$params_ref);
}

sub platform_shim_device_enable_logical_error_injection_object
{
    my ($device, $object) = @_;
    my %params = (object => $object, enabled => 1);
    $device->setLogicalErrorInjectionObject(%params);
}

sub platform_shim_device_disable_logical_error_injection_object
{
    my ($device, $object) = @_;
    my %params = (object => $object, enabled => 0);
    $device->setLogicalErrorInjectionObject(%params);
}

sub platform_shim_device_enable_logical_error_injection_fbe_object_id
{
    my ($device, $object_fbe_id) = @_;
    my %params = (object_fbe_id => $object_fbe_id, enabled => 1);
    
    $device->dispatch('deviceSetLogicalErrorInjectionObject', %params)
}

sub platform_shim_device_disable_logical_error_injection_fbe_object_id
{
    my ($device, $object_fbe_id) = @_;
    my %params = (object_fbe_id => $object_fbe_id, enabled => 0);
    
    $device->dispatch('deviceSetLogicalErrorInjectionObject', %params)
}

sub platform_shim_device_get_logical_error_records
{
    my ($device, $sp) = @_;
    my %params = (active_sp => $sp);
    return $device->getLogicalErrorInjectionRecords(%params);
}

sub platform_shim_device_get_logical_error_injection_statistics
{
    my ($device, $sp) = @_;
    my %params = (active_sp => $sp);
    return $device->getLogicalErrorInjectionStatistics(%params);
}

sub platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id
{
    my ($device, $sp, $object_fbe_id) = @_;
    my %params = (object_fbe_id => $object_fbe_id);
    
    my ($statsInfo) = $sp->dispatch('deviceShowLogicalErrorInjectionStatistics', %params);
    return %{$statsInfo->{parsed}};
}

sub platform_shim_device_get_logical_error_injection_statistics_for_object
{
    my ($device, $sp, $object) = @_;
    my %params = (active_sp => $sp, object => $object);
    return $device->getLogicalErrorInjectionStatistics(%params);
}


sub platform_shim_device_initiate_rw_background_verify
{
    my ($device) = @_;
    
    if (platform_shim_device_is_unified($device))
    {
        my %params = ('initiate_verify' => 1);
        $device->dispatch('deviceSetBackgroundVerify', %params);
        return;
    }
    
    $device->setBackgroundVerify('initiate_verify' => 1);
}

sub platform_shim_device_initialize_rdgen
{
    my ($device) = @_;
    
    # @note If memory wasn't allocated this call fails the test.
    #       Therefore we cannot cleanup.
    #$device->releaseRdGenDpsMemory();

    # set dps mem size
    eval {$device->initializeRdGen(memory => '72MB')};

    # reset statistics
    $device->resetRdGenStatistics();
}

# $generatedIoInfo{thread_count} = $1;
# $generatedIoInfo{object_count} = $1;
# $generatedIoInfo{request_count} = $1;
# $generatedIoInfo{memory_size} = $1 . ' MB';
# $generatedIoInfo{memory_type} = $1;
# $generatedIoInfo{io_count} = $1;
# $generatedIoInfo{pass_count} = $1;
# $generatedIoInfo{error_count} = $1;
# $generatedIoInfo{media_error_count} = $1;
# $generatedIoInfo{aborted_error_count} = $1;
# $generatedIoInfo{invalidated_block_count} = $1;
# $generatedIoInfo{bad_crc_block_count} = $1;
# $generatedIoInfo{io_failure_error_count} = $1;
# elsif ($line =~ /^Historical\s*stat/i) {
#             $generatedIoInfo{historic_data}{io_count} = $1;
#             $generatedIoInfo{historic_data}{pass_count} = $1;
#             $generatedIoInfo{historic_data}{aborted_error_count} = $1;
#             $generatedIoInfo{historic_data}{bad_crc_block_count} = $1;
#             $generatedIoInfo{historic_data}{io_failure_error_count} = $1;
#             $generatedIoInfo{historic_data}{invalidated_request_error_count} = $1;
#             $generatedIoInfo{historic_data}{request_from_peer_count} = $1;
#             $generatedIoInfo{historic_data}{request_to_peer_count} = $1;
#             $generatedIoInfo{historic_data}{error_count} = $1;
#             $generatedIoInfo{historic_data}{media_error_count} = $1;
#             $generatedIoInfo{historic_data}{invalidated_block_count} = $1;
sub platform_shim_device_rdgen_stats
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    
    if (platform_shim_device_is_unified($device)) 
    {
        my ($retHash) = $device->dispatch('backendLunRdGenShow', %params);
        return %{$retHash->{parsed}};
    } 
    else
    {
        my ($retHash) = $device->dispatch('lunRdGenShow', %params);
        return %{$retHash->{parsed}};
    }
}

sub platform_shim_device_insert_log_message
{
    my ($device, $msg) = @_;
    $device->spInsertMsg(log_option => 'both_msg',
                           msg_type   => 'info',
                           msg_text   => $msg
                           );
}

sub platform_shim_device_wait_for_property_value
{
    my ($device, $params_ref) = @_;
    
    $device->waitForPropertyValue(%$params_ref);
}

sub platform_shim_device_set_sparing_timer
{
    my ($device, $timer) = @_;
    $device->setSparingTimer(timer => $timer."S");
}

sub platform_shim_device_recover_from_faults
{
    my ($device) = @_;
    $device->recoverFromFaults();
}

sub platform_shim_device_cleanup_rdgen
{
    my ($device) = @_;

    # Release previously allocated memory
    eval {$device->releaseRdGenDpsMemory()};
}

sub platform_shim_device_get_attached_host
{
    my $device = shift;
    return ($device->getAttachedHost());
}

sub platform_shim_device_set_performance_statistics{
    my ($device, $hash_ref) =  @_;
    my %data_hash = %$hash_ref;

    $device->setPerformanceStatistics(%data_hash);
}

sub platform_shim_device_get_performance_statistics_summed
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    return $device->getPerformanceStatisticsSummed(%params);
}

sub platform_shim_device_clear_performance_statistics
{
    my ($device, $params_ref) = @_;
    my %params = %$params_ref;
    $device->clearPerformanceStatistics(%params);
}

sub platform_shim_device_disable_storagegroups
{
    my $device = shift;
	$device->disableStorageGroups();
}

sub platform_shim_device_recover_fault
{
    my ($device) = @_;
	return $device->recover();
}

sub platform_shim_device_get_sniff_verify
{
    my $self = shift;
    return $self->getSniffVerify();
}

sub platform_shim_device_set_sniff_verify
{
    my ($self, $hash_ref) = @_;
    my %hash_data = %$hash_ref;
    return $self->setSniffVerify(%hash_data);
}

sub platform_shim_device_rdgen_reset_stats
{
    my ($device) = @_;
    
    # reset statistics
    $device->resetRdGenStatistics();
}

sub platform_shim_device_is_simulator
{
    my ($device) = @_;

    my $sp = platform_shim_device_get_target_sp($device);
    
    if (platform_shim_device_is_unified($device)) {
       my @sps = $device->getSp();
       my $sp_obj = $sps[0]->getHostObject();
       my %plat = $sp_obj->getPlatformType();
       my $platform = $plat{platform};
       print "platform is: $platform\n";
       if ($platform =~ /SIM/i) {
          return 1;
       } else {
          return 0;
       }
    } else {
       my $sn = $sp->getProperty("serial_number");
       if ($sn =~ /VSA/i)
       {
           return 1; 
       }
       else 
       {
           return 0;   
       }
    }
}

sub platform_shim_device_get_psm_raid_group_id
{
    my ($device) = @_;
    
    return $device->getPsmRaidGroupId();
}

sub platform_shim_device_get_psm_lun_id
{
    my ($device) = @_;
    
    return $device->getPsmLunId();
}

sub platform_shim_device_get_vault_raid_group_id
{
    my ($device) = @_;
    
    return $device->getVaultRaidGroupId();
}

sub platform_shim_device_get_vault_lun_id
{
    my ($device) = @_;
    
    return $device->getVaultLunId();
}

sub platform_shim_device_get_downstream_objects_for_fbe_object_id
{
    my ($device, $object_id) = @_;
    my @downstream_fbe_ids = ();
            
    my %params = (object_fbe_id => $object_id);
    my ($ret_hash) = $device->dispatch('deviceGetDownstreamObject', %params);
        @downstream_fbe_ids = split(/\s+/, $ret_hash->{parsed}{downstream_object_list});

    return \@downstream_fbe_ids;
}

sub platform_shim_device_clear_eol_faults
{
    my ($device) = @_;
    $device->clearEndOfLifeOnDisks();
    
}

sub platform_shim_device_is_unified
{
    my ($device) = @_;
    if ($device->isa('Automatos::Device::Storage::Emc::Unified::Vnxe'))
    {
        return 1;
    }
    return 0;
}

sub platform_shim_device_wait_for_ecom
{
    my ($device, $timeout) = @_;

    if (!platform_shim_device_is_unified($device)){ 
       return;
    }
    
    my @sps = $device->getSp();
    my $wait_until = $timeout + time();
    
    while (time() < $wait_until)
    {
        foreach my $sp (@sps)
        {
            if ($sp->checkIfMasterSp())
            {
                $sp->waitForEcomReady(timeout => "$timeout S");
                return;
            }
            
            sleep(5);
        }
    }    
    Automatos::Exception::Base->throw("Timed out waiting for ecom"); 
}

sub platform_shim_device_disable_sp_cache
{
    my ($device) = shift;	

    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($device))
    {
        $device->setMccCache({enabled => 0});
    }
    else
    {
        my %data_hash = (write_state => 0);
        $device->setSpCache(%data_hash);
    }
}

sub platform_shim_device_enable_sp_cache
{
    my ($device) = shift;

    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($device))
    {
        $device->setMccCache({enabled => 1});
    }
    else
    {
        my %data_hash = (write_state => 1);
        $device->setSpCache(%data_hash);
    }
}

sub platform_shim_device_get_sp_cache_info
{
    my ($device) = @_;
	return $device->getSpCacheInfo();
}

sub platform_shim_device_power_saving_set_system_state_enabled
{
    my ($device) = @_;
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -state enable", \%hash)};
}

sub platform_shim_device_power_saving_set_system_state_disabled
{
    my ($device) = @_;
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -state disable", \%hash)};
}

sub platform_shim_device_power_saving_is_system_state_enabled
{
    my ($device) = @_;
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -state state", \%hash)};

    my $string = $ret_hash{stdout};
    my @lines = split /\n/, $string;
    my $capacity;
    my %rg_info_hash = ();
    my @rb_chkpt;
    my $index = 0;
    foreach my $line (@lines) {
        if ($line =~ /System Power Save is (\w+)/)
        {
            if ($1 eq "Enabled")
            {
                return 1;
            }
            return 0;           
        }
    }
    Automatos::Exception::Base->throw("cmd failed: fbecli> green -state state"); 
}

1;
