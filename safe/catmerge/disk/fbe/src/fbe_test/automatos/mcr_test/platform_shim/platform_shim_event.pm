package mcr_test::platform_shim::platform_shim_event;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_disk;
use Automatos::Util::Event;
use strict;

our %g_event_hash =
(
    'SPARE_SWAP_IN' => '1680111',
    'PROACTIVE_SPARE_SWAP_IN' => '1680113',
);


sub platform_shim_event_verify_lun_zeroing_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
                             
    Automatos::Util::Event::verifyLunZeroingStarted(%params); 
}

sub platform_shim_event_verify_lun_zeroing_completed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyLunZeroingCompleted(%params);
                             
}

sub platform_shim_event_verify_disk_zeroing_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
                             
    Automatos::Util::Event::verifyDiskZeroingStarted(%params); 
}

sub platform_shim_event_verify_disk_zeroing_completed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyDiskZeroingCompleted(%params);
                             
}

sub platform_shim_event_verify_system_verify_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyLunAutomaticVerifyStarted(%params);
}

sub platform_shim_event_verify_system_verify_completed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyLunAutomaticVerifyCompleted(%params);
}

sub platform_shim_event_verify_rw_verify_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyUserRWBackgroundVerifyStarted(%params);
}

sub platform_shim_event_verify_rw_verify_completed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyUserRWBackgroundVerifyCompleted(%params);
}


sub platform_shim_event_verify_media_error
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifySepErrorCodeRaidHostCoherencyError(%params);
}

# TODO: this isn't working properly
sub platform_shim_event_get_error_code
{
    my ($error_key) = @_;

    return $Automatos::Util::Event::EVENT_HASH{$error_key};
}

sub platform_shim_event_get_events
{
    my ($params_ref) = @_;
    Automatos::Util::Event::getEvent(%$params_ref);
}

sub platform_shim_event_get_log
{
    my ($device, $params_ref) = @_;
    $device->retrieveSystemLog(%$params_ref);
}

sub platform_shim_event_verify_rebuild_logging_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;                    
    Automatos::Util::Event::verifyRebuildLoggingStarted(%params); 
}

sub platform_shim_event_verify_rebuild_logging_stopped
{
    my ($params_ref) = @_;
    my %params = %$params_ref;   
    Automatos::Util::Event::verifyRebuildLoggingStopped(%params);
}

sub platform_shim_event_verify_rebuild_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyRebuildStarted(%params);
}

sub platform_shim_event_verify_rebuild_completed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    Automatos::Util::Event::verifyRebuildComplete(%params);
}

sub platform_shim_event_verify_spare_swapped_in
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    
    if ($params{disk})
    {
        $params{expression} = $g_event_hash{SPARE_SWAP_IN};
        $params{regex} = "is permanently replacing disk ".platform_shim_disk_get_property($params{disk}, 'id');

    }
    
    my @events = Automatos::Util::Event::getEvent(%params);
    return Automatos::Util::Event::getPermanentSpare(disk => $params{disk}, events => \@events);
    #return Automatos::Util::Event::verifySpareSwappedIn(%params);
    
}

sub platform_shim_event_proactive_spare_swapped_in
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    
    if ($params{disk})
    {
        $params{expression} = $g_event_hash{PROACTIVE_SPARE_SWAP_IN};
        $params{regex} = "is permanently replacing disk ".platform_shim_disk_get_property($params{disk}, 'id');

    }
    
    my @events = Automatos::Util::Event::getEvent(%params);
    return Automatos::Util::Event::getProactiveSpare(disk => $params{disk}, events => \@events);
    #return Automatos::Util::Event::verifyProactiveSpareSwappedIn(%params);
}

sub platform_shim_event_copy_started
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyCopyStarted(%params);
}

sub platform_shim_event_copy_complete
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyCopyComplete(%params);
}

sub platform_shim_event_user_copy_denied
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyCopyDenied(%params);
}

sub platform_shim_event_proactive_copy_denied
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyProactiveSpareDenied(%params);
}

sub platform_shim_event_copy_failed_due_to_source_removed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyCopyFailedBySourceRemoved(%params);
}

sub platform_shim_event_copy_failed_due_to_destination_removed
{
    my ($params_ref) = @_;
    my %params = %$params_ref;
    return Automatos::Util::Event::verifyCopyFailedByDestinationRemoved(%params);
}

1;
