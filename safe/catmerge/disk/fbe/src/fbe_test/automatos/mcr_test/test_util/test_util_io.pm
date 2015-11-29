package mcr_test::test_util::test_util_io;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_test_case;
use Params::Validate;
use strict;

our $FBE_OBJECT_ID_INVALID = 0xFFFFFFFF;
# @todo Find a way to get the class id if a lun.
our $FBE_CLASS_ID_LUN = 90;
our $DEFAULT_MAX_LBA = 0xF000;
our $TEST_ELEMENT_SIZE = 0x80;
our $FBE_RDGEN_PATTERN_LBA_PASS = 2; 


sub test_util_io_display_rdgen_stats
{
    my ($tc) = @_;   
    
    my $device = $tc->{device};
    my %active_stats = platform_shim_device_rdgen_stats($device, {});
    my @active_keys = keys %active_stats;
    my %historic_stats = %{$active_stats{historic_data}};;
    my @historic_keys = keys %historic_stats;

    $tc->platform_shim_test_case_log_info("rdgen stats: memory type:" . $active_stats{memory_type} . " size: " . $active_stats{memory_size} . "\n");
    $tc->platform_shim_test_case_log_info("Active Thread Statistics (Totals for threads currently running).");
    $tc->platform_shim_test_case_log_info("========================");
    foreach my $active_key (@active_keys) {
        if (($active_key ne 'historic_data') &&
            ($active_key ne 'memory_type')   &&
            ($active_key ne 'memory_size')      ) {
            $tc->platform_shim_test_case_log_info("    $active_key: $active_stats{$active_key}");
        }
    }
    $tc->platform_shim_test_case_log_info("Historical Statistics (Totals for threads that have completed) Reset with rdgen -reset_stats.");
    $tc->platform_shim_test_case_log_info("=====================");
    foreach my $historic_key (@historic_keys) {
        $tc->platform_shim_test_case_log_info("    $historic_key: $historic_stats{$historic_key}");
    }
}
sub test_util_io_write_background_pattern
{
    my $tc = shift;
    my $device = $tc->{device};
    my $io_size = $TEST_ELEMENT_SIZE * 4;
    my %params = platform_shim_test_case_validate(@_,
                             {luns => { type => ARRAYREF, optional => 1 },
                              b_validate_pattern => { type => SCALAR, default => 1},
                              block_count => => { type => SCALAR, default => $io_size},
                              start_lba => { type => SCALAR, default => 0},
                              minimum_lba => { type => SCALAR, default => 0},
                              maximum_lba => { type => SCALAR, default => $DEFAULT_MAX_LBA},
                             });

    # Monitor FBE_CLI>pdo -get_io_count 1_0_46 to confirm the number of threads
    # @note rdgen does not automatically truncate the I/O to guarantee that
    #       all blocks will be written.  Therefore round the max lba up to the
    #       next block count.
    my $maximum_lba = $params{maximum_lba};
    if ($params{maximum_lba} % $params{block_count}) {
        $maximum_lba += $params{block_count} - ($params{maximum_lba} % $params{block_count});
    }
    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        pass_count => 1, 
                        sequential_increasing_lba => 1,
                        use_block => 1,
                        block_count => $params{block_count},
                        start_lba => $params{start_lba},
                        minimum_lba =>$params{minimum_lba},
                        maximum_lba => $maximum_lba,
                        );
    my $expected_io_count = ($rdgen_params{maximum_lba} / $rdgen_params{block_count});
    platform_shim_device_rdgen_reset_stats($device);
    $tc->platform_shim_test_case_log_info("Writing background pattern - maximum_lba: $rdgen_params{maximum_lba}");
    if ($params{luns}) {
        foreach my $lun (@{$params{luns}}) {
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
        }
        foreach my $lun (@{$params{luns}}) {
            test_util_io_wait_rdgen_thread_complete($tc, 3600, $lun);
            test_util_io_verify_rdgen_io_complete($tc, $expected_io_count, $lun);
        }
    } else {  
        platform_shim_device_rdgen_start_io($device, \%rdgen_params);
        test_util_io_wait_rdgen_thread_complete($tc, 3600);
        test_util_io_verify_rdgen_io_complete($tc, $expected_io_count);
    }


    # Now (if requested) validate the background pattern.
    if ($params{b_validate_pattern}) {
        my %rdgen_params = (read_check   => 1, 
                            thread_count => 1, 
                            pass_count => 1, 
                            sequential_increasing_lba => 1,
                            use_block => 1,
                            block_count => $params{block_count},
                            start_lba => $params{start_lba},
                            minimum_lba =>$params{minimum_lba},
                            maximum_lba => $maximum_lba,
                            );
        $tc->platform_shim_test_case_log_info("Checking background pattern - maximum_lba: $rdgen_params{maximum_lba}");
        if ($params{luns}) {
            foreach my $lun (@{$params{luns}}) {
                platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
            }
            foreach my $lun (@{$params{luns}}) {
                platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
                test_util_io_wait_rdgen_thread_complete($tc, 3600, $lun);
            }
        } else {  
            platform_shim_device_rdgen_start_io($device, \%rdgen_params);
            test_util_io_wait_rdgen_thread_complete($tc, 3600);
        }
    }
}

sub test_util_io_wait_rdgen_complete
{
    my $tc = shift;
    my $device = $tc->{device};
    my $io_size = $TEST_ELEMENT_SIZE * 4;
    my %params = platform_shim_test_case_validate(@_,
                             {luns => { type => ARRAYREF, optional => 1 },
                              b_validate_pattern => { type => SCALAR, default => 1},
                              block_count => => { type => SCALAR, default => $io_size},
                              start_lba => { type => SCALAR, default => 0},
                              minimum_lba => { type => SCALAR, default => 0},
                              maximum_lba => { type => SCALAR, default => $DEFAULT_MAX_LBA},
                             });

    # Monitor FBE_CLI>pdo -get_io_count 1_0_46 to confirm the number of threads
    # @note rdgen does not automatically truncate the I/O to guarantee that
    #       all blocks will be written.  Therefore round the max lba up to the
    #       next block count.
    my $maximum_lba = $params{maximum_lba};
    if ($params{maximum_lba} % $params{block_count}) {
        $maximum_lba += $params{block_count} - ($params{maximum_lba} % $params{block_count});
    }
    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        pass_count => 1, 
                        sequential_increasing_lba => 1,
                        use_block => 1,
                        block_count => $params{block_count},
                        start_lba => $params{start_lba},
                        minimum_lba =>$params{minimum_lba},
                        maximum_lba => $maximum_lba,
                        );
    my $expected_io_count = ($rdgen_params{maximum_lba} / $rdgen_params{block_count});
    test_util_io_wait_rdgen_thread_complete($tc, 3600);
    test_util_io_verify_rdgen_io_complete($tc, $expected_io_count);
    test_util_io_display_rdgen_stats($tc);
}

sub test_util_io_start_check_pattern
{
    my $tc = shift;
    my $device = $tc->{device};
    my $io_size = $TEST_ELEMENT_SIZE * 4;
    my %params = platform_shim_test_case_validate(@_,
                             {luns => { type => ARRAYREF, optional => 1 },
                              block_count => => { type => SCALAR, default => $io_size},
                              start_lba => { type => SCALAR, default => 0},
                              minimum_lba => { type => SCALAR, default => 0},
                              maximum_lba => { type => SCALAR, default => $DEFAULT_MAX_LBA},
                             });

    # Monitor FBE_CLI>pdo -get_io_count 1_0_46 to confirm the number of threads
    # @note rdgen does not automatically truncate the I/O to guarantee that
    #       all blocks will be written.  Therefore round the max lba up to the
    #       next block count.
    my $maximum_lba = $params{maximum_lba};
    if ($params{maximum_lba} % $params{block_count}) {
        $maximum_lba += $params{block_count} - ($params{maximum_lba} % $params{block_count});
    }

    # Now (if requested) validate the background pattern.
    my %rdgen_params = (read_check   => 1, 
                        thread_count => 1, 
                        pass_count => 1, 
                        sequential_increasing_lba => 1,
                        use_block => 1,
                        block_count => $params{block_count},
                        start_lba => $params{start_lba},
                        minimum_lba =>$params{minimum_lba},
                        maximum_lba => $maximum_lba,
                        );
    $tc->platform_shim_test_case_log_info("Checking background pattern - maximum_lba: $rdgen_params{maximum_lba}");
    if ($params{luns}) {
        foreach my $lun (@{$params{luns}}) {
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
        }
    } else {  
        platform_shim_device_rdgen_start_io($device, \%rdgen_params);
    }
}
sub test_util_io_start_write_background_pattern
{
    my $tc = shift;
    my $device = $tc->{device};
    my $io_size = $TEST_ELEMENT_SIZE * 4;
    my %params = platform_shim_test_case_validate(@_,
                             {luns => { type => ARRAYREF, optional => 1 },
                              b_validate_pattern => { type => SCALAR, default => 1},
                              block_count => => { type => SCALAR, default => $io_size},
                              start_lba => { type => SCALAR, default => 0},
                              minimum_lba => { type => SCALAR, default => 0},
                              maximum_lba => { type => SCALAR, default => $DEFAULT_MAX_LBA},
                             });

    # Monitor FBE_CLI>pdo -get_io_count 1_0_46 to confirm the number of threads
    # @note rdgen does not automatically truncate the I/O to guarantee that
    #       all blocks will be written.  Therefore round the max lba up to the
    #       next block count.
    my $maximum_lba = $params{maximum_lba};
    if ($params{maximum_lba} % $params{block_count}) {
        $maximum_lba += $params{block_count} - ($params{maximum_lba} % $params{block_count});
    }
    my %rdgen_params = (write_only   => 1, 
                        thread_count => 1, 
                        pass_count => 1, 
                        sequential_increasing_lba => 1,
                        use_block => 1,
                        block_count => $params{block_count},
                        start_lba => $params{start_lba},
                        minimum_lba =>$params{minimum_lba},
                        maximum_lba => $maximum_lba,
                        );
    my $expected_io_count = ($rdgen_params{maximum_lba} / $rdgen_params{block_count});
    $tc->platform_shim_test_case_log_info("Writing background pattern - maximum_lba: $rdgen_params{maximum_lba}");
    if ($params{luns}) {
       $expected_io_count *= scalar(@{$params{luns}});
        foreach my $lun (@{$params{luns}})
        {
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
        }
    }    
    else
    {  
        platform_shim_device_rdgen_start_io($device, \%rdgen_params);
    }
}

sub test_util_io_check_background_pattern
{
    my $tc = shift;
    my %params = platform_shim_test_case_validate(@_,
                             {luns => { type => ARRAYREF, optional => 1 },
                              block_count => => { type => SCALAR, default => 0x80 },
                              start_lba => { type => SCALAR, default => 0},
                              minimum_lba => { type => SCALAR, default => 0},
                              maximum_lba => { type => SCALAR, default => $DEFAULT_MAX_LBA},
                              minimum_blocks => { type => SCALAR, default => $TEST_ELEMENT_SIZE},   
                              # pattern defaults to LBA_PASS. To change, select only one of the following patterns.
                              pattern_zeros =>  { type => BOOLEAN, optional => 1},   
                              pattern_clear =>  { type => BOOLEAN, optional => 1},
                             });
                                                          
    my $device = $tc->{device};
    # @note rdgen does not automatically truncate the I/O to guarantee that
    #       all blocks will be written.  Therefore round the max lba up to the
    #       next block count.
    my $maximum_lba = $params{maximum_lba};
    if ($params{maximum_lba} % $params{block_count}) {
        $maximum_lba += $params{block_count} - ($params{maximum_lba} % $params{block_count});
    }
    my %rdgen_params = (thread_count => 1,
                        read_check   => 1, 
                        #object_id    => $FBE_OBJECT_ID_INVALID,
                        #class_id     => $FBE_CLASS_ID_LUN,
                        pass_count => 1, 
                        sequential_increasing_lba => 1,
                        block_count => $params{block_count},
                        start_lba => $params{start_lba},
                        minimum_lba =>$params{minimum_lba},
                        maximum_lba => $maximum_lba,
                        minimum_blocks => $params{minimum_blocks},
                        available_for_writes => 1,
                        );

    $rdgen_params{pattern_zeros} = $params{pattern_zeros} if (exists $params{pattern_zeros});
    $rdgen_params{pattern_clear} = $params{pattern_clear} if (exists $params{pattern_clear});    
    $tc->platform_shim_test_case_log_info("Check background pattern - maximum_lba: $rdgen_params{maximum_lba}");
    if ($params{luns}) {
        foreach my $lun (@{$params{luns}})
        {
            platform_shim_lun_rdgen_start_io($lun, \%rdgen_params);
            test_util_io_wait_rdgen_thread_complete($tc, 3600, $lun);
        }
    }    
    else
    {                    
        platform_shim_device_rdgen_start_io($device, \%rdgen_params);
        test_util_io_wait_rdgen_thread_complete($tc, 3600);
    }
}

sub test_util_io_wait_rdgen_thread_complete
{
    my ($tc, $timeout, $lun) = @_;   
    
    my $device = $tc->{device};
    my %rdgen_params = ();
    if (defined $lun) {
        $rdgen_params{object} = $lun;
    }
    my %stats = platform_shim_device_rdgen_stats($device, \%rdgen_params);
    my $wait_until = time() + $timeout;
    while (time() < $wait_until)
    {
        my %stats = platform_shim_device_rdgen_stats($device, \%rdgen_params);
        if ($stats{thread_count} == 0)
        {
            return;
        }
        sleep(1);
    } 
    test_util_io_display_rdgen_stats($tc);
    platform_shim_test_case_throw_exception("Timed out waiting for rdgen threads to complete");
    
}

sub test_util_io_wait_rdgen_lun_io_complete
{
    my ($tc, $timeout, $expected_io_count, $lun) = @_;   
    
    my $device = $tc->{device};
    my %rdgen_params = ();
    if (defined $lun) {
        $rdgen_params{object} = $lun;
    }
    my $wait_until = time() + $timeout;
    while (time() < $wait_until)
    {
        my %stats = platform_shim_device_rdgen_stats($device, \%rdgen_params);
        my $io_count = $stats{historic_data}{io_count};
        if ($io_count >= $expected_io_count)
        {
            return;
        }
        sleep(1);
    } 
    test_util_io_display_rdgen_stats($tc);
    platform_shim_test_case_throw_exception("Timed out waiting for rdgen lun I/O to complete");
}

sub test_util_io_verify_rdgen_io_complete
{
    my ($tc, $minimum_io_count_expected, $lun) = @_;   
    
    my $device = $tc->{device};
    #$tc->platform_shim_test_case_log_info("verify rdgen io complete");
    my %rdgen_params = ();
    if (defined $lun) {
        $rdgen_params{object} = $lun;
    }
    my %stats = platform_shim_device_rdgen_stats($device, \%rdgen_params);
    my $io_count = $stats{historic_data}{io_count};
    if ($io_count < $minimum_io_count_expected) {
        test_util_io_display_rdgen_stats($tc);
        platform_shim_test_case_throw_exception("rdgen did not complete successfully");
    }
}

sub test_util_io_verify_rdgen_io_progress
{
    my ($tc, $io_count_to_wait_for, $timeout) = @_;   
    
    my $device = $tc->{device};
    my %stats = platform_shim_device_rdgen_stats($device, {});
    my $initial_io_count = $stats{io_count};
    my $start_time = time();
    my $elapsed_time = 0;
    while ($elapsed_time < $timeout) {
        $elapsed_time = time() - $start_time;
        my %stats = platform_shim_device_rdgen_stats($device, {});
        if ($stats{io_count} >= ($initial_io_count + $io_count_to_wait_for)) {
            $tc->platform_shim_test_case_log_info("rdgen io count: $stats{io_count} in: $elapsed_time seconds"); 
            return;
        }
        sleep(1);
    } 
    
    test_util_io_display_rdgen_stats($tc);
    platform_shim_test_case_throw_exception("rdgen did not find: $io_count_to_wait_for ios in: $elapsed_time seconds!");
    
}

sub test_util_io_set_raid_group_debug_flags
{
    my ($tc, $sp, $rgs_ref, $b_set) = @_;

    my @rgs = @{$rgs_ref};
    my $raid_group_debug_flags = $tc->platform_shim_test_case_get_parameter(name => "raid_group_debug_flags");
    my $raid_library_debug_flags = $tc->platform_shim_test_case_get_parameter(name => "raid_library_debug_flags");
    my $status = 0;
    
    # Since it takes some time to set/clear the flags only continue if enabled
    if (!$tc->platform_shim_test_case_get_parameter(name => "set_clear_raid_debug_flags")) {
        return $status;
    }

    if (!$b_set                          ||
        (!$raid_group_debug_flags   &&
         !$raid_library_debug_flags    )    ) {
        $tc->platform_shim_test_case_log_info("Clearing raid group and library debug flags.");
        $raid_group_debug_flags = 0;
        $raid_library_debug_flags = 0;
    } else {
        my $hex_group_flags = sprintf "0x%lx", $raid_group_debug_flags;
        my $hex_library_flags = sprintf "0x%lx", $raid_library_debug_flags;
        $tc->platform_shim_test_case_log_info("Setting raid group: $hex_group_flags and library: $hex_library_flags debug flags.");
    }
    foreach my $rg (@rgs) {
        # Simply set the raid group and raid library debug flags
        $status = platform_shim_raid_group_set_raid_group_debug_flags($sp, $rg, $raid_group_debug_flags, $raid_library_debug_flags);
    }
    $tc->platform_shim_test_case_log_info("Setting($b_set)/clearing raid group debug flags complete");

    return $status;
}

sub test_util_io_wait_for_dirty_pages_flushed
{
    my ($tc, $timeout) = @_;
    my @sps = platform_shim_device_get_sp($tc->{device});
    
    my $wait_until = time() + $timeout;
    foreach my $sp (@sps)
    {
        while (time() < $wait_until)
        {
            my %mcc_counters = platform_shim_sp_cache_get_counters($sp);
            my $sp_name = platform_shim_sp_get_property($sp, "name");
            $tc->platform_shim_test_case_log_info("$sp_name cache dirty pages: ". $mcc_counters{dirty_pages});
            if ($mcc_counters{dirty_pages} == 0) 
            {
                last;
            }
            sleep(2);
        }
    }
}

1;
