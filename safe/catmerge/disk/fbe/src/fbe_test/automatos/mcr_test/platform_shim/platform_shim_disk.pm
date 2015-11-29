package mcr_test::platform_shim::platform_shim_disk;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_device;
use strict;


#put all Fbeapix wrapper related properties here
#List each individually in case we need to inherit it in other objects
# my %diskProperties = (
#    vendor => {},
#    id => {},
#    fbe_id => {getmethod => 'diskShowObjectId'},                   #pvd id
#    pdo_fbe_id => {getmethod => 'diskShowObjectPdoFbeId'}, 
#    vd_fbe_id => {getmethod => 'diskShowVd'},                      #FbeAPIX specific property
#    product_id => {},
#    fbe_capacity => {getmethod => 'diskShowProvisionDriveInfo'},
#    product_revision => {},
#    serial_number => {},
#    clariion_part_number => {},
#    block_count => {},
#    block_size => {},
#    bridge_hw_revision => {},
#    size => {},
#    description => {},
#    drive_type => {},
#    drive_vendor => {},
#    price_class => {},
#    queue_depth => {},
#    revision_length => {},
#    speed => {},
#    exported_size => {getmethod => 'diskShowVerifyStatus'},
#    verify_percent => {getmethod => 'diskShowVerifyStatus'},
#    verify_checkpoint => {getmethod => 'diskShowVerifyStatus'},
#    verify_enable => {getmethod => 'diskShowVerifyStatus'},
#    zero_checkpoint => {getmethod => 'diskShowZeroCheckpoint'},
#    zero_percent => {getmethod => 'diskShowZeroPercent'},
#    cumulative_error_tag => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    cumulative_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    link_error_tag => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    link_error_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    data_error_tag => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    data_error_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    reset_error_tag =>{getmethod => 'diskShowErrorHandleHealthInfo'},
#    reset_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    hardware_error_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    hardware_error_tag   => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    io_count => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    error_count => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    media_error_tag => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    media_error_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    power_cycle_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    recovered_error_ratio => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    recovered_error_tag => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    remap_block_count =>  {getmethod => 'diskShowErrorHandleHealthInfo'},
#    reset_count => {getmethod => 'diskShowErrorHandleHealthInfo'},
#    is_write_cache_enabled => {getmethod => 'diskIsWriteCacheEnabled'},
#    eol_state => {getmethod => 'diskShowProvisionDriveInfo',
#                  setmethod => 'diskSetEndOfLife'},
#    default_offset => {getmethod => 'diskShowProvisionDriveInfo'},
#    is_single_loop_failure => {getmethod => 'diskIsSingleLoopFailure'},
#    rebuild_checkpoint => {getmethod => 'diskShowRebuild'},
#    rebuild_logging => {getmethod => 'diskShowRebuild'},

sub platform_shim_disk_get_property
{
	my ($disk, $property) =  @_;

    if (platform_shim_device_is_unified($disk->getDevice()))
    {
        $property = 'b_e_s' if ($property eq 'id');
    }
    
	return $disk->getProperty($property); 	
}

sub platform_shim_disk_zero_disk
{
	my ($disk) = @_;
	
	$disk->zeroDisk();	
}

sub platform_shim_disk_remove
{
	my ($disk) = @_;
	return $disk->getSlot()->bypass();
}

sub platform_shim_disk_remove_on_sp
{
    my ($disk, $sp) = @_;
    return $disk->getSlot()->bypass(sp => $sp);
}

sub platform_shim_disk_reinsert
{
	my ($diskFault) = @_;
	$diskFault->recover();
}

sub platform_shim_disk_wait_for_disk_state
{
	my ($device, $disk_ref, $state) = @_;
	
	$device->waitForPropertyValue(objects => $disk_ref,
                                  property => 'state',
                                  value => $state);
                                      
}

sub platform_shim_disk_wait_for_disk_ready
{
	my ($device, $disk_ref) = @_;
	$device->waitForPropertyValue(
								objects          => $disk_ref,
								property         => 'state',
								timeout 		 => '2 M',
								value_expression => qr/Enabled|Unbound/i	);
}

sub platform_shim_disk_wait_for_disk_removed
{
    my ($device, $disk_ref) = @_;
    if (platform_shim_device_is_unified($device))
    {
        $device->waitForPropertyValue(
                                objects          => $disk_ref,
                                property         => 'health_details',
                                timeout          => '5 M',
                                value_expression => qr/removed/ );
    } 
    else
    {
        $device->waitForPropertyValue(
                                objects          => $disk_ref,
                                property         => 'state',
                                timeout          => '2 M',
                                value_expression => qr/Removed|Empty/ );
    }
}

sub platform_shim_disk_set_eol
{
	my ($disk) = @_;
	
    $disk->setProperty(eol_state => 1);
    my $eolstate = $disk->getProperty("eol_state");
    if (!$eolstate) {
        my $disk_id = $disk->getProperty("id");
        Automatos::Exception::Base->throw("Set EOL disk: $disk_id does not have EOL set");
    }
}

sub platform_shim_disk_get_eol_state
{
	my ($disk) = @_;
	
    return $disk->getProperty("eol_state");
}

sub platform_shim_disk_clear_eol_state
{
	my ($disk) = @_;

    $disk->clearEolState();
}

sub platform_shim_disk_clear_eol
{
	my ($device, $disk, $disk_ref) = @_;

    # Clear the eol bit
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    platform_shim_disk_clear_eol_state($disk);
	$device->waitForPropertyValue(objects => $disk_ref,
                                  property => 'eol_state',
                                  value => 0);
    my $eolstate = platform_shim_disk_get_eol_state($disk);
    if ($eolstate) {
        Automatos::Exception::Base->throw("disk: $disk_id still marked EOL");
    }
    platform_shim_disk_wait_for_disk_ready($device, $disk_ref);
}


sub platform_shim_disk_get_copy_checkpoint
{
   my $self = shift;

    my $vd = $self->getProperty('vd_fbe_id');
    
    if (!defined($vd)) {
       # Copy finished or did not start yet. Just return the end marker.
       my $disk_id = platform_shim_disk_get_property($self, 'id');
       print("platform_shim_disk_get_copy_checkpoint " .
             "VD for $disk_id is not defined yet, return 0xFFFFFFFFFFFFFFFF for checkpoint.\n");
       return "0xFFFFFFFFFFFFFFFF";
    }
    my %params = (object_fbe_id => $vd);

    my ($retHash) = $self->dispatch("raidGroupShowRebuild", %params);
    my $checkpoint = $retHash->{parsed}{$vd}{rebuild_checkpoint};

    return $checkpoint;
}

=for nd

Method: sendDebugControl
Second function for remove / insert unrecognized drives (Issue #504489)

Input:
$spObject, 
$driveObject, 
$action - 1 = ON, 0 = OFF

Returns:
*None*

Exceptions:
Automatos::Exception::Base

Debugging Flags:
*None*

=cut

sub platform_shim_disk_send_debug_control {
	my ($sp, $disk, $action ) = @_;
	
	my $disk_id = platform_shim_disk_get_property($disk, 'id');
	
	my $sp_name = platform_shim_sp_get_short_name($sp);
	
	if ( defined $action and $action == 1 ) {
		$action = 'CRU_ON';
	} elsif  ( defined $action and $action == 0) {
		$action = 'CRU_OFF';
	} elsif ( !defined $action ) {
		Automatos::Exception::Base->throw('You should define ACTION boolean value: 1 = ON, 0 = OFF');
	}

	my @cmd = 	(
					'fbe_api_object_interface_cli.exe',
					'-k', '-' . $sp_name,
					'-esp', '-drivemgmt', 'sendDebugControl', $disk_id, $action
				);

	my %return_hash = $sp->getHostObject()->run( command => \@cmd );
	my $line = $return_hash{stdout};
	
	return;
}

sub platform_shim_disk_user_copy {
   my ($disk) = @_;  
   $disk->copy();
}

sub platform_shim_disk_user_copy_to {
   my ($source, $destination) = @_;   
   $source->copy(destination => $destination);
}

sub platform_shim_disk_get_last_system_drive_slot
{
    my ($disk) = @_;
    my $upperVault = 3;
    # Only Rockies and later is supported
    #if ($disk->isRelease((release => 31, qualifier => '<'))) {
    #    $upperVault = 4;
    #}
    return $upperVault;
}


# Get current system disks. The number of system disk may be less than 4 due to drive removal/fault ...
#
# @parameter: 
#    $1: test config
# @return:
#    List of current system disks
# 
# @version:
#    2014-08-13. Created. Jamin Kang
sub platform_shim_disk_get_current_system_disks
{
    my $tc = shift;
    my $device = $tc->{device};    
    my @system_drives = ();
    my %params = ( vault => 1,
                   buses => ['0_0'] );
    @system_drives = platform_shim_device_get_disk($device, \%params);
    return @system_drives;
}

sub platform_shim_disk_set_performance_statistics{
	my ($disk, $hash_ref) =  @_;
	my %data_hash = %$hash_ref;

	$disk->setPerformanceStatistics(%data_hash);
}

sub platform_shim_disk_start_background_zero{
	my ($disk) =  shift @_;
	$disk->startBackgroundZero();
}


sub platform_shim_disk_get_pvd_metadata_info{
    my ($disk) = @_;

    my $device = $disk->getDevice(); 
    my %params = (object => $disk);
    my ($ret_hash) = $device->dispatch('diskShowProvisionDriveInfo', %params);

    my $object_id = $params{object}->getProperty('fbe_id');
    my %pvd_metadata_info;
    $pvd_metadata_info{paged_metadata_start_lba} = $ret_hash->{parsed}->{$object_id}->{paged_metadata_lba};
    $pvd_metadata_info{paged_metadata_size} = $ret_hash->{parsed}->{$object_id}->{paged_metadata_capacity};

    return \%pvd_metadata_info;
}

sub platform_shim_disk_get_pvd_default_offset
{
    my ($disk) = @_;

    my $device = $disk->getDevice();
    my %params = (object => $disk);
    my $object_id = platform_shim_disk_get_pvd_object_id($disk);
    my ($ret_hash) = $device->dispatch('diskShowProvisionDriveInfo', %params);

    return $ret_hash->{parsed}->{$object_id}->{default_offset};
}

sub platform_shim_disk_get_pvd_object_id
{
    my ($disk) = @_;

    return platform_shim_disk_get_property($disk, "fbe_id");
}

sub platform_shim_disk_clear_error_handle_health_info
{
    my ($self, $sp) = @_;
    my $cmd = 'diskClearErrorHandleHealthInfo';
    my %param = (
        'object'    => $self,
    );
    $sp->dispatch($cmd, %param);
}


sub platform_shim_disk_clear_verify_report($$)
{
    my ($self, $sp) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my $object_id = platform_shim_disk_get_pvd_object_id($self);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-pvd', 'clearVerifyReport',
        $object_id
    );
    platform_shim_sp_execute_command($sp, \@cmd);
}


sub platform_shim_disk_set_sniff_checkpoint($$$)
{
    my ($self, $sp, $chkpt) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my $object_id = platform_shim_disk_get_pvd_object_id($self);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-pvd', 'setVerifyCheckpoint',
        $object_id, $chkpt
    );
    platform_shim_sp_execute_command($sp, \@cmd);
}


sub platform_shim_disk_get_sniff_report($$)
{
    my ($self, $sp) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my $object_id = platform_shim_disk_get_pvd_object_id($self);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-pvd', 'getverifyreport', $object_id
    );

    my %record_hash;
    platform_shim_sp_execute_command($sp, \@cmd, \%record_hash);
    return %record_hash;
}


sub platform_shim_disk_set_drive_fault
{
    my ($disk, $sp) = @_;
    
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my %hash;

    # Set the DRIVE_FAULT attribute on the PDO for the specified SP
    # FBE_CLI>pdo_srvc {-bes <b_e_s> | -object_id <id>} -drive_fault <set>          : Sets drive_fault flag for this PDO
    my $cmd = "pdo_srvc -bes $disk_id -drive_fault set";
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}

sub platform_shim_disk_clear_drive_fault
{
    my ($disk, $sp) = @_;
    my $cmd = 'diskClearDriveFault';
    my %param = (
        'object'    => $disk,
    );
    $sp->dispatch($cmd, %param);
}

#'zeroing'             => 'PVD_DZ',
#'sniffing'            => 'PVD_SNIFF',
#'disk_all_operations' => 'PVD_ALL',
sub platform_shim_disk_enable_background_operation
{
    my ($disk, $operation) = @_;
    my $device = $disk->getDevice();
    my %params = (object => $disk, 
                  background_operation_state => 1, 
                  background_operation_code => $operation);
    
    $device->dispatch('diskSetBackgroundOperation', %params);
}

sub platform_shim_disk_enable_zeroing
{
    my ($disk) = @_;
    platform_shim_disk_enable_background_operation($disk, 'zeroing');
}


sub platform_shim_disk_set_pvd_debug_tracing
{
    my ($disk, $sp) = @_;
    
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my %hash;

    my $cmd = "spddf -bes $disk_id -debug_flags 0xff";
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}

sub platform_shim_disk_clear_pvd_debug_tracing
{
    my ($disk, $sp) = @_;
    
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my %hash;

    my $cmd = "spddf -bes $disk_id -debug_flags 0x0";
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}

sub platform_shim_disk_clear_all_pvd_debug_tracing
{
    my ($sp) = @_;    
    my %hash;

    my $cmd = "spddf -object_id all -debug_flags 0x0";
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}


sub platform_shim_disk_power_savings_set_state_enable
{
    my ($sp, $disk) = @_;

    my %params = (object => $disk, 
                  power_saving_state => 1);

    $sp->dispatch('diskSetPowerSaving', %params);
}

sub platform_shim_disk_power_savings_set_state_disable
{
    my ($sp, $disk) = @_;

    my %params = (object => $disk, 
                  power_saving_state => 0);

    $sp->dispatch('diskSetPowerSaving', %params);
}

sub platform_shim_disk_power_savings_set_idle_time
{    
    my ($sp, $disk, $idle_time) = @_;
        
    my $time_sec = Automatos::Util::Units::getNumber(Automatos::Util::Units::convertTime($idle_time, 'S'));

    my $disk_id = platform_shim_disk_get_property($disk, "id");
    my ($bus, $encl, $slot) = split/_/, $disk_id;    
    
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -disk -b $bus -e $encl -s $slot -idle_time $time_sec", \%hash)};
}

# platform_shim_disk_power_savings_show()
# 
# Returns a hash of:
# {
#  'power_saving_capable' => 1,
#  'power_saving_enabled' => 0,
#  'power_saving_state' => 'Not  saving  power'
# }
#
sub platform_shim_disk_power_savings_show
{
    my ($sp, $disk) = @_;

    my $disk_id = platform_shim_disk_get_property($disk, "id");
    my %params = (object => $disk);

    my ($ret_hash) = $sp->dispatch('diskShowPowerSaving', %params);
    return $ret_hash->{parsed}->{$disk_id};
}

sub platform_shim_disk_power_savings_is_capable
{
    my ($sp, $disk) = @_;

    my ($ret_hash) = platform_shim_disk_power_savings_show($sp, $disk);
    return $ret_hash->{power_saving_capable};
}

sub platform_shim_disk_power_savings_is_enabled
{
    my ($sp, $disk) = @_;

    my ($ret_hash) = platform_shim_disk_power_savings_show($sp, $disk);
    return $ret_hash->{power_saving_enabled};
}

sub platform_shim_disk_power_savings_is_saving
{
    my ($sp, $disk) = @_;

    my ($ret_hash) = platform_shim_disk_power_savings_show($sp, $disk);
    if ($ret_hash->{power_saving_state} =~ /Not/)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}



1;
