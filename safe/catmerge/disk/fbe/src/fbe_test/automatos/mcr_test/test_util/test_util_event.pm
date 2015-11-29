package mcr_test::test_util::test_util_event;

use strict;
use warnings;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_test_case;
use mcr_test::test_util::test_util_general;
use mcr_test::platform_shim::platform_shim_event;
use Automatos::ParamValidate qw(validate_pos validate :types validate_with :callbacks);

our $g_default_timeout = '300'; #5 min
our %g_event_codes = ('SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED'               => '1680002',
                    'SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED'        => '1680003',
                    'SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED'           => '1688001',
                    'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR'    => '1688002',
                    'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR'  => '1688003',
                    'SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED'           => '1688007',
                    'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR'         => '1688008',
                    'SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR'          => '168C001',
                    'SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR'        => '168C002',
                    'SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR'              => '168C003',
                    'SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR'              => '168C004',);


sub test_util_event_wait_for_checksum_error_events
{
    my $tc = shift;
    my %params = validate(@_, 
                            {raid_group_fbe_id => { type => SCALAR, optional => 1},
                             position   => { type => SCALAR, optional => 1},
                             lba        => { type => SCALAR, optional => 1},
                             blocks     => { type => SCALAR, optional => 1},
                             error_info => { type => SCALAR, optional => 1},
                             extra_info => { type => SCALAR, optional => 1},
                             timeout => { type => SCALAR, default => $g_default_timeout },
                            });
                            
    my $wait_until = time() + (delete $params{timeout});
    my @checksum_data_event_codes = ($g_event_codes{'SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR'},
                                     #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED'},
                                     #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR'}
                                     );

    my @checksum_parity_event_codes = ($g_event_codes{'SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR'},
                                       #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED'},      
                                       #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR'}
                                       );
    
    my $raid_regex = test_util_event_get_raid_regex($tc, \%params); 

        
                             
    while (time() < $wait_until)
    {   
        my %device_events = platform_shim_event_get_log($tc->{device});    
        my @matches = test_util_event_get_message_matches($tc, $raid_regex, \%device_events);
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@checksum_data_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all data checksum event codes for ".$params{raid_group_fbe_id});
            return;
        }
        
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@checksum_parity_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all parity checksum event codes for ".$params{raid_group_fbe_id});
            return;
        }
             
    }
    
    platform_shim_test_case_log_info($tc, "regex: $raid_regex");
    platform_shim_test_case_throw_exception("Timed out waiting for checksum events  for ".$params{raid_group_fbe_id});
                                
}

sub test_util_event_wait_for_coherency_error_events
{
    my $tc = shift;
    my %params = validate(@_, 
                            {raid_group_fbe_id => { type => SCALAR, optional => 1},
                             position   => { type => SCALAR, optional => 1},
                             lba        => { type => SCALAR, optional => 1},
                             blocks     => { type => SCALAR, optional => 1},
                             error_info => { type => SCALAR, optional => 1},
                             extra_info => { type => SCALAR, optional => 1},
                             timeout => { type => SCALAR, default => $g_default_timeout },
                            });
                            
    my $wait_until = time() + (delete $params{timeout});
    my @coh_data_event_codes = ($g_event_codes{'SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR'},
                                #$g_event_codes{'SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED'},
                                #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED'},
                                #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR'}
                                );

    my @coh_parity_event_codes = ($g_event_codes{'SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR'},
                                  #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED'},      
                                  #$g_event_codes{'SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED'},
                                  #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR'},
                                  );
    
    my $raid_regex = test_util_event_get_raid_regex($tc, \%params); 
                                    
    while (time() < $wait_until)
    {   
        my %device_events = platform_shim_event_get_log($tc->{device});    
        my @matches = test_util_event_get_message_matches($tc, $raid_regex, \%device_events);
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@coh_data_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all data coherency event codes for ".$params{raid_group_fbe_id});
            return;
        }
        
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@coh_parity_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all parity coherency event codes for ".$params{raid_group_fbe_id});
            return;
        }
             
    }
    
    $tc->platform_shim_test_case_log_info("regex: $raid_regex");
    platform_shim_test_case_throw_exception("Timed out waiting for coherency events  for ".$params{raid_group_fbe_id});
                                
}

sub test_util_event_wait_for_media_error_events
{
    my $tc = shift;
    my %params = validate(@_, 
                            {raid_group_fbe_id => { type => SCALAR, optional => 1},
                             position   => { type => SCALAR, optional => 1},
                             lba        => { type => SCALAR, optional => 1},
                             blocks     => { type => SCALAR, optional => 1},
                             error_info => { type => SCALAR, optional => 1},
                             extra_info => { type => SCALAR, optional => 1},
                             timeout => { type => SCALAR, default => $g_default_timeout },
                            });
                            
    my $wait_until = time() + (delete $params{timeout});
    my @media_data_event_codes = ($g_event_codes{'SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED'},
                                  #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR'},
                                  #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED'}
                                 );

    my @media_parity_event_codes = ($g_event_codes{'SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED'},
                                    #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR'},      
                                    #$g_event_codes{'SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED'}
                                   );
    
    my $raid_regex = test_util_event_get_raid_regex($tc, \%params);
                                        
    while (time() < $wait_until)
    {   
        my %device_events = platform_shim_event_get_log($tc->{device});    
        my @matches = test_util_event_get_message_matches($tc, $raid_regex, \%device_events);
        
        map {$tc->logInfo("matching regex $_\n")} @matches;
        
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@media_data_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all data media event codes for ".$params{raid_group_fbe_id}." $raid_regex");
            return;
        }
        
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@media_parity_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all parity media event codes for ".$params{raid_group_fbe_id});
            return;
        }
             
    }
    
    $tc->platform_shim_test_case_log_info("regex: $raid_regex");
    platform_shim_test_case_throw_exception("Timed out waiting for media events for ".$params{raid_group_fbe_id});
                                
}

sub test_util_event_wait_for_soft_media_error_events
{
    my $tc = shift;
    my %params = validate(@_, 
                            {raid_group_fbe_id => { type => SCALAR, optional => 1},
                             position   => { type => SCALAR, optional => 1},
                             lba        => { type => SCALAR, optional => 1},
                             blocks     => { type => SCALAR, optional => 1},
                             error_info => { type => SCALAR, optional => 1},
                             extra_info => { type => SCALAR, optional => 1},
                             timeout => { type => SCALAR, default => $g_default_timeout },
                            });
                            
    my $wait_until = time() + (delete $params{timeout});
    my @soft_media_data_event_codes = ($g_event_codes{'SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED'});

    my @soft_media_parity_event_codes = ($g_event_codes{'SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED'});
    
    my $raid_regex = test_util_event_get_raid_regex($tc, \%params); 
                                            
    while (time() < $wait_until)
    {   
        my %device_events = platform_shim_event_get_log($tc->{device});    
        my @matches = test_util_event_get_message_matches($tc, $raid_regex, \%device_events);
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@soft_media_data_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all data soft media event codes for ".$params{raid_group_fbe_id});
            return;
        }
        
        if (test_util_event_contains_all_event_codes($tc, {events => \@matches, event_codes =>\@soft_media_parity_event_codes}))
        {
            platform_shim_test_case_log_info($tc, "Found all parity soft media event codes for ".$params{raid_group_fbe_id});
            return;
        }
             
    }
    $tc->platform_shim_test_case_log_info("regex: $raid_regex");
    platform_shim_test_case_throw_exception("Timed out waiting for soft media events");
                                
}

sub test_util_event_contains_all_event_codes
{
    my $tc = shift;
    my %params = validate(@_, 
                            {events      => { type => ARRAYREF},
                             event_codes => { type => ARRAYREF},
                            });
    
    my %hash = ();
    foreach my $event_code (@{$params{event_codes}})
    {
        $tc->logInfo("see if there is a match for $event_code");
        my @matches = ();
        if (platform_shim_device_is_unified($tc->{device}))
        {
             @matches = grep {$_->{message_id} =~ m/$event_code/i} @{$params{events}};
        } 
        else
        {
            @matches = grep {m/$event_code/i} @{$params{events}};
        }
        if (!scalar(@matches))
        {
            $tc->logInfo("no matches");
            return 0;
        }
    }
    
    $tc->logInfo("match for all event codes");
    return 1;
}

sub test_util_event_get_raid_regex
{
    my $tc = shift;
    my %params = validate(@_, 
                            {raid_group_fbe_id => { type => SCALAR, optional => 1},
                             position   => { type => SCALAR, optional => 1},
                             lba        => { type => SCALAR, optional => 1},
                             blocks     => { type => SCALAR, optional => 1},
                             error_info => { type => SCALAR, optional => 1},
                             extra_info => { type => SCALAR, optional => 1}
                            });
                            
    my $regex = '(RAID Group|RG): [0-9a-f]+ (Position|Pos): [0-9a-f]+ LBA: [0-9a-f]+ Blocks: [0-9a-f]+ (Error|Err) info: [0-9a-f]+ Extra info: [0-9a-f]+';

    if ($params{raid_group_fbe_id}) {
        $regex =~ s/\(RAID Group\|RG\)\: \[0\-9a\-f\]\+/\(RAID Group\|RG\)\: $params{raid_group_fbe_id}/i;
    } 
    
    if ($params{position}) {
        $regex =~ s/\(Position\|Pos\)\: \[0\-9a\-f\]\+/\(Position\|Pos\)\: $params{position}/i;
    } 
    
    if ($params{lba}) {
        $regex =~ s/LBA\: \[0\-9a\-f\]\+/LBA\: $params{lba}/i;
    } 
    
    if ($params{blocks}) {
        $regex =~ s/Blocks\: \[0\-9a\-f\]\+/Blocks\: $params{blocks}/i;
    } 
    
    if ($params{error_info}) {
        $regex =~ s/\(Error\|Err\) info\: \[0\-9a\-f\]\+/\(Error\|Err\) info\: $params{error_info}/i;
    } 
    
    if ($params{extra_info}) {
        $regex =~ s/Extra info\: \[0\-9a\-f\]\+/Extra Info\: $params{extra_info}/i;
    } 
    
    return $regex;
}

sub test_util_event_get_message_matches
{
    my ($tc, $raid_regex, $device_events_ref) = @_;
    
    my %device_events = %$device_events_ref;
    my @events = ();
    my @matches = ();
    if (platform_shim_device_is_unified($tc->{device}))
    {
        map {push @events, @{$device_events{$_}}} keys %device_events;
        #foreach my $event (@events) {
        #    foreach my $key (%$event) {
        #        $tc->platform_shim_test_case_log_info("event: ".$event->{$key});
        #    }
        #}
        @matches = grep {$_->{message_text} =~ m/$raid_regex/i} @events;
    } 
    else
    {
        map {push @events, @{$device_events{$_}{entries}}} keys %device_events;
        @matches = grep {$_ =~ m/$raid_regex/i} @events;
    }
        
    return @matches;
}

sub test_util_event_check_for_unexpected_errors
{
   my $tc = shift;
   my $timeout = shift || 60;
   
   my $device = $tc->{device};
   my $errors = 0;
   $tc->platform_shim_test_case_log_step('Checking for unexpected errors');
   
   # Until rdgen is updated to remember ios that were written, we need to ignore
   # erorrs that were found with rebuild. It is possible that some full stripe write IO
   # may not have ended up in the write_log and is not yet flushed yet. Rebuild may have gotten
   # to the error region before flushes went out. 
   # Edit: We also need to ignore any media errors found during the test
   my $regex = '(Error|Err) info: (?:(?!40000\\d+).)* Extra info: (?!\\d+0e|e)';
   
   foreach my $sp ( platform_shim_device_get_sp($device) ) {

        if ( platform_shim_sp_check_for_event($sp, expression => '1680002', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Sector Reconstructed Raid Group Error');
            $errors++;
        } else {
           $tc->platform_shim_test_case_log_info("No sector reconstructed found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '1680003', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Parity Sector Reconstructed Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No parity sector reconstructed found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '1688007', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Parity Invalidated Raid Group');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No parity invalidated found");
        }
        
        if ( platform_shim_sp_check_for_event($sp,expression => '1688008', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Uncorrectable Sector Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No uncorrectable sector found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '1688001', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Data Sector Invalidated Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No sector invalidated found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '168C003', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Coherency Error Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No coherency found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '168C001', regex => $regex) ) {
            $tc->platform_shim_test_case_log_error('Checksum Error Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No Checksum found");
        }

        if ( platform_shim_sp_check_for_event($sp, expression => '168C002', regex => $regex ) ) {
            $tc->platform_shim_test_case_log_error('Parity Checksum Error Raid Group Error');
            $errors++;
        }else {
           $tc->platform_shim_test_case_log_info("No Parity Checksum found");
        }
    }
    if ($errors > 0) {
       platform_shim_test_case_throw_exception("Unexpected Errors found.");
    }
}
1;
