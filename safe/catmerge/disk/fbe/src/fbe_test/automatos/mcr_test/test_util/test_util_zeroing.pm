package mcr_test::test_util::test_util_zeroing;

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
use mcr_test::platform_shim::platform_shim_test_case;
use mcr_test::test_util::test_util_disk;

our $g_min_disks_zeroed = 20;

sub test_util_zeroing_wait_for_lun_zeroing_percent
{
	my ($tc, $lun, $target_percent, $timeout_sec) = @_;
	my $zero_percent = 0;
	$timeout_sec = time + $timeout_sec;
	
	$tc->platform_shim_test_case_log_info("Wait for Zeroing for lun ".
				 platform_shim_lun_get_property($lun, 'id')); 
	while (time < $timeout_sec) 
	{
		my $zeroing_state = platform_shim_lun_get_zeroing_state($lun);
		$zero_percent = $zeroing_state->{zero_percent};
		if ($zero_percent >= $target_percent) 
		{
			$tc->platform_shim_test_case_log_info("Zeroing percent on lun ".
						 platform_shim_lun_get_property($lun, 'id') .
						 " reached $zero_percent. Target $target_percent\n");
			return; 
		}
		elsif ($zero_percent % 10 == 0)
        {
            $tc->platform_shim_test_case_log_info("Zeroing percent on lun ".
                         platform_shim_lun_get_property($lun, 'id') .
                         " reached $zero_percent.\n");
        }
		sleep(2);
	}
	
	$tc->platform_shim_test_case_log_error("Timeout waiting for zeroing percent");

}

sub test_util_zeroing_wait_for_disk_zeroing_percent
{
	my ($tc, $disk, $target_percent, $timeout_sec) = @_;
	
	my $zero_percent = 0;
	$timeout_sec = time + $timeout_sec;
	
	$tc->platform_shim_test_case_log_info("Wait for Zeroing for disk ".
				 platform_shim_disk_get_property($disk, 'id')); 
	while (time < $timeout_sec) 
	{
		my $zero_percent = platform_shim_disk_get_property($disk, 'zero_percent');
		if ($zero_percent >= $target_percent) 
		{
			$tc->platform_shim_test_case_log_info("Zeroing percent on disk ".
						 platform_shim_disk_get_property($disk, 'id') .
						 " reached $zero_percent. Target $target_percent\n");
			return; 
		}
		elsif ($zero_percent % 10 == 0)
		{
		    $tc->platform_shim_test_case_log_info("Zeroing percent on disk ".
                         platform_shim_disk_get_property($disk, 'id') .
                         " reached $zero_percent.\n");
		}
		sleep(2);
	}
	
	$tc->platform_shim_test_case_log_error("Timeout waiting for zeroing percent");
	
}

sub test_util_zeroing_wait_for_disk_zeroing_percent_increase
{
	my ($tc, $disk, $zero_percent, $timeout_sec) = @_;	
	
	$timeout_sec = time + $timeout_sec;
	
	my $disk_id = platform_shim_disk_get_property($disk, 'id');
	$tc->platform_shim_test_case_log_info("Wait for Zeroing increase for disk $disk_id"); 
	while (time < $timeout_sec) 
	{
		my $current_zero_percent = platform_shim_disk_get_property($disk, 'zero_percent');
		if ($current_zero_percent > $zero_percent) 
		{
			$tc->platform_shim_test_case_log_info("Zeroing percent on disk ".$disk_id.
						 " increased to $current_zero_percent\n");
			return $current_zero_percent; 
		}
		elsif ($zero_percent == 100 && $current_zero_percent == 100)
        {
            $tc->platform_shim_test_case_log_warn("Zero percent before: $zero_percent after: $current_zero_percent for disk ".$disk_id);
            return $current_zero_percent;
        } 
		sleep(2);
	}
	
	$tc->platform_shim_test_case_log_error("Timeout waiting for zeroing percent to increase");
}

sub test_util_zeroing_wait_min_disks_zeroed
{
    my ($tc, $timeout) = @_;
    
    my $wait_until = time() + $timeout;

    my @beds =();
    while (time() < $wait_until)
    {
        my %disk_info = %{test_util_disk_get_pvd_info_hash($tc)};
        @beds = grep {$disk_info{$_}{zero_percent} == 100} keys %disk_info;
        if (scalar(@beds) >=  $g_min_disks_zeroed)
        {
            $tc->platform_shim_test_case_log_info(scalar(@beds)." disks finished zeroing.");
            return;
        }
        
        sleep(10); # sleep for a long time, zeroing takes awhile. 
    }
    
    platform_shim_test_case_throw_exception("Not enough disks finished zeroing within $timeout seconds ".scalar(@beds)." < $g_min_disks_zeroed");
    
}


1; 
