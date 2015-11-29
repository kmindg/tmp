package mcr_test::test_util::test_util_copy;

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

# Values for checking copy status
our $FBE_LBA_INVALID = '0xFFFFFFFFFFFFFFFF';
our $FBE_LBA_INVALID_LOWER = '0xffffffffffffffff';
our $copy_complete_checkpoint = Math::BigInt->from_hex($FBE_LBA_INVALID);

sub test_util_copy_get_percent_copied 
{
    my ($tc, $disk, $b_expect_zero) = @_;
    my $vd = $disk->getProperty('vd_fbe_id');
    
    if (!defined($vd)) {
       # Copy finished or did not start yet. Just return an invalid checkpoint.

       my $disk_id = platform_shim_disk_get_property($disk, 'id');
       print("platform_shim_disk_get_percent_copied " .
             "VD for $disk_id is not defined yet.");
       if ($b_expect_zero) {
          return 0;
       } else {
          return 100;
       }
    }
    my $device = $tc->{device};
    
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "rginfo -object_id $vd", \%hash)};
    my $string = $ret_hash{stdout};
    my @lines = split /\n/, $string;
    my $vd_checkpoint = $copy_complete_checkpoint;
    my $capacity;
    foreach my $line (@lines) {
       #print $line . "\n";
       if ($line =~ /Rebuild checkpoint\[\d+\].+(0x[0123456789abcdef]+)/){
          if ($1 ne $FBE_LBA_INVALID_LOWER) {
             $vd_checkpoint = hex($1);
             print "VD: $vd checkpoint $vd_checkpoint\n";
          }
       } elsif ($line =~ /user Capacity:.+(0x[0123456789abcdef]+)/){
          $capacity = hex($1);
          print "VD: $vd capacity $capacity\n";
       }
    }
    if (($vd_checkpoint == $copy_complete_checkpoint) ||
        ($vd_checkpoint == 0)) {
       if ($b_expect_zero) {
          return 0;
       } else {
          return 100;
       }
    } else{
       return int(($vd_checkpoint * 100) / $capacity);
    }
}

sub test_util_copy_get_copy_doesnt_start_checkpoint
{
	my ($tc, $disk, $disk_id, $percent_copied_ref) = @_;

    # @note This method returns a fbe_u64_t
    my $percent_copied = test_util_copy_get_percent_copied($tc, $disk, 1);
    my $copy_checkpoint = Math::BigInt->from_hex(platform_shim_disk_get_copy_checkpoint($disk));
    my $copy_checkpoint_hex = $copy_checkpoint->as_hex();

    # If the get copy checkpoint results in a undefined it means that
    # the copy was never started.
    if (!defined $copy_checkpoint) {
        # Copy never started over-ride percent copied and checkpoint
        $percent_copied = 0;
        $copy_checkpoint = 0;
        $copy_checkpoint_hex = $copy_checkpoint->as_hex();
         $tc->logInfo("disk: $disk_id copy checkpoint: " . $copy_checkpoint_hex .
                      " percent copied: $percent_copied copy never started");
    } elsif (($percent_copied == 100)                        ||
             ($copy_checkpoint == $copy_complete_checkpoint)    ) {
        # Check if either the copied percent is 100 or the checkpoint
        # is at the end marker.
        $tc->logInfo("disk: $disk_id copy checkpoint: " . $copy_checkpoint_hex .
                     " percent copied: $percent_copied copy complete");
        $percent_copied = 100;
        $copy_checkpoint = $copy_complete_checkpoint;
    }
    
    # Update te percent copied and return the checkpoint
    $$percent_copied_ref = $percent_copied;
    return $copy_checkpoint;
}

sub test_util_copy_get_copy_progress_checkpoint
{
	my ($tc, $disk, $disk_id, $percent_copied_ref) = @_;

    # @note This method returns a fbe_u64_t
    my $copy_checkpoint = Math::BigInt->from_hex($FBE_LBA_INVALID);
    my $percent_copied = test_util_copy_get_percent_copied($tc, $disk, 0);
    if ($percent_copied != 100) {
       $copy_checkpoint = Math::BigInt->from_hex(platform_shim_disk_get_copy_checkpoint($disk));       
    }

    # If the get copy checkpoint results in a undefined it means that
    # the copy was never started.
    if (!defined $copy_checkpoint) {
        # Copy never started over-ride percent copied and checkpoint
        $percent_copied = 0;
        $copy_checkpoint = 0;
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        $tc->logInfo("disk: $disk_id copy checkpoint: " . $copy_checkpoint_hex .
                      " percent copied: $percent_copied copy never started");
    } elsif ($percent_copied != 100) {
       if ($copy_checkpoint == $copy_complete_checkpoint) {
           # @todo There is an issue with RAID-10 where the checkpoint is always
           #       the end marker.  Simply use the disk capacity and the copy
           #       percent.
           if ($percent_copied == 0) {
               $percent_copied = 1;
           }
           # Capacity is in MB convert to blocks
           my $disk_capacity = platform_shim_disk_get_property($disk, 'capacity') * 2048;
           my $temp_copy_checkpoint = int(($disk_capacity * $percent_copied) / 100);
           my $copy_checkpoint_hex = sprintf "0x%lx", $temp_copy_checkpoint;
           $copy_checkpoint = Math::BigInt->from_hex($copy_checkpoint_hex);
           $tc->logInfo("disk: $disk_id copy checkpoint: " . $copy_checkpoint_hex .
                        " percent copied: $percent_copied RAID-10 use percent");
       }
    } elsif (($percent_copied == 100)                        ||
             ($copy_checkpoint == $copy_complete_checkpoint)    ) {
        # Check if either the copied percent is 100 or the checkpoint
        # is at the end marker.
        my $copy_checkpoint_hex = $copy_checkpoint->as_hex();
        $tc->logInfo("disk: $disk_id copy checkpoint: " . $copy_checkpoint_hex .
                     " percent copied: $percent_copied copy complete");
        $percent_copied = 100;
        $copy_checkpoint = $copy_complete_checkpoint;
    }
    
    # Update te percent copied and return the checkpoint
    $$percent_copied_ref = $percent_copied;
    return $copy_checkpoint;
}
sub test_util_copy_display_progress
{
   my $copy_percent;
   my ($tc, $dest_disk, $dest_id, $copy_percent_ref) = @_;
   my $copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($tc, $dest_disk, $dest_id, \$copy_percent);
   my $copy_checkpoint_hex = $copy_checkpoint->as_hex();

   $tc->platform_shim_test_case_log_step("disk: $dest_id copy checkpoint: " .
                                          $copy_checkpoint_hex . " percent copied: $copy_percent");
   $$copy_percent_ref = $copy_percent;
   return $copy_checkpoint;

}
sub test_util_copy_check_copy_progress
{
	my ($tc, $dest_disk, $dest_id, $percent_copied_ref, $b_wait_for_percent_increase, $timeout_sec) = @_;	
	
    my $start_copy_percent = $$percent_copied_ref;
    my $start_copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($tc, $dest_disk, $dest_id, \$start_copy_percent);
    my $start_copy_checkpoint_hex = $start_copy_checkpoint->as_hex();
    my $start_time = time;
    my $elapsed_secs = time - $start_time;
    my $timeout_sec = $start_time + $timeout_sec;

    # Check if the copy is already complete
    if (test_util_is_copy_complete($start_copy_checkpoint)) {
        $tc->platform_shim_test_case_log_info("Check copy progress for disk: $dest_id checkpoint: " .
                                              $start_copy_checkpoint_hex . " percent copied: $start_copy_percent copy complete");
        $$percent_copied_ref = $start_copy_percent;
        return $start_copy_checkpoint;
    }

    # @todo Currently we log this to debug
    $tc->platform_shim_test_case_log_info("Check copy progress for disk: $dest_id start checkpoint: " .
                                          $start_copy_checkpoint_hex . " percent copied: $start_copy_percent");
    my $current_copy_percent = $start_copy_percent;
    my $current_copy_checkpoint = $start_copy_checkpoint; 
    my $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex();
	while (time < $timeout_sec) {
        $elapsed_secs = time - $start_time;
        $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex();
        if (test_util_is_copy_complete($current_copy_checkpoint)) {
            $tc->platform_shim_test_case_log_info("Check copy progress - checkpoint: " . 
                                                  $current_copy_checkpoint_hex . " percent copied: $current_copy_percent copy complete");
            $$percent_copied_ref = $current_copy_percent;
            return $current_copy_checkpoint;
        }
        if ($b_wait_for_percent_increase == 1) {
            if (($current_copy_percent > $start_copy_percent) ||
                ($current_copy_percent == 100)                   ) {   
                $tc->platform_shim_test_case_log_info("Percent copied for $dest_id increased from: $start_copy_percent" .
                                                      " to $current_copy_percent in: $elapsed_secs seconds");
                $$percent_copied_ref = $current_copy_percent;
                return $current_copy_checkpoint;
            }
        } elsif ($current_copy_checkpoint > $start_copy_checkpoint) {
            $tc->platform_shim_test_case_log_info("Copy checkpoint for $dest_id increased from: " . 
                                                  $start_copy_checkpoint_hex . " to: " .
                                                  $current_copy_checkpoint_hex . " in $elapsed_secs seconds");
            $$percent_copied_ref = $current_copy_percent;
            return $current_copy_checkpoint;
        }
        $tc->platform_shim_test_case_log_info("Copy checkpoint for $dest_id  : " .
                                              $current_copy_checkpoint_hex . " start was: " . 
                                              $start_copy_checkpoint_hex . " in $elapsed_secs seconds wait for percent: $b_wait_for_percent_increase");
		sleep(1);
        $current_copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($tc, $dest_disk, $dest_id, \$current_copy_percent); 
	}
	
    # Don't fail the test if the raid group is large and we observe that
    # the copy made progress but never observed a percentage increase
    if (($b_wait_for_percent_increase == 1)                 &&
        ($current_copy_checkpoint > $start_copy_checkpoint)    ) {
        $tc->platform_shim_test_case_log_info("Copy checkpoint for $dest_id increased from: " . 
                                              $start_copy_checkpoint_hex . " to: " .
                                              $current_copy_checkpoint_hex . " in $elapsed_secs seconds");
        $$percent_copied_ref = $current_copy_percent;
        return $current_copy_checkpoint;
    } elsif ($b_wait_for_percent_increase == 0) {
        # @todo RAID-10 checkpoint is always end marker. Wait for percent increase
        $tc->platform_shim_test_case_log_info("Copy checkpoint for $dest_id wait 300 seconds for RAID-10");
        $timeout_sec = time + 300;
        while (time < $timeout_sec) {
            $elapsed_secs = time - $start_time;
            $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex();
            if (test_util_is_copy_complete($current_copy_checkpoint)) {
                $tc->platform_shim_test_case_log_info("Check copy progress - checkpoint: " . 
                                                  $current_copy_checkpoint_hex . " percent copied: $current_copy_percent copy complete");
                $$percent_copied_ref = $current_copy_percent;
                return $current_copy_checkpoint;
            }
            if (($current_copy_percent > $start_copy_percent) ||
                ($current_copy_percent == 100)                   ) {   
                $tc->platform_shim_test_case_log_info("Percent copied for $dest_id increased from: $start_copy_percent" .
                                                      " to $current_copy_percent in: $elapsed_secs seconds");
                $$percent_copied_ref = $current_copy_percent;
                return $current_copy_checkpoint;
            }
            $tc->platform_shim_test_case_log_info("Copy checkpoint for $dest_id  : " .
                                              $current_copy_checkpoint_hex . " start was: " . 
                                              $start_copy_checkpoint_hex . " in $elapsed_secs seconds wait for percent: 1");
            sleep(1);
            $current_copy_checkpoint = test_util_copy_get_copy_progress_checkpoint($tc, $dest_disk, $dest_id, \$current_copy_percent); 
        }
    }

	platform_shim_test_case_throw_exception("Timeout waiting for copy percent to increase");
    $$percent_copied_ref = $current_copy_percent;
    return $current_copy_checkpoint;
}

sub test_util_copy_validate_copy_doesnt_start
{
	my ($tc, $source_disk, $source_id, $percent_copied_ref, $expected_checkpoint, $timeout_sec) = @_;	
	
    my $start_copy_percent = $$percent_copied_ref;   
    my $start_copy_checkpoint = test_util_copy_get_copy_doesnt_start_checkpoint($tc, $source_disk, $source_id, \$start_copy_percent);
    my $start_copy_checkpoint_hex = $start_copy_checkpoint->as_hex();
    my $start_time = time;
    my $elapsed_secs = time - $start_time;
	$timeout_sec = $start_time + $timeout_sec;
	
    # If the copy was never started return now
	$tc->platform_shim_test_case_log_info("Verify copy doesn't start on disk: $source_id ");
	my $current_copy_percent = $start_copy_percent;
    my $current_copy_checkpoint = test_util_copy_get_copy_doesnt_start_checkpoint($tc, $source_disk, $source_id, \$current_copy_percent);
    my $expected_checkpoint_hex = $expected_checkpoint->as_hex();
    my $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex(); 
	while (time < $timeout_sec) {
        $elapsed_secs = time - $start_time;
        if ($current_copy_checkpoint != $expected_checkpoint &&
            $current_copy_checkpoint != $copy_complete_checkpoint) {
            platform_shim_test_case_throw_exception("Verify copy doesn't start checkpoint: " .
                                                    $current_copy_checkpoint_hex . " does not match expected: " .
                                                    $expected_checkpoint_hex . " after: $elapsed_secs seconds");
            $$percent_copied_ref = $current_copy_percent;
            return $current_copy_checkpoint;
        }

		sleep(1);
        $current_copy_checkpoint = test_util_copy_get_copy_doesnt_start_checkpoint($tc, $source_disk, $source_id, \$current_copy_percent); 
        $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex();
	}
	
    $elapsed_secs = time - $start_time;
    $tc->platform_shim_test_case_log_info("Verify copy does not start checkpoint: $current_copy_checkpoint_hex" .
                                          " has not changed in $elapsed_secs seconds");
    $$percent_copied_ref = $current_copy_percent;
    return $current_copy_checkpoint;
}

sub test_util_copy_get_copy_no_progress_checkpoint
{
	my ($tc, $disk, $disk_id, $percent_copied_ref) = @_;

    # @note This method returns a fbe_u64_t
    # It is assumed that copy checkpoint is defined but the
    # percent copied will not be defined.  Just use the passed
    # percent copied.
    my $percent_copied = $$percent_copied_ref;
    my $copy_checkpoint = Math::BigInt->from_hex(platform_shim_disk_get_copy_checkpoint($disk));
    my $copy_checkpoint_hex = $copy_checkpoint->as_hex();

    # If the get copy checkpoint results in a undefined it means that
    # the copy was never started.  This is unexpected
    if (!defined $copy_checkpoint) {
        # Unexpected
        platform_shim_test_case_throw_exception("Get copy no progress checkpoint undefined for disk: $disk_id");
    }
    
    # Don't update the percent copied and return the checkpoint
    return $copy_checkpoint;
}

sub test_util_copy_check_copy_no_progress
{
	my ($tc, $dest_disk, $dest_id, $percent_copied_ref, $timeout_sec) = @_;	
	
    my $start_copy_percent = $$percent_copied_ref;   
    my $start_copy_checkpoint = test_util_copy_get_copy_no_progress_checkpoint($tc, $dest_disk, $dest_id, \$start_copy_percent);
    my $start_copy_checkpoint_hex = $start_copy_checkpoint->as_hex();
    my $start_time = time;
    my $elapsed_secs = time - $start_time;
	$timeout_sec = $start_time + $timeout_sec;
	
    # If the copy was never started return now
	$tc->platform_shim_test_case_log_info("Verify no copy progress on disk: $dest_id ");
	my $current_copy_percent = $start_copy_percent;
    my $current_copy_checkpoint = test_util_copy_get_copy_no_progress_checkpoint($tc, $dest_disk, $dest_id, \$current_copy_percent);
    my $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex(); 
	while (time < $timeout_sec) {
        $elapsed_secs = time - $start_time;
        if ($current_copy_checkpoint > $start_copy_checkpoint) {
            platform_shim_test_case_throw_exception("Verify no copy progress checkpoint: " .
                                                    $current_copy_checkpoint_hex . " has increased from: " .
                                                    $start_copy_checkpoint . " in: $elapsed_secs seconds");
            $$percent_copied_ref = $current_copy_percent;
            return $current_copy_checkpoint;
        }
		sleep(1);
        $current_copy_checkpoint = test_util_copy_get_copy_no_progress_checkpoint($tc, $dest_disk, $dest_id, \$current_copy_percent); 
        $current_copy_checkpoint_hex = $current_copy_checkpoint->as_hex();
	}
	
    $tc->platform_shim_test_case_log_info("Verify no copy progress checkpoint: " .
                                          $current_copy_checkpoint_hex . " has not changed in $elapsed_secs seconds");
    $$percent_copied_ref = $current_copy_percent;
    return $current_copy_checkpoint;
}

sub test_util_is_copy_complete
{
	my ($copy_checkpoint) = @_;

    if ($copy_checkpoint == $copy_complete_checkpoint) {
        return 1;
    }

    return 0;
}

sub test_util_copy_get_vd_capacity
{
	my ($tc, $vd_fbe_id, $sp) = @_;
    	
    my ($info) = $sp->dispatch('raidGroupShow', object_fbe_id => $vd_fbe_id);
    my $capacity = $info->{parsed}{ $vd_fbe_id }{paged_metadata_start_lba};

    $tc->platform_shim_test_case_log_info("Get vd obj: $vd_fbe_id capacity: $capacity");

    return $capacity;
}

sub test_util_set_copy_operation_speed
{
    my ($tc, $sp, $copy_speed, $default_rg_reb_speed_ref) = @_;
    
    my %hash;
    # @todo hash sbgos -status and get the default RG_REB speed:
    # set_bg_op_speed -status
    # Default speed
    # PVD_DZ    - 120
    # PVD_SNIFF -  10
    # RG_REB    - 120
    # RG_VER    -  10
    $$default_rg_reb_speed_ref = 120;

    # @note A speed of 0 is not allowed
    if ($copy_speed == 0) {
        Automatos::Exception::Base->throw("set copy speed: $copy_speed not allowed");
    }
    # -speed 1 means we will delay every other I/O by 200ms
    # FBE_CLI>sbgos -operation RG_REB -speed 1
    my $cmd = "sbgos -operation RG_REB -speed " . $copy_speed;
    platform_shim_sp_execute_fbecli_command($sp, $cmd, \%hash);
    $tc->platform_shim_test_case_log_info("set copy speed: sent: $cmd");
}

1;
