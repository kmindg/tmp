package mcr_test::platform_shim::platform_shim_test_case;

use strict;
use warnings;

use Config; 
use base qw(Automatos::Test::Case);
use Automatos::ParamValidate qw(validate_pos validate :types validate_with :callbacks);

use Automatos::AutoLog;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_raid_group;

use Exporter 'import';
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

=for nd

Method: preTestCase
This is a Classwide version of preTestCase used for all the tests in the ParentFeature
Directory.
Performs all actions required prior to the test case being executed.
The test developer can list the actions that will happen in the pre test case.

Input:
*None*

Returns:
*None*

Exceptions:
*None*

Debugging Flags:
*None*

=cut

sub preTestCase
{
    my ($self) = @_;

    my $name = $self->getName();

    if ($Config{longsize} >= 8) {
        $self->logInfo("64-bit Perl. longsize:" . $Config{longsize});
    } else {
        $self->logInfo("32-bit Perl. longsize:" . $Config{longsize});
    }
    $self->logInfo("In the pre test case of $name");
    $self->platform_shim_test_case_initialize();
    
    $self->platform_shim_test_case_print_purpose();
    
    return;
}



=for nd

Method: createMetadata

This is a Class wide version of createMetadata used for all the tests in the Zeroing
Directory.
Creates the metadata for the test.
This method will add the following test parameters of the following types:
- drivePosition
- sleepInterval
- zeroingPercentFinal
- zeroingPercentReadyForAction
- and many more...

Input:
*None*

Returns:
*None*

Exceptions:
*None*

Debugging Flags:
*None*

=cut

sub createMetadata
{
    my ($self) = @_;
    $self->addParameter(
                        name             => 'test_level',
                        description      => 'Test level determining runtime, lun size, additional actions, etc. ',
                        default          => 0,
                        validation       => {
                                                min => 0,
                                                max => 2,
                                            },
                        type             => 'Number',
                        display_name     => 'Test Level',
                        optional         => 1,
                        );  
    $self->addParameter(
                        name             => 'clean_config',
                        description      => 'Determine whether or not to clean the config before testing ',
                        default          => 1,
                        type             => 'Boolean',
                        display_name     => 'Clean config prior to test',
                        optional         => 1,
                        ); 
    $self->addParameter(
                        name             => 'skip_reboot',
                        description      => 'If set, we will skip the reboot of the SPs during the test. ',
                        default          => 0,
                        type             => 'Boolean',
                        display_name     => 'Skip rebootting SPs.',
                        optional         => 1,
                        );                          
    $self->addParameter(
                        name             => 'clear_drive_errors',
                        description      => 'Determine whether or not to clear EOL and other drive errors ',
                        default          => 0,
                        type             => 'Boolean',
                        display_name     => 'Clean drive errors prior to test',
                        optional         => 1,
                        );
    $self->addParameter(
                        name             => 'set_clear_raid_debug_flags',
                        description      => 'If True set the raid_group_debug_flags and raid_library_debug_flags as requested ',
                        default          => 0,
                        type             => 'Boolean',
                        display_name     => 'Use raid groups and raid library debug flags',
                        optional         => 1,
                        );
    $self->addParameter(
                        name             => 'raid_group_debug_flags',
                        description      => 'Set raid group debug flags before starting io',
                        default          => 0,
                        type             => 'Number',
                        display_name     => 'Raid Group Debug Flags',
                        optional         => 1,
                        );
    $self->addParameter(
                        name             => 'raid_library_debug_flags',
                        description      => 'Set raid library debug flags before starting io',
                        default          => 0,
                        type             => 'Number',
                        display_name     => 'Raid Library Debug Flags',
                        optional         => 1,
                        );
    $self->addParameter(
                        name            => 'encryption_enabler',
                        description      => 'Path to encryption enabler',
                        default         => '',
                        type             => 'TEXT',
                        display_name     => 'Encryption Enabler',
                        optional         => 1,
                       );
    $self->addParameter(
                        name            => 'host_audit_log_path',
                        description      => 'Host Path to audit log',
                        default         => 'c:',
                        type             => 'TEXT',
                        display_name     => 'Host Audit Log Path',
                        optional         => 1,
                       );
                        
    # remove the below                    
    $self->addParameter(
                        name             => 'raid_type',
                        description      => 'This is a raid type',
                        default          => 'r5',
                        validation       => {
                                               valid_values => ['r0', 'r1', 'r3', 'r5', 'r6', 'r1_0'],
                                            },
                        type             => 'SINGLE_SELECT',
                        display_name     => 'Raid Type',
                        optional         => 1,
                       );
                       
    $self->addParameter(
                        name             => 'target_lun_zeroing_percent',
                        description      => 'Drive zeroing percent that must be reached '.
                                            'for test to pass; '.
                                            'reaching 100 percent can take several hours',
                        default          => 100,
                        validation       => {
                                                min => 0,
                                                max => 100,
                                            },
                        type             => 'Number',
                        display_name     => 'Lun zeroing percent final',
                        optional         => 1,
                        );      
                                                                  
}


=for nd

Method: printPurpose
Print the purpose of the test case

Input:
*None*

Returns:
*None*

Exceptions:
*None*

Debugging Flags:
*None*

=cut 
sub platform_shim_test_case_print_purpose {
    my $self = shift; 
    my $purpose = shift; 
    $self->logInfo($purpose);
}

=for nd

Method: initialize
Required actions prior to running a test case

Input:
*None*

Returns:
*None*

Exceptions:
*None*

Debugging Flags:
*None*

=cut 
sub platform_shim_test_case_initialize
{
	my $self = shift;
    
    # Get the array device object.
    my $device = $self->platform_shim_test_case_get_device();
    
    $device->clearSystemLog();
    $device->clearAllFbeDebugHooks();
    platform_shim_device_stop_all_rdgen_io($device);        

    if ($self->platform_shim_test_case_get_parameter(name => "clean_config"))
    {
        $self->logInfo("Removing existing configuration");
        $device->wipe();
        # explicity remove raid groups since the framework does not automatically
        # do so for vnxe
        if (platform_shim_device_is_unified($device))
        {
            my @rgs = platform_shim_device_get_raid_group($device);
            foreach my $rg (@rgs)
            {
                eval {
                    foreach my $lun (platform_shim_raid_group_get_lun($rg))
                    {
                        $lun->wipe();
                    }
                };

                $rg->wipe();
            }
        }
    }
    

    platform_shim_device_wait_for_ecom($device, 3600);
    
    $self->{device} = $device;
    if ($self->platform_shim_test_case_get_parameter(name => "clear_drive_errors"))
    {
        platform_shim_device_clear_eol_faults($device);
    }
    platform_shim_device_clear_error_trace_counters($device);
    my $seed = srand();
    $self->logInfo("Seed is $seed");
}

sub platform_shim_test_case_get_device
{
	my $self = shift;
	
	# Get the Resource Object.
    my $resource_object = $self->getResource();
	
	# Get the array device object.
    my ($device) = $resource_object->getDevice(type => 'block');
    
    if (!$device) 
    {
		($device) = $resource_object->getDevice(type => 'file');    
	}
	
	if (!$device) 
	{
		($device) = $resource_object->getDevice(type => 'unified'); 
	}
	print "device: " . $device . "\n";
	return $device;
}


sub platform_shim_test_case_assert_true
{
	my ($self, $assert) = @_;
	
	if (!$assert) 
	{
		$self->logError("Assert failed");	
	}
}

sub platform_shim_test_case_assert_true_msg
{
	my ($self, $assert, $msg) = @_;
	
	if (!$assert) 
	{
		$self->logError("Assert failed: ".$msg);	
	}
}

sub platform_shim_test_case_assert_false
{
	my ($self, $assert) = @_;
	
	if ($assert) 
	{
		$self->logError("Assert failed");	
	}
}

sub platform_shim_test_case_assert_false_msg
{
	my ($self, $assert, $msg) = @_;
	
	if ($assert) 
	{
		$self->logError("Assert failed: ".$msg);	
	}
}

sub platform_shim_test_case_assert_msg
{
    my ($self, $assert, $msg) = @_;
    if (!$assert) {
        platform_shim_test_case_throw_exception($msg);
    }
}


sub platform_shim_test_case_log_info
{
    my ($self, $msg) = @_;
    $self->logInfo($msg);
}
sub platform_shim_test_case_log_property_info
{
    my ($self, $output, $msg) = @_;
    $self->logPropertyInfo($output, $msg);
}

sub platform_shim_test_case_log_step
{
    my ($self, $msg) = @_;
    $self->logStep($msg);
}

sub platform_shim_test_case_log_error
{
    my ($self, $msg) = @_;
    $self->logError($msg);
}

sub platform_shim_test_case_log_warn
{
    my ($self, $msg) = @_;
    $self->logWarning($msg);
}

sub platform_shim_test_case_log_debug
{
    my ($self, $msg) = @_;
    $self->logDebug($msg);
}

sub platform_shim_test_case_get_name
{
    my ($self) = @_;
    return $self->getName();
}

sub platform_shim_test_case_get_parameter
{
    my $self = shift;
    return $self->getParameter(@_);
}

sub platform_shim_test_case_log_start
{
    my $self = shift;
    platform_shim_device_insert_log_message($self->{device}, 'BEGIN_' . $self->platform_shim_test_case_get_name());
}

sub platform_shim_test_case_is_promotion
{
    my $self = shift;
    my $test_level = $self->platform_shim_test_case_get_parameter(name => "test_level");
    return ($test_level == 0); 
}

sub platform_shim_test_case_is_functional
{
    my $self = shift;
    my $test_level = $self->platform_shim_test_case_get_parameter(name => "test_level");
    return ($test_level == 1); 
}

sub platform_shim_test_case_is_bughunt
{
    my $self = shift;
    my $test_level = $self->platform_shim_test_case_get_parameter(name => "test_level");
    return ($test_level == 2); 
}

sub platform_shim_test_case_add_to_cleanup_stack
{
    my $self = shift;
    my $hash_ref = shift;
    
    $self->addToCleanupStack($hash_ref);
}


#######################################
# Static functions
#######################################

sub platform_shim_test_case_throw_exception
{
    my ($msg) = @_;
    Automatos::Exception::Base->throw($msg);    
}

sub platform_shim_test_case_get_logger
{
    my $gLog = Automatos::AutoLog->getLogger(__PACKAGE__);
    return $gLog;
}

sub platform_shim_test_case_validate
{
    my $specs = pop(@_);
    return Automatos::ParamValidate::validate(@_, $specs);
}




1;
