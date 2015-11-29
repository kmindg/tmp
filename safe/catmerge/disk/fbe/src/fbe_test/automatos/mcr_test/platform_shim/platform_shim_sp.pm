package mcr_test::platform_shim::platform_shim_sp;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use strict;

use Automatos::Exception;

#    fbe_sp                       => {},
#    mode                         => {},
#    name                         => {},
#    state                        => {},
#    sp_state                     => {},
#    peer_alive                   => {},
#    power_saving                 => {getmethod => 'deviceShowPowerSaving',
#                                     setmethod => 'deviceSetPowerSaving'},
#    hibernation_wake_up_time     => {getmethod => 'deviceShowPowerSaving'},
sub platform_shim_sp_get_property
{
	my ($sp, $property) =  @_;

	return $sp->getProperty($property); 	
}

sub platform_shim_sp_get_short_name
{
	my ($sp) = @_;
	
	my $name = platform_shim_sp_get_property($sp, 'name');	
	$name =~ s/sp(\w)/$1/ig;
	
	return $name;
}

sub platform_shim_sp_reboot
{       
	my ($sp) = @_;

    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($sp->getDevice())) 
    {
        $sp->reboot(target_mode => 'normal');	
    }
    else
    {
        $sp->reboot();        
    }	
    $sp->getHostObject()->waitForShutdown();
}


sub platform_shim_sp_panic
{
	my ($sp) = @_;
	
	$sp->panic();
	$sp->getHostObject()->waitForShutdown();
}

sub platform_shim_sp_wait_for_agent
{
	my ($sp, $timeout_sec) = @_;
	
	if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($sp->getDevice())) 
	{
	   $sp->waitForSystemCompleteState(timeout => $timeout_sec." S");    
	}
	else 
	{
	   $sp->waitForNaviAgent(timeout => $timeout_sec." S");
	}
	
}

sub platform_shim_sp_enable_automatic_sparing
{
	my ($sp) = @_;

	my %params = (state => 1);
	$sp->dispatch('deviceAutomaticHotSpareSet', %params);
}

sub platform_shim_sp_disable_automatic_sparing
{
	my ($sp) = @_;

	my %params = (state => 0);
	$sp->dispatch('deviceAutomaticHotSpareSet', %params);
}

sub platform_shim_sp_start_rdgen
{
    my ($sp, $params_ref) = @_;
    $sp->startRdGen(%$params_ref);
}

sub platform_shim_sp_execute_command {

    my ($sp, $cmd, $hash_ref) = @_;
    my %res  = $sp->getHostObject()->run( command => $cmd, );
    my $line = $res{stdout};
    my @rtn  = split( "\n", $line );

    if (! defined $hash_ref) {
        foreach my $val (@rtn) {
            # Check if command failed to execute.
            if ( $val =~ /\<ERROR\!\>/i ) {
                Automatos::Exception::Base->throw(
                    "FAILURE in executeCmd function ...");
            }
            if ($val =~ /\<Data\>\s+\w+/) {
                $val =~ s/\<Data\>//g;
                return $val;
            }
        }
        return $res{stdout};
    }

    foreach my $val (@rtn) {

        # Check if command failed to execute.
        if ( $val =~ /\<ERROR\!\>/i ) {
            Automatos::Exception::Base->throw("FAILURE in executeCmd function ...");
        }

        $val =~ s/\<Key\>//g;
        $val =~ s/\<Data\>//g;
        if ( $val =~ /^\s*\<(.+)\>\s+(\w+)/ ) {
            my $key = $1;
            if ($key) { ${$hash_ref}{$key} = $2;}
        }       
    }
    return;
}

# exploratory
sub platform_shim_sp_execute_fbecli_command
{
    my ( $sp, $cmd, $hash_ref ) = @_;
    
    my $spHost = $sp->getHostObject();
        my @wrapperPairs = $spHost->getWrapper();
        my $fbecli;
        foreach my $pair (@wrapperPairs) {
            if ($pair->{wrapper}->isa('Automatos::Wrapper::Tool::FbeCli')) {
                $fbecli = $pair->{wrapper};
                last;
            }
        }
    my $setup = $fbecli->hostSetup();
    my %cmdInfo = $setup->(host_object => $spHost,
                           cmd_info => {cmdline => ['fbecli.exe'], input => "access -m 1\n$cmd\n"});

    my %res  = $sp->getHostObject()->run(command => $cmdInfo{cmdline});
    my $line = $res{stdout};
    my @rtn  = split( "\n", $line );
    
            
    foreach my $val (@rtn) {
        # can parse values into hash?
        # Check if command failed to execute.
        if ( $val =~ /\<ERROR\!\>/i ) {
            Automatos::Exception::Base->throw(
                "FAILURE in executeCmd function ...");
        }    
    }
    
    return \%res;
}


sub platform_shim_sp_set_primarysp
{
	my ($sp, $sp_object) =  @_;

	return $sp->setPrimarySp($sp_object); 	
}

sub platform_shim_sp_set_background_op_speed
{
    my ($sp, $background_op, $speed) = @_;
    
    my %hash;
    my $cmd = "sbgos -operation $background_op -speed $speed";
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
}


sub platform_shim_sp_check_for_event {
    my ($sp, %event_params) = @_;
    
    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($sp->getDevice()))
    {
        my $name = $sp->getProperty("name");
        my %hash = $sp->checkForSystemLogEvent(%event_params);
        return @{$hash{$name}};      
    }
   
    return $sp->checkForSystemLogEvent(%event_params);
}


=begin nd

Method: platform_shim_sp_is_sniff_verify_enabled
Function to check if sniff is enabled system wide

Input:
    Test config
    SP Object

Returns:
    *none*

Exceptions:
    thrown if system sniff verify is disabled

Debugging Flags:
*None*

=cut
sub platform_shim_sp_is_sniff_verify_enabled
{
    my ($sp) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-SYSBGSERVICE', 'isSniffVerifyEnabled'	
	);
    my $ret = platform_shim_sp_execute_command($sp, \@cmd);
    if ( $ret =~ /System sniff verify is enabled/) {
        return 1;
    }
    return 0;
}

sub platform_shim_sp_bookmark_system_log
{
    my ($sp) = @_;        

    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($sp->getDevice())) 
    {
        my $bookmark = $sp->bookmarkSystemLog();
        return $bookmark;
    }
    else
    {
        my %bookmark = $sp->bookmarkSystemLog();
        return $bookmark{'bookmark'};
    }
}

sub platform_shim_sp_get_eventlog_entry
{
    my ($sp) = shift;
    my @entry;

    if (mcr_test::platform_shim::platform_shim_device::platform_shim_device_is_unified($sp->getDevice())) 
    {
        my $spName = $sp->getProperty('name');
        my %hash = $sp->checkForSystemLogEvent(@_);
        @entry = @{$hash{$spName}};
    }
    else
    {
        # See Framework/Dev/lib/Automatos/Component/Sp/Emc/Vnx.pm for detail
        @entry = $sp->checkForSystemLogEvent(@_);
    }
    return @entry;
}

=begin nd

This functions is used when we can't get lun object.
If we have lun object, use `platform_shim_lun_map_lba'

=cut

sub platform_shim_sp_map_lun_lba_by_object_id($$$)
{
    my ($sp, $lun_object_id, $lba) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);

    my @cmd = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-lun', 'mapLba', $lun_object_id, $lba
    );
    my %hash;

    platform_shim_sp_execute_command($sp, \@cmd, \%hash);
    return $hash{'PBA'};
}

=begin nd

Get all sep object id
$1: SP
$2: type: RAID/LUN

=cut

sub platform_shim_sp_discover_sep_objects($$)
{
    my ($sp, $type) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);

    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sys', '-discovery', 'discoverObjs', 'sep'
    );
    my @objects;
    my %hash;
    platform_shim_sp_execute_command($sp, \@cmd, \%hash);
    foreach my $key (keys %hash) {
        if (($type =~ /RAID.*/) && ($key =~ /RG_(.+)/)) {
            push(@objects, $hash{$key});
        } elsif (($type =~ /LUN/) && ($key =~ /LUN_(.+)/)) {
            push(@objects, $hash{$key});
        }
    }
    return @objects;
}

=begin nd

Test if an object is system object
$1: SP
$2: fbe object id

=cut

sub platform_shim_sp_is_system_object($$)
{
    my ($sp, $object_id) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);

    my @cmd     = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-database', 'isSystemObject', $object_id
    );
    my %hash;
    platform_shim_sp_execute_command($sp, \@cmd, \%hash);
    if ($hash{'Is System Object'} =~ /^Yes$/i) {
        return 1;   # system object
    }
    return 0;
}

=begin nd

Test if an object is system object
WARNING: This api works on Rockies 33 only.
The purpose of this api is to avoid the slowness of 'platform_shim_sp_is_system_object',
which causes 3 seconds for each call.

$1: SP
$2: fbe object id

=cut
sub platform_shim_sp_is_system_object_fast($$)
{
    my ($sp, $object_id_str) = @_;
    my $object_id = hex($object_id_str);

    #TODO: Add version check here
    return hex($object_id) < 0x100;
}

=begin nd

Clear LUN verify report

=cut
sub platform_shim_sp_clear_lun_bv_report_by_object_id($$)
{
    my ($sp, $obj_id ) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);
    my @cmd     = (
        'fbe_api_object_interface_cli.exe', '-k', '-' . $sp_name,
        '-sep', '-lun', 'clearVerifyRep', $obj_id);
    platform_shim_sp_execute_command($sp, \@cmd);
}

sub platform_shim_sp_cache_get_counters
{
    my ($sp) = @_;
    
    my ($return) = $sp->dispatch('deviceShowSpCounters');
    return %{$return->{parsed}};
}

sub platform_shim_sp_cache_clear_clean_pages
{
    my ($sp) = @_;
    platform_shim_sp_execute_command($sp,  ['mcccli.exe', '-x', '3']);
    
}


1;
