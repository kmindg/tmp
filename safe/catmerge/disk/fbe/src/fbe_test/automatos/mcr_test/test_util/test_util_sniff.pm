package mcr_test::test_util::test_util_sniff;

use strict;
use Math::BigInt;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_lun;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_test_case;
use mcr_test::test_util::test_util_general;
use mcr_test::test_util::test_util_dest;
use mcr_test::test_util::test_util_configuration;
use Data::Dumper;

sub test_util_sniff_get_checkpoint
{
    my ($tc, $disk) = @_;

    my $device = $tc->{device};
    my $object_id = platform_shim_disk_get_pvd_object_id($disk);
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $disk}); 
    my $disk_id = platform_shim_disk_get_property($disk, 'id');

    $tc->platform_shim_test_case_log_info("Get SV chkpt for $disk_id from " .
                                          platform_shim_sp_get_short_name($active_sp));
    my $save_sp = platform_shim_device_get_target_sp($device);
    test_util_general_set_target_sp($tc, $active_sp);

    my $sv_checkpoint = platform_shim_disk_get_property($disk, 'verify_checkpoint');

    test_util_general_set_target_sp($tc, $save_sp);

    $tc->platform_shim_test_case_log_info("Sniff checkpoint for disk $disk_id: $sv_checkpoint");

    return $sv_checkpoint;
}


sub test_util_sniff_set_checkpoint
{
    my ($tc, $disk, $chkpt_str) = @_;

    my $device = $tc->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $disk});
    my $sp_name = platform_shim_sp_get_short_name($active_sp);
    my $disk_id = platform_shim_disk_get_property($disk, 'id');

    $tc->platform_shim_test_case_log_info("Set SV chkpt " . $chkpt_str . 
                                          " for $disk_id from sp$sp_name");

    platform_shim_disk_set_sniff_checkpoint($disk, $active_sp, $chkpt_str);
}


sub test_util_sniff_wait_for_checkpoint_update
{
    my ($tc, $disk, $prev_chkpt_str, $timeout) = @_;
    my $timeout_sec = time + $timeout;
    my $disk_id = platform_shim_disk_get_property($disk, 'id');

    $tc->platform_shim_test_case_log_info("Wait for SV checkpoint update for disk $disk_id");
    my $prev_chkpt = Math::BigInt->new($prev_chkpt_str);

    while (time < $timeout_sec) {
        my $cur_checkpoint_str = test_util_sniff_get_checkpoint($tc, $disk);
        my $cur_checkpoint = Math::BigInt->new($cur_checkpoint_str);

        if ($cur_checkpoint > $prev_chkpt) {
            $tc->platform_shim_test_case_log_info("SV on disk $disk_id update from $prev_chkpt to $cur_checkpoint");
            return;
        } elsif ($cur_checkpoint < $prev_chkpt) {
            # This may be cause by:
            # 1. Sp reboot and now we get checkpoint on the other SP
            # 2. We finish 1 pass verify
            $tc->platform_shim_test_case_log_info("SV on disk $disk_id reset checkpoint from $prev_chkpt to $cur_checkpoint");
            $prev_chkpt = $cur_checkpoint;
        } else {
            $tc->platform_shim_test_case_log_info("Disk $disk_id: prev: $prev_chkpt, cur: $cur_checkpoint");
        }
        sleep(2);
    }
    $tc->platform_shim_test_case_assert_msg(0, "Timeout waiting for SV checkpoint to update, disk: $disk_id");
}


sub test_util_sniff_wait_for_checkpoint($$$$)
{
    my ($tc, $disk, $chkpt, $timeout) = @_;
    my $timeout_sec = time + $timeout;
    my $disk_id = platform_shim_disk_get_property($disk, 'id');

    $tc->platform_shim_test_case_log_info("Wait for SV checkpoint to $chkpt for disk $disk_id");
    while (time < $timeout_sec) {
        my $cur_chkpt = test_util_sniff_get_checkpoint($tc, $disk);
        if (hex $cur_chkpt >= hex $chkpt) {
            return;
        }
        $tc->platform_shim_test_case_log_info("Disk $disk_id: curv: $cur_chkpt, wait: $chkpt");
        sleep(2);
    }
    $tc->platform_shim_test_case_throw_exception("Timeout waiting for SV checkpoint reach $chkpt, disk: $disk_id");
}


sub test_util_sniff_inject_media_error
{
    my $tc = shift;
    my $disk = shift;
    my $sp = shift;

    my %params = platform_shim_test_case_validate(
        @_,
        {
            'lba' => { type => SCALAR },
            'blocks' => { type => SCALAR, default => "1" },
            'opcode' => { type => SCALAR, default => 'verify' },
            'error_insertion_count' => { type => SCALAR, default => 1 },
        }
    );

    my $block_size = 0;
    my $block_size_str = platform_shim_disk_get_property($disk, 'block_size');

    if ($block_size_str =~ /^(\d+)/)
    {
        $block_size = int($1);

        if ($block_size != 520 && $block_size != 4160)
        {
            platform_shim_test_case_throw_exception("Invalid block size val: $block_size.  Only 520 and 4160 supported");
        }
    }
    else
    {
        platform_shim_test_case_throw_exception("Invalid block size str: $block_size_str");
    }

    my $lba = Math::BigInt->new($params{lba});
    my $blocks = Math::BigInt->new($params{blocks});
    if ($blocks <= 0) {
        platform_shim_test_case_throw_exception("Invalid blocks: " . $blocks);
    }

    if ($block_size == 4160)
    {
        $lba = $lba/8;
        $blocks = ($blocks+7)/8;    
    }
    my $lba_end = $lba + $blocks - 1;

    my $lba_desc = $lba->as_hex();
    my $blocks_desc = $blocks->as_hex();
    my $lba_end_desc = $lba_end->as_hex();


    my %inject_params = (
        lba_start    => $lba_desc,
        lba_end      => $lba_end_desc,
        'opcode'     => $params{opcode},
        'error_insertion_count' => $params{error_insertion_count}
    );
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    $tc->platform_shim_test_case_log_info(
        "Sniff: Inject error to disk $disk_id PBA [$lba_desc, $lba_end_desc], blocks:$blocks, block_size:$block_size");
    test_util_dest_inject_media_error($tc, $disk, $sp, %inject_params);
}


sub test_util_sniff_get_verify_error_count($$$)
{
    my ($tc, $disk, $sp) = @_;
    my %error_hash = platform_shim_disk_get_sniff_report($disk, $sp);
    my $soft_error = $error_hash{'Total Recoverable Read Errors'};
    my $hard_error = $error_hash{'Total UnRecoverable Read Errors'};
    my $total = $soft_error + $hard_error;
    $tc->platform_shim_test_case_log_info("Soft: $soft_error, Hard: $hard_error, Total: $total");

    return $total;
}

# please check available method in Automatos::Utils::Event
sub test_util_sniff_check_event_log($$$$)
{
    my ($tc, $sp, $bookmark, $count) = @_;
    my $timeout = 60 * 10 + time;
    my %params = (
        expression => '11c4004',   # event log ID for "Soft media error."
        bookmark   => $bookmark,
    );
    my $event_count;

    while (time < $timeout) {
        my @entries = platform_shim_sp_get_eventlog_entry($sp, %params);

        $event_count = scalar(@entries);
        $tc->platform_shim_test_case_log_info("Current event count: $event_count, expected: $count");
        if ($event_count >= $count ) {
            return;
        }
        sleep(2);
    }
    $tc->platform_shim_test_case_assert_msg(0, "Event count: $event_count, expected: $count");
}

sub test_util_sniff_verify_error_count($$$$$)
{
    my ($tc, $disk, $sp, $count, $use_lei) = @_;
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    $tc->platform_shim_test_case_log_info("Check error count for disk $disk_id");

    if ($use_lei) {
        my $pvd_object_id = platform_shim_disk_get_property($disk, 'fbe_id');
        my %lei_stats = platform_shim_device_get_logical_error_injection_statistics_for_fbe_object_id($tc->{device}, $sp, $pvd_object_id);

        $tc->platform_shim_test_case_log_info("LEI record: " . Dumper(\%lei_stats));
    }
    else {
        my %dest_hash = platform_shim_dest_get_records($sp);
        $tc->platform_shim_test_case_log_info("Dest record: " . Dumper(\%dest_hash));
    }

    my $total_verify_error = test_util_sniff_get_verify_error_count($tc, $disk, $sp);
    $tc->platform_shim_test_case_assert_msg(
        $total_verify_error >= $count,
        "Error count $total_verify_error is less than $count");
}


sub test_util_sniff_clear_verify_report {

    my ( $tc, $disk ) = @_;

    my $device = $tc->{device};
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $disk});
    my $sp_name = platform_shim_sp_get_short_name($active_sp);
    my $disk_id = platform_shim_disk_get_property($disk, 'id');

    $tc->platform_shim_test_case_log_info("Clear SV report for $disk_id from sp$sp_name");

    platform_shim_disk_clear_verify_report($disk, $active_sp);
}

sub test_util_sniff_enable
{
    my ($tc) = @_;
    
    my %sys_snv = platform_shim_device_get_sniff_verify($tc->{device});

    if (!$sys_snv{'state'}) {
        $tc->platform_shim_test_case_log_info('Enabling sniff verify on the array');
        my %sniff_state = ('state' => 1);
        platform_shim_device_set_sniff_verify($tc->{device}, \%sniff_state);
        %sys_snv = platform_shim_device_get_sniff_verify($tc->{device});
    }
    
    my $sys_snv_state = $sys_snv{'state'} ? 'ENABLED' : 'DISABLED';
    $tc->platform_shim_test_case_log_info('SniffVerify is ' . $sys_snv_state . ' on the array');
}

sub test_util_sniff_wait_for_sniff_percent_increase
{
    my ($tc, $disk, $initial_percent, $timeout) = @_;
    my $device = $tc->{device};
    my $wait_until = time() + $timeout;
    
    my $disk_id = platform_shim_disk_get_property($disk, 'id');
    my $active_sp = platform_shim_device_get_active_sp($device, {object => $disk});
    platform_shim_device_set_target_sp($device, $active_sp);
    
    #my $initial_percent = platform_shim_disk_get_property($disk_object, 'verify_percent');
    while (time() < $wait_until)
    {
        my $current_percent = platform_shim_disk_get_property($disk, 'verify_percent');
        if ($current_percent > $initial_percent) {
            $tc->platform_shim_test_case_log_info("Sniff Verify percent increase from $initial_percent to $current_percent for disk $disk_id");
            my $checkpoint = platform_shim_disk_get_property($disk, 'verify_checkpoint');
            $tc->platform_shim_test_case_log_info("Sniff Verify checkpoint is $checkpoint");
            return $current_percent;
        } 
        elsif ($initial_percent == 100 && $current_percent == 100)
        {
            $tc->platform_shim_test_case_log_warn("Sniff Verify percent before: $initial_percent after: $current_percent for disk $disk_id");
            return $current_percent;
        } 
        sleep(2);
    }    
    
    $tc->platform_shim_test_case_throw_exception("Timed out waiting for sniff to proceed for $disk_id");
    
}

sub test_util_sniff_select_disk_for_sniff_from_array($$)
{
    my ($tc, $disks_ref) = @_;
    my @test_disks;

    foreach my $disk (@{$disks_ref}) {
        push(@test_disks, $disk);
        my $disk_zero_pct = platform_shim_disk_get_property($disk, 'zero_percent');
        my $disk_id = platform_shim_disk_get_property($disk, 'id');
        if ($disk_zero_pct != 100) {
            $tc->platform_shim_test_case_log_info("disk $disk_id zero percent is $disk_zero_pct, find another disk for sniff");
        } else {
            $tc->platform_shim_test_case_log_info("disk $disk_id zero percent is $disk_zero_pct.  Use for sniff.");
            return $disk;
        }
    }
    if (@test_disks) {
        return $test_disks[0];
    }
    return undef;
}


# find the fastest non-zeroing drive, otherwise return fastest drive
sub test_util_sniff_select_disk_for_sniff {

    my ($tc) = shift @_;

    my @disks = @{test_util_configuration_get_fastest_disks($tc, {num_disks => 'all'})};
    my $chosen;

    $chosen = test_util_sniff_select_disk_for_sniff_from_array($tc, \@disks);
    if ($chosen)
    {
        return $chosen;
    }
    else
    {
        platform_shim_test_case_throw_exception("no suitable disks for sniff");
    }
}


sub test_util_sniff_wait_min_disks_zeroed($$$)
{
    my ($tc, $min_disks_zeroed, $timeout) = @_;
    my $wait_until = time() + $timeout;

    my @beds =();
    while (time() < $wait_until) {
        my %disk_info = %{test_util_disk_get_pvd_info_hash($tc)};
        @beds = grep {
            $disk_info{$_}{zero_percent} == 100 
          } keys %disk_info;

        if (scalar(@beds) >=  $min_disks_zeroed)
        {
            $tc->platform_shim_test_case_log_info(scalar(@beds)." disks finished zeroing.");
            return;
        }
        sleep(10); # sleep for a long time, zeroing takes awhile.
    }
    platform_shim_test_case_throw_exception("Not enough disks finished zeroing within $timeout seconds ".scalar(@beds)." < $min_disks_zeroed");
}


1;
