#!/usr/bin/perl

use Cwd;
use File::Spec;
#use Time::localtime;
use POSIX qw(strftime);
use Getopt::Long qw(GetOptions);
use Data::Dumper;

use strict;
use warnings;

my %g_disk_info = ();
my @disks = ();
my $g_fbe_cli_input = "fbe_cli_input.txt";
my $g_fbe_cli_output = "fbe_cli_output.txt";
my $g_passthru_cmd = "fbe_api_object_interface_cli.exe -k -a -phy -phydrive passthru ";
my $g_cdb_out = "PASS_THRU_DATA_IN.BIN";
my $LOG;


my %tla_drivetype_hash = (
			  '005051224M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005049622EFD' => "Hitachi Ralston Peak Refresh 6Gb SAS", 
			  '005049297EFD' => "Hitachi Ralston Peak 6Gb SAS",
			  '005049263EFD' => "Hitachi Ralston Peak 6Gb SAS",
			  '005050540M' => "Samsung RDX MLC 6Gb SAS",
			  '005050523M' => "Samsung RDX MLC 6Gb SAS",
			  '005050500EFD' => "Samsung RDX SLC 6Gb SAS",
			  '005050502EFD' => "Samsung RDX SLC 6Gb SAS",
			  '005050498EFD' => "Samsung RDX SLC 6Gb SAS",
			  '005049185' => "Samsung RCX 3Gb SATA Steeplechase Saddlebag",
			  '005049184' => "Samsung RCX 3Gb SATA Steeplechase Saddlebag",
			  '005050599M' => "Micron Buckhorn 6Gb SAS",
			  '005050112M' => "Micron Buckhorn 6Gb SAS",  
			  '005051215M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051216M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051217M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051218M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051223M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051225M' => "Hitachi SunsetCove Plus 12Gb SAS",
			  '005051226M' => "Hitachi SunsetCove Plus 12Gb SAS",

			 );


my $g_test_dir = get_test_dir();
my $g_log_file = create_file($g_test_dir, "unmap_log.txt");
my $g_log_data_csv = create_file($g_test_dir, "unmap_data.csv");
my $g_log_wa_csv = create_file($g_test_dir, "unmap_wa.csv");
my %test_cycle =();
my $g_num_cycles = 10;
my $g_do_clean = 1;
my $g_io_time = 1 * 60 * 60; # time in seconds 1 hr
my $g_precondition_time = 4 * 60 * 60; # time in seconds 4 hrs
my $g_threads_per_drive = 32;
my $g_rdgen_random_io_size = 16;

open($LOG, ">$g_log_file");
open(my $LOG_DATA, ">$g_log_data_csv");
open(my $LOG_WA, ">$g_log_wa_csv");

GetOptions(
  "d=s" => \@disks,
  "cycles=i" => \$g_num_cycles,
  "clean=i" => \$g_do_clean,
  "io_time=i" => \$g_io_time,
  "p_time=i" => \$g_precondition_time,
  "io_size=i" => \$g_rdgen_random_io_size,
  "h" => \&print_usage
);


print_output("Do clean: $g_do_clean\n");
print_output("Cycles: $g_num_cycles\n");
print_output("IO Cycle Time: $g_io_time\n");

run_random_test();

close($LOG);
close($LOG_DATA);
close($LOG_WA);

sub run_random_test
{
	# step 0a: initialize some stuff and clean the system
	start_up();
	remove_config() if ($g_do_clean);
	
	# step 0b: get information about the disk we want to test
	get_disk_info(@disks);
	dump_disk_info();
	
	# step 0c: create the logical configuration on the disk
	if ($g_do_clean) 
	{
		create_config(0);
	} else {
		discover_config();
	}

	wait_for_disk_zeroing_to_complete();
	validate_pvd_paged_metadata(1, 32, 10) if ($g_do_clean);
	
	# step 1: precondition drive (random 8k write to entire drive)
	precondition_drives(0);
	# TODO validate 1st and 2nd lun
	validate_pvd_paged_metadata(0, 32, 10) if ($g_do_clean);
		
	# step 2: start taking measurements at t0 and t1 per cycle
	start_cycles($g_num_cycles, "mapped");
		
	# step 3: reconfigure with unmap
	remove_config();
		
	# step 4: recreate config to restart unmap
	print_output("RUNNING TEST WITH UNMAP\n");
	create_config(1);
	
	wait_for_disk_zeroing_to_complete();
	validate_pvd_paged_metadata(1, 32, 10) if ($g_do_clean);
	
	# step 5: precondition drive (random 8k write to half drive)
	precondition_drives(1);
	# TODO validate 1st and 2nd lun
	validate_pvd_paged_metadata(0, 32, 10) if ($g_do_clean);
		
		
	# step 2: start taking measurements at t0 and t1 per cycle
	start_cycles($g_num_cycles, "unmapped");
	
	# print the summary to csv
	print_summary($g_num_cycles);
}

sub print_summary
{
	my $num_cycles = shift;
	
	foreach my $location (keys %g_disk_info)
	{
		print $LOG_WA "Location, Drive, Product id, Cycle, Mapped rdgen IOPs, Mapped Host Writes, Mapped Nand Writes, Unmapped rdgen IOPs, Unmapped Host Writes, Unmapped Nand Writes, Mapped WA, Unmapped WA\n";
		foreach my $cycle (1..$num_cycles)
		{
			my $log_info = join ",", 
				$location, 
				$tla_drivetype_hash{$g_disk_info{$location}{tla}}, 
				$g_disk_info{$location}{product_id},
				$cycle, 
				$test_cycle{mapped}{$cycle}->{$location}->{rdgen_iops},
				$test_cycle{mapped}{$cycle}->{$location}->{disk_host_writes},
				$test_cycle{mapped}{$cycle}->{$location}->{nand_writes},
				$test_cycle{unmapped}{$cycle}->{$location}->{rdgen_iops},
				$test_cycle{unmapped}{$cycle}->{$location}->{disk_host_writes},
				$test_cycle{unmapped}{$cycle}->{$location}->{nand_writes},
				$test_cycle{mapped}{$cycle}->{$location}->{write_amp},
				$test_cycle{unmapped}{$cycle}->{$location}->{write_amp};
			$log_info .= "\n";
			print $LOG_WA  $log_info;
			print_output($log_info);
		}
		
		print $LOG_WA "\n";
	}
	

}


sub start_cycles
{
	my $num_cycles = shift;
	my $tag = shift;
	
	print $LOG_DATA "Location,Drive,Product id,Cycle,Rdgen IOPS, Rdgen Resp Time (ms),Rdgen IO count,Host Writes,Nand Writes,Write Amplification, WA2\n";

	foreach my $location (keys %g_disk_info)
	{
		start_random_io($g_disk_info{$location}{lun});
	}
	
	sleep($g_io_time);
	
	foreach my $cycle (1..$num_cycles)
	{
		run_cycle($cycle, $tag);
		
	}
	
	stop_rdgen();
	wait_for_rdgen_complete();
	

}

sub run_cycle
{
	my $cycle_id = shift | 0;
	my $tag = shift;
	my %t0 = ();
	my %t1 = ();

	foreach my $location (keys %g_disk_info)
	{
		# get T0: erase cycles, host writes, NAND writes, ios sent
		$t0{$location} = get_disk_data($location);
	}

	print_output("Sleeping for $g_io_time seconds for cycle $cycle_id\n");
	sleep($g_io_time);
	
	foreach my $location (keys %g_disk_info)
	{
		# get T1: erase cycles, host writes, NAND writes, ios sent
		$t1{$location} = get_disk_data($location);
	}
	
	foreach my $location (keys %g_disk_info)
	{
		$test_cycle{$tag}{$cycle_id}->{$location} = calculate_data($location, $cycle_id, $t0{$location}, $t1{$location});
	}
	
	
	rdgen_reset_stats();
	
	
	print $LOG_DATA "\n";

}

sub calculate_data
{
	my $location = shift;
	my $cycle_id = shift;
	my $t0_ref = shift;
	my $t1_ref = shift;
	
	my %t0 = %$t0_ref;
	my %t1 = %$t1_ref;
	
	my %iteration_info = ();
	
	# calculate T1-T0: erase cycles, host writes, NAND writes, ios sent, WRITE_AMPLIFICATION
	$iteration_info{disk_host_writes} = $t1{disk_host_writes} - $t0{disk_host_writes} || 1;
	$iteration_info{nand_writes} = $t1{nand_writes} - $t0{nand_writes} || 1;
	$iteration_info{rdgen_io_count} = $t1{rdgen_io_count} - $t0{rdgen_io_count};
	$iteration_info{wa2} = 0;
	$iteration_info{rdgen_iops} = $t1{rdgen_iops};
	$iteration_info{rdgen_resp_time_ms} = $t1{rdgen_resp_time_ms};
	
	if (is_drive_micron($location))
	{
		# Write amplification          = (FTL bytes written) / (host bytes written) 
		#			       = (ID 248 RAW value * 8192) / (ID 247 RAW value * 512)

		$iteration_info{write_amp} = ($iteration_info{nand_writes} * 16)/ $iteration_info{disk_host_writes};
		$iteration_info{wa2} = ($iteration_info{nand_writes} / $iteration_info{rdgen_io_count}) * (8192 / ($g_rdgen_random_io_size * 520));
	}
	elsif (is_drive_samsung($location))
	{
		# Samsung RDX - Log Page 2F, Attributes 233 (Host write LBA count) and 235  (Physical Write Count - # of sectors).
		$iteration_info{write_amp} = $iteration_info{nand_writes} / $iteration_info{disk_host_writes};
		$iteration_info{wa2} = $iteration_info{nand_writes} / ($iteration_info{rdgen_io_count} * $g_rdgen_random_io_size);
	}
	elsif (is_drive_hitachi($location))
	{
		# HGST – log page 0x2,  attribute 5  (bytes written)
		# rdgen io count = 8kb per io = 0x2000 bytes
		# UPDATE use total bytes written and total write commands from log page 37
		# $iteration_info{write_amp} = ($iteration_info{nand_writes} / $iteration_info{rdgen_io_count}) / 8192 ;
		# $iteration_info{write_amp} = ($iteration_info{nand_writes} / $iteration_info{disk_host_writes}) / ($g_rdgen_random_io_size * 512);
		# $iteration_info{wa2} = ($iteration_info{nand_writes} / $iteration_info{rdgen_io_count}) / ($g_rdgen_random_io_size * 512);
                # LATEST UPDATE : use host write count (52-59) and nand write count (60-67)
                $iteration_info{write_amp} = $iteration_info{nand_writes} / $iteration_info{disk_host_writes};
                $iteration_info{wa2} = ($iteration_info{nand_writes} * 32 *1024 *1024) / ($iteration_info{rdgen_io_count} * $g_rdgen_random_io_size * 520);
	} 
	else
	{
		error_msg("Unknown drive type $location\n");
	}
	
	$iteration_info{t0} = \%t0;
	$iteration_info{t1} = \%t1;
	
	print_output("================================\n");
	print_output("DISK DATA $location -- $cycle_id\n");
	print_output("t0: rdgen ios:".$t0{rdgen_io_count}." rdgen iops:".$t0{rdgen_iops}." rdgen resp time:".$t0{rdgen_resp_time_ms});
	print_output(" host:".$t0{disk_host_writes}." nand:".$t0{nand_writes}."\n");
	print_output("t1: rdgen ios:".$t1{rdgen_io_count}." rdgen iops:".$t1{rdgen_iops}." rdgen resp time:".$t1{rdgen_resp_time_ms});
	print_output(" host:".$t1{disk_host_writes}." nand:".$t1{nand_writes}."\n");
	print_output("delta: rdgen:".$iteration_info{rdgen_io_count}." host:".$iteration_info{disk_host_writes});
	print_output(" nand:".$iteration_info{nand_writes}." wa:".$iteration_info{write_amp}."\n");
	print_output("================================\n");
	
	
	my $log_info = join ",", 
				$location, 
				$tla_drivetype_hash{$g_disk_info{$location}{tla}}, 
				$g_disk_info{$location}{product_id},
				$cycle_id, 
				$iteration_info{rdgen_iops},
				$iteration_info{rdgen_resp_time_ms},
				$iteration_info{rdgen_io_count},
				$iteration_info{disk_host_writes},
				$iteration_info{nand_writes},
				$iteration_info{write_amp},
				$iteration_info{wa2};
	$log_info .= "\n";
	print $LOG_DATA $log_info;

	return \%iteration_info;

}

=begin
sub run_random_io_cycles
{
	my $num_cycles = shift;
	my $tag = shift;
	
	foreach my $cycle (1..$num_cycles)
	{
		$test_cycle{$tag}{$cycle} = run_one_random_io_cycle($cycle);
	}

	print $LOG_DATA "Location,Drive,Cycle,Rdgen IO count,Host Writes,Nand Writes,Write Amplification, WA2\n";
	
	foreach my $location (keys %g_disk_info)
	{
		foreach my $cycle (1..$num_cycles)
		{
			my $log_info = join ",", 
				$location, $tla_drivetype_hash{$g_disk_info{$location}{tla}}, $cycle, 
				$test_cycle{$tag}{$cycle}->{$location}->{rdgen_io_count},
				$test_cycle{$tag}{$cycle}->{$location}->{disk_host_writes},
				$test_cycle{$tag}{$cycle}->{$location}->{nand_writes},
				$test_cycle{$tag}{$cycle}->{$location}->{write_amp},
				$test_cycle{$tag}{$cycle}->{$location}->{wa2};
			$log_info .= "\n";
			print $LOG_DATA  $log_info;
			print_output($log_info);
		}
	}

	
}

sub run_one_random_io_cycle
{
	my $cycle_id = shift;
	my %disk_cycle  =();
	foreach my $location (keys %g_disk_info)
	{
		$disk_cycle{$location} = run_one_random_io_cycle_on_disk($location, $cycle_id);
		
	}
	
	return \%disk_cycle;
}

# run random test on one disk (this makes it easier to get io count from rdgen. 
# Maybe run all tests at once if it's not necessary to be accurate)
sub run_one_random_io_cycle_on_disk
{
	my $location = shift;
	my $cycle_id = shift | 0;
	my %iteration_info = ();
	
	# reset rdgen stats etc.
	init_rdgen();

	# get T0: erase cycles, host writes, NAND writes, ios sent
	my %t0 = get_disk_data($location);
	
	start_random_io($g_disk_info{$location}{lun});
	print_output("sleeping for $g_io_time seconds\n");
	sleep($g_io_time);
	
	stop_rdgen();
	wait_for_rdgen_complete();
	
	
	# get T1: erase cycles, host writes, NAND writes, ios sent
	my %t1 = get_disk_data($location);
	
	
	
	# calculate T1-T0: erase cycles, host writes, NAND writes, ios sent, WRITE_AMPLIFICATION
	$iteration_info{disk_host_writes} = $t1{disk_host_writes} - $t0{disk_host_writes};
	$iteration_info{nand_writes} = $t1{nand_writes} - $t0{nand_writes};
	$iteration_info{rdgen_io_count} = $t1{rdgen_io_count} - $t0{rdgen_io_count};
	$iteration_info{wa2} = 0;
	
	if (&is_drive_micron($location))
	{
		# Write amplification          = (FTL bytes written) / (host bytes written) 
		#			       = (ID 248 RAW value * 8192) / (ID 247 RAW value * 512)

		$iteration_info{write_amp} = ($iteration_info{nand_writes} * 16)/ $iteration_info{disk_host_writes};
	}
	elsif (is_drive_samsung($location))
	{
		# Samsung RDX - Log Page 2F, Attributes 233 (Host write LBA count) and 235  (Physical Write Count - # of sectors).
		$iteration_info{write_amp} = $iteration_info{nand_writes} / $iteration_info{disk_host_writes};
		$iteration_info{wa2} = $iteration_info{nand_writes} / ($iteration_info{rdgen_io_count} * 16);
	}
	elsif (is_drive_hitachi($location))
	{
		# HGST – log page 0x2,  attribute 5  (bytes written)
		# rdgen io count = 8kb per io = 0x2000 bytes
		# UPDATE use total bytes written and total write commands from log page 37
		#$iteration_info{write_amp} = ($iteration_info{nand_writes} / $iteration_info{rdgen_io_count}) / 8192 ;
		#$iteration_info{write_amp} = ($iteration_info{nand_writes} / $iteration_info{disk_host_writes}) / 8192 ;
		#$iteration_info{wa2} = ($iteration_info{nand_writes} / $iteration_info{rdgen_io_count}) / 8192 ;
	} 
	else
	{
		error_msg("Unknown drive type $location\n");
	}
	
	$iteration_info{t0} = \%t0;
	$iteration_info{t1} = \%t1;
	
	print_output("================================\n");
	print_output("DISK DATA $location -- $cycle_id\n");
	print_output("t0: rdgen:".$t0{rdgen_io_count}." host:".$t0{disk_host_writes}." nand:".$t0{nand_writes}."\n");
	print_output("t1: rdgen:".$t1{rdgen_io_count}." host:".$t1{disk_host_writes}." nand:".$t1{nand_writes}."\n");
	print_output("delta: rdgen:".$iteration_info{rdgen_io_count}." host:".$iteration_info{disk_host_writes});
	print_output(" nand:".$iteration_info{nand_writes}." wa:".$iteration_info{write_amp}."\n");
	print_output("================================\n");
	
	rdgen_reset_stats();
	
	return \%iteration_info;

}
=cut

sub get_disk_data
{
	my $location = shift;
	my %disk_data = ();
	
	print_output("dd $location ".$g_disk_info{$location}{tla}."--\n");
	
	my $rdgen_info = get_rdgen_info($g_disk_info{$location}{lun_obj_id});
	$disk_data{rdgen_io_count} = $rdgen_info->{io_count};
	$disk_data{rdgen_iops} = $rdgen_info->{iops};
	$disk_data{rdgen_resp_time_ms} = $rdgen_info->{resp_time_ms};
	
	if (is_drive_micron($location))
	{
		my $log_30_info = get_log_30($location);
		
		foreach my $attr_code (keys %$log_30_info)
		{
			print_output("$location: $attr_code: ".$log_30_info->{$attr_code}."\n");
		}
		
		$disk_data{nand_writes} = $log_30_info->{248};
		$disk_data{disk_host_writes} =  $log_30_info->{247};
	}
	elsif (is_drive_hitachi($location))
	{
		#my $log_2_info = get_log_2($location);
		#$disk_data{disk_host_writes} = $disk_data{rdgen_io_count};
		#$disk_data{nand_writes} = $log_2_info->{5};
		
		#foreach my $attr_code (keys %$log_2_info)
		#{
		#	print_output("$location: $attr_code: ".$log_2_info->{$attr_code}."\n");
		#}
                # LATEST UPDATE : use host write count (52-59) and nand write count (60-67)
                # $log_37_hash{nand_write_count} = $nand_write_count;
	       # $log_37_hash{host_write_count} = $host_write_count;
		
		my $log_37_info = get_log_37($location);
		$disk_data{disk_host_writes} = $log_37_info->{host_write_count};
		$disk_data{nand_writes} = $log_37_info->{nand_write_count};
		
		foreach my $attr_code (keys %$log_37_info)
		{
			print_output("$location: $attr_code: ".$log_37_info->{$attr_code}."\n");
		}
	
	}
	elsif (is_drive_samsung($location))
	{
		my $log_2f_info = get_log_2f($location);
		$disk_data{disk_host_writes} = $log_2f_info->{233};
		$disk_data{nand_writes} = $log_2f_info->{235};
		
		foreach my $attr_code (keys %$log_2f_info)
		{
			print_output("$location: $attr_code: ".$log_2f_info->{$attr_code}."\n");
		}
	
	}
	else 
	{
		assert_true(0,"unknown drive type $location\n");
	}
	
	return \%disk_data;
}

sub is_drive_micron
{
	my $location = shift;
	
	print_output("$location drivetype ".$tla_drivetype_hash{$g_disk_info{$location}{tla}}."\n");		
	if ($tla_drivetype_hash{$g_disk_info{$location}{tla}} =~ /buckhorn/i) {
		return 1;
	}
	else
	{
		return 0;
	}
}

sub is_drive_hitachi
{
	my $location = shift;
		
	if ($tla_drivetype_hash{$g_disk_info{$location}{tla}} =~ /hitachi/i) {
		return 1;
	}
	else
	{
		return 0;
	}
	
}

sub is_drive_samsung
{
	my $location = shift;
	
	if ($tla_drivetype_hash{$g_disk_info{$location}{tla}} =~ /samsung/i) {
		return 1;
	}
	else
	{
		return 0;
	}
	
}

sub get_log_2f
{
	my $location = shift;
	
	my $cdb_file = "LOG_SENSE_2F_CDB.BIN";

	my $cmd = $g_passthru_cmd." $location --cdb_file $cdb_file";
	my $output = `$cmd`;	
	if ($output =~ /check/i) {
		print_output("log sense 2f fail $location\n");
	} else {
		return parse_log_2f($g_cdb_out, $location);
	}
	
}

sub parse_log_2f
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my %log_2f_hash = ();
	my $data;

	open(INFILE, "<", $file);
	binmode(INFILE);
	read(INFILE, $data, 17);   # attr header
	
	my $i = 0;

	my $end_flag = 0;
	# parse parameters
	while (!$end_flag)
	{
	    $i++;
	    #my $data;
	    # parse attr header
	    read(INFILE, my $attr_header, 5);   # attr info
	    my @attr_info = unpack 'C*', $attr_header;
	    my $attr_code = $attr_info[0];
	    
	    if ($attr_code == 235)
	    {
	    	$end_flag = 1;
	    }
	    
	    #my $result = 0;
	    #for (my $index = 5; $index <= 10; $index++)
	    #{
	    	
	    #	$result = ($result << 8) + $attr_info[$index];
	    #	print_output("result $result");
	    #}
	    
	    read(INFILE, my $attr_data, 6);   # attr info
	    my $result = hex(unpack 'H*', $attr_data);
	    
	    $log_2f_hash{$attr_code} = $result; 
	    
	    #print_output("log2 $attr_code: $result\n");
	    #$log31 .= sprintf ("param:%x              field:%s data:0x%x \n", $param, $FIELD[$field], $result);
	    
	    read(INFILE, my $data, 1);   # attr info #junk
			
	}
	
	
	close(INFILE);
	return \%log_2f_hash;
}

sub get_log_37
{
	my $location = shift;
	
	my $cdb_file = "LOG_SENSE_37_CDB.BIN";

	my $cmd = $g_passthru_cmd." $location --cdb_file $cdb_file";
	my $output = `$cmd`;	
	if ($output =~ /check/i) {
		print_output("log sense 37 fail $location\n");
	} else {
		return parse_log_37($g_cdb_out, $location);
	}
	
}

sub parse_log_37
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my %log_37_hash = ();
	my $data;

	open(INFILE, "<", $file);
	binmode(INFILE);
	
	read(INFILE, $data, 20);   # junk
	read(INFILE, my $nand_data, 8);
	read(INFILE, $data, 13);   #junk
	read(INFILE, my $host_data, 8);
        read(INFILE, $data, 3);   #junk
        read(INFILE, my $host_write_count, 8); # 52-59
        read(INFILE, my $nand_write_count, 8); #60-67
	
	my $raw_nand = hex(unpack 'H*', $nand_data);
	my $raw_host = hex(unpack 'H*', $host_data);
        my $raw_nand_write_count = hex(unpack 'H*', $nand_write_count);
	my $raw_host_write_count = hex(unpack 'H*', $host_write_count);

	$log_37_hash{nand_bytes_written} = $raw_nand;
	$log_37_hash{host_write_commands} = $raw_host;
        $log_37_hash{nand_write_count} = $raw_nand_write_count;
	$log_37_hash{host_write_count} = $raw_host_write_count;

	close(INFILE);
	return \%log_37_hash;
}


sub get_log_2
{
	my $location = shift;
	
	my $cdb_file = "LOG_SENSE_2_CDB.BIN";

	my $cmd = $g_passthru_cmd." $location --cdb_file $cdb_file";
	my $output = `$cmd`;	
	if ($output =~ /check/i) {
		print_output("log sense 2 fail $location\n");
	} else {
		return parse_log_2($g_cdb_out, $location);
	}
	
}

sub parse_log_2
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my %log_2_hash = ();
	my $data;

	open(INFILE, "<", $file);
	binmode(INFILE);
	read(INFILE, $data, 17);   # attr header
	
	my $i = 0;

	my $end_flag = 0;
	# parse parameters
	while (!$end_flag)
	{
	    $i++;
	    #my $data;
	    # parse attr header
	    read(INFILE, my $attr_header, 5);   # attr info
	    my @attr_info = unpack 'C*', $attr_header;
	    my $attr_code = $attr_info[0];
	    
	    if ($attr_code == 5)
	    {
	    	$end_flag = 1;
	    }
	    
	    #my $result = 0;
	    #for (my $index = 5; $index <= 10; $index++)
	    #{
	    	
	    #	$result = ($result << 8) + $attr_info[$index];
	    #	print_output("result $result");
	    #}
	    
	    read(INFILE, my $attr_data, 6);   # attr info
	    my $result = hex(unpack 'H*', $attr_data);
	    
	    $log_2_hash{$attr_code} = $result; 
	    
	    #print_output("log2 $attr_code: $result\n");
	    #$log31 .= sprintf ("param:%x              field:%s data:0x%x \n", $param, $FIELD[$field], $result);
	    
	    read(INFILE, my $data, 1);   # attr info #junk
			
	}
	
	
	close(INFILE);
	return \%log_2_hash;
}



sub get_log_30 
{
	my $location = shift;
	my $cdb_file = "LOG_SENSE_30_CDB.BIN";

	my $cmd = $g_passthru_cmd." $location --cdb_file $cdb_file";
	my $output = `$cmd`;	
	print_output($cmd."\n");
	print_output($output."\n");
	if ($output =~ /check/i) {
		print_output("log sense 30 fail $location\n");
	} else {
		return parse_log_30($g_cdb_out, $location);
	}

}

sub parse_log_30
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my %log_30_hash = ();
	my $data;

	open(INFILE, "<", $file);
	binmode(INFILE);
	read(INFILE, $data, 6);   # attr header
	
	my $i = 0;

	my $end_flag = 0;
	# parse parameters
	while (!$end_flag)
	{
	    $i++;
	    my $data;
	    # parse attr header
	    read(INFILE, $data, 12);   # attr info
	    my @attr_info = unpack 'C*', $data;
	    my $attr_code = $attr_info[0];
	    
	    if ($attr_code == 248)
	    {
	    	$end_flag = 1;
	    }
	    
	    my $result = 0;
	    for (my $index = 10; $index >= 5; $index--)
	    {
	    	$result = ($result << 8) + $attr_info[$index];
	    }
	    
	    $log_30_hash{$attr_code} = $result; 
	    
	    #print_output("log30 $attr_code: $result\n");
	    #$log31 .= sprintf ("param:%x              field:%s data:0x%x \n", $param, $FIELD[$field], $result);
		
	}
	
	
	close(INFILE);
	return \%log_30_hash;
}


# check a small portion of the paged metadata
sub validate_pvd_paged_metadata
{
	my $expected_nz_bit = shift | 0;
	my $chunk_index = shift | 32;
	my $chunk_count = shift | 10;
	
	foreach my $location (keys %g_disk_info)
	{
		my $pvd_id = $g_disk_info{$location}{pvd_id};
		my $cmd = "pvdinfo -object_id $pvd_id -chunk_index $chunk_index -chunk_count $chunk_count";
		my @output = run_fbe_cli_cmd($cmd);
		
		my $match_flag = 0;
		foreach my $line (@output)
		{
			chomp($line);
			if ($line =~ /0x\d+\s+\|\s*Valid\s*\|\s*Need Zero\s*\|\s*User Zero/ig)
			{
				$match_flag = 1;
			}
			elsif ($match_flag) 
			{
				$line =~ s/\s+//ig;
				my @fields = split /\|/, $line;
				my $v = $fields[1]; #valid
				my $nz = $fields[2]; #need_zero
				my $uz = $fields[3]; #user_zero
				my $cu = $fields[4]; #consumed
				
				print_output($fields[0]." v:$v nz:$nz uz:$uz cu:$cu\n");
				assert_true($v == 1, "invalid valid bit $v\n");
				assert_true($nz == $expected_nz_bit, "invalid nz bit $nz $expected_nz_bit\n");
				assert_true($cu == 1, "invalid consumed bit $cu\n");
			
			}
		
		}
	
	} 


}

sub precondition_drives
{
	my $is_unmap_test = shift;
	

	# write sequential to twice capacity of drive 
	foreach my $i (1..2) {
		print_output("Writing to entire capacity of all drives sequential iteration $i\n");

		if ($is_unmap_test)
		{
			write_sequential_half_luns();
		}
		else
		{
			write_sequential_all_luns();
		}
		
		wait_for_rdgen_complete();
	}
	
	
	# write random 8k to all drives
	foreach my $location (keys %g_disk_info)
	{
		# step 1: precondition drive (random 8k write )
		precondition_drive($location, $is_unmap_test);			
	}
	
	print_output("Sleeping for $g_precondition_time seconds\n");
	sleep($g_precondition_time);
	
	stop_rdgen();
	wait_for_rdgen_complete();
}

sub precondition_drive
{
	my $location = shift;
	my $is_unmap_test = shift;
	
	print_output("Preconditioning drive $location is_unmap: $is_unmap_test\n");
	
	my $num_luns = $g_disk_info{$location}{num_luns_per_rg};
	my $lun_id = $g_disk_info{$location}{lun};
	$num_luns = 1 if ($is_unmap_test);
	foreach my $i (1..$num_luns) {
		print_output("Starting random IO to lun $lun_id\n");
		start_random_io($lun_id);
		$lun_id++;
	}
}

# run 8k random writes to lun
sub start_random_io
{
	my $lun_id = shift;
	my $num_threads_base_10 = shift || 32;
	
	my $num_threads_hex = sprintf("%x", $num_threads_base_10);
	my $rdgen_io_size = sprintf("%x", $g_rdgen_random_io_size);
	
	my $cmd = "rdgen -constant -align -perf w $lun_id $num_threads_hex $rdgen_io_size";
	run_fbe_cli_cmd($cmd);
}

# get the historical io count
sub get_rdgen_info
{
	my $lun_obj_id = shift;
	my %rdgen_info = ();
	
	my $cmd = "rdgen -s";
	my @output = run_fbe_cli_cmd($cmd);

	# object_id: 308 interface: Packet threads: 1 ios: 0x3b3d passes: 0x3b3d errors: 0
	# rate: 179.39 avg resp tme: 5.5449 
	
	my $match_flag = 0;		
	foreach my $line (@output) 
	{
		chomp($line);
		#print_output($line."\n");
		if ($line =~ /object_id\: (\d+) interface\:/ig)
		{
			my $object_id = $1;
			$match_flag = 1 if ($object_id == $lun_obj_id);
		}
		
		if ($match_flag) 
		{
			print_output($line."\n");
			if ($line =~ /ios\: (0x\w+) passes\:/ig) 
			{
				$rdgen_info{io_count} = hex($1);
				
			}
			
			if ($line =~ /rate\: (\d+\.\d+) avg resp tme\: (\d+\.\d+)/ig) 
			{
				$rdgen_info{iops} = $1;
				$rdgen_info{resp_time_ms} = $2;
				
				print_output("rdgen $lun_obj_id ios:".$rdgen_info{io_count}."\n");
				print_output("rdgen $lun_obj_id iops ".$rdgen_info{iops}." resp_time:".$rdgen_info{resp_time_ms}."\n");
				return \%rdgen_info;
			}
		}
	}
	
	return \%rdgen_info;

}


# write sequential to all luns
sub write_sequential_all_luns
{
	foreach my $location (keys %g_disk_info)
	{
		my $num_luns = $g_disk_info{$location}{num_luns_per_rg};
		my $lun_id = $g_disk_info{$location}{lun};
		foreach my $i (1..$num_luns) {
			write_sequential_to_lun($lun_id);
			$lun_id++;
		}
	
	} 
}

# write sequential to all luns
sub write_sequential_half_luns
{
	foreach my $location (keys %g_disk_info)
	{
		write_sequential_to_lun($g_disk_info{$location}{lun});	
	} 
}

# write sequential io to lun for 1 pass
sub write_sequential_to_lun
{
	my $lun_id = shift;
	
	my $cmd = "rdgen -constant -perf -cat_inc -start_lba 0 -pass_count 1 w $lun_id 10 400";
	run_fbe_cli_cmd($cmd);
}



# get the historical io count
sub get_rdgen_io_count
{
	my $cmd = "rdgen -d";
	my @output = run_fbe_cli_cmd($cmd);
		
	# Historical Statistics (Totals for threads that have completed) Reset with rdgen -reset_stats.
	# --------------------
	# io count            : 64172

	my $match_flag = 0;		
	foreach my $line (@output) 
	{
		chomp($line);
		#print_output($line."\n");
		if ($line =~ /Historical Statistics/ig)
		{
			$match_flag = 1;
		}
		
		if ($match_flag) 
		{
			if ($line =~ /io count\s+\:\s+(\d+)/ig) 
			{
				my $io_count = $1;
				return $io_count;
			}
		}
	}
	
	assert_true(0, "Unable to get IO count\n");

}

# wait for all active threads to complete
sub wait_for_rdgen_complete
{

	print_output("Waiting for rdgen threads to complete");
	while (1) 
	{
		my $cmd = "rdgen -d";
		my @output = run_fbe_cli_cmd($cmd);
		
		#Active Thread Statistics (Totals for threads currently running).
		#--------------------
		#threads           : 0
		
		foreach my $line (@output) {
			if ($line =~ /threads\s+:\s(\d+)/ig)
			{
				my $thread_count = $1;
				if ($thread_count == 0)
				{
					print_output($line."\n");
					return;
				}
			}
		}
		
		sleep(30);
	}
}

# print the disk information in the hash
sub dump_disk_info
{
	print Dumper(%g_disk_info);
}

# wait for zeroing to complete on all target drives
sub wait_for_disk_zeroing_to_complete
{

	my $is_zeroing_complete = 0;
	
	while (!$is_zeroing_complete) 
	{
		my @output = run_fbe_cli_cmd("pvdinfo -list");
		$is_zeroing_complete = 1;
		foreach my $line (@output)
		{
			chomp($line);
			if ($line =~ /flash/i) {
				my @fields = split /\s+/, $line;
				my $location = normalize_location($fields[1]);
				my $pvd_id = $fields[0];
				my $zeroing_percent = $fields[6];
				#$location =~ s/\s//ig;
				if (grep {$location eq $_} @disks) {
					if ($zeroing_percent ne '100%')
					{
						$is_zeroing_complete = 0;
					}
					print_output("Disk $location zeroing: $zeroing_percent ".$fields[6]." ".$fields[5]."\n");
				}
			}
		}
		sleep(30);
	}

}

# create the config with the drives we want to run the test against
sub create_config
{
	my $is_unmap_test = shift || 0;
	my $rg_id = 0;
	my $lun_id = 0;
	my $num_luns_per_rg = 1;
	$num_luns_per_rg = 2 if ($is_unmap_test);
	foreach my $location (keys %g_disk_info)
	{
		# enable unmap so that all of pvd metadata has need zero set
		if ($tla_drivetype_hash{$g_disk_info{$location}{tla}} =~ /micron/i) {
			run_fbe_cli_cmd("pvdinfo -object_id ".$g_disk_info{$location}{pvd_id}." -enable_unmap");
		}
	
		my $create_rg_cmd = "naviseccli createrg $rg_id $location -raidtype disk -o";
		run_command($create_rg_cmd);
		
		# lun id = rg id
		my $create_lun_cmd = "naviseccli bind id $lun_id -rg $rg_id -cnt $num_luns_per_rg -noinitialverify";
		run_command($create_lun_cmd);
		
		$g_disk_info{$location}{rg} = $rg_id;
		$g_disk_info{$location}{lun} = $lun_id;
		$g_disk_info{$location}{num_luns_per_rg} = $num_luns_per_rg;
		$g_disk_info{$location}{lun_obj_id} = get_lun_object_id($lun_id);
		
		$rg_id++;
		$lun_id += $num_luns_per_rg;
	}
}

sub discover_config
{
	foreach my $location (keys %g_disk_info)
	{
		my @output = `naviseccli getdisk $location -rg -lun`;
		foreach my $line (@output) 
		{
			chomp($line);
			if ($line =~ /Raid Group ID\:\s+(\d+)/ig)
			{
				$g_disk_info{$location}{rg} = $1;
				
				print_output($location." rg:".$g_disk_info{$location}{rg}."\n");
			}
			
			if ($line =~ /Lun\:\s+(.*)/ig)
			{
				my $lun_list = $1;
				my @lun_ids = split /\s/, $lun_list;
				
				$g_disk_info{$location}{lun} = $lun_ids[0];
				$g_disk_info{$location}{num_luns_per_rg}  = scalar(@lun_ids);
				$g_disk_info{$location}{lun_obj_id} = get_lun_object_id($lun_ids[0]);
				
				print_output("$location: lun: ".$g_disk_info{$location}{lun}." ".$g_disk_info{$location}{lun_obj_id} ." num_luns:".$g_disk_info{$location}{num_luns_per_rg}."\n" );
				
				

			}

		}
	
	}


}

sub get_lun_object_id
{
	my $lun_id = shift;
	my @output = run_fbe_cli_cmd("luninfo -lun $lun_id");
	foreach my $line (@output)
	{
		chomp($line);
		if ($line =~ /Lun Object\-id\:\s+(0x\w+)/)
		{
			my $lun_obj_id = $1;
			return hex($lun_obj_id);	
		}
	}
}


sub remove_config
{
	
	my $cmd = "naviseccli getlun -state";
	my @output = `$cmd`;
	
	foreach my $line (@output) 
	{
		if ($line =~ /LOGICAL UNIT NUMBER (\d+)/ig)
		{
			run_command("naviseccli unbind $1 -o");
		}
	
	}
	
	@output = `naviseccli getrg -state`;
	foreach my $line (@output) {
		if ($line =~ /RaidGroup ID:\s+(\d+)/ig)
		{
			run_command("naviseccli removerg $1 -o");
		}
	}



}



# populate the disk information
sub get_disk_info 
{
	my @disks = @_;
	
	my %pvd_hash = ();
	
	my @output = run_fbe_cli_cmd("pvdinfo -list");
	foreach my $line (@output)
	{
		if ($line =~ /flash/i) {
			my @fields = split /\s+/, $line;
			my $location = $fields[1];
			my $pvd_id= $fields[0];
			$location =~ s/\s//ig;
			$pvd_hash{$location}{pvd_id} = $pvd_id;
			
			#print_output("$location normalized_pvd_loc: ".normalize_location($location)."\n");
			
		}
	}
	
	@output = run_fbe_cli_cmd("pdo -abr");

	foreach my $line (@output)
	{
		if ($line =~ /flash/i) {
			chomp($line);
			my @fields = split /\|/, $line;
			my $location = $fields[3];
			my $vendor = $fields[4];
			my $product_id = $fields[5];
			my $tla = $fields[8];
			$tla =~ s/\s//ig;
			$location =~ s/\s//ig;

			
			my ($bus, $enclosure, $slot) = split /\_/, $location;
			my $pvd_location = sprintf("%02d", $bus)."_".sprintf("%02d", $enclosure)."_".sprintf("%02d", $slot);
			
			if (grep {$location eq $_} @disks) 
			{
			
				#print_output("use $pvd_location: $tla $vendor\n");
				$g_disk_info{$location}{pvd_id} = $pvd_hash{$pvd_location}{pvd_id};
				$g_disk_info{$location}{tla} = $tla;
				$g_disk_info{$location}{vendor} = $vendor;
				$g_disk_info{$location}{product_id} = $product_id;
			}
		}
	}

}

sub normalize_location
{
	my $pvd_location = shift;
	
	my ($b, $e, $s) = split /\_/, $pvd_location;
	
	$pvd_location = ($b+0)."_".($e+0)."_".($s +0);
	
	return $pvd_location;	
	
}

sub assert_true
{
	my $conditional = shift;
	my $err_msg = shift;
	
	if (!$conditional) 
	{
		print_output("ERROR ".$err_msg);
		exit(0);
	}
	

}

# star neit, init rdgen, and reset stats
sub start_up
{
	start_neit();
	stop_rdgen();
	init_rdgen();
	rdgen_reset_stats();
}

sub start_neit
{
	run_command("net start newneitpackage");
}

sub init_rdgen
{
	my @cmds = (
			"rdgen -init_dps 500",
			"rdgen -dp",
			"rdgen -term",
			"rdgen -reset_stats"
		   );
	run_fbe_cli_cmds(\@cmds);
}

sub stop_rdgen
{
	my @cmds = ("rdgen -dp",
	            "rdgen -term");
	run_fbe_cli_cmds(\@cmds);

}

sub rdgen_reset_stats 
{
	my $cmd = "rdgen -reset_stats";
	
	run_fbe_cli_cmd($cmd);
}


sub run_command
{
	my $cmd = shift;
	
	print_output($cmd."\n");
	my $ret = system($cmd);
}

# run the fbe_cli command
sub run_fbe_cli_cmd
{
	my $cmd = shift;
	
	open(FILE, ">$g_fbe_cli_input");
	print FILE "acc -m 1\n";
	print FILE "$cmd\n";
	print_output("acc -m 1\n");
	print_output("$cmd\n");
	close(FILE);
	
	my $garbage = `fbecli z$g_fbe_cli_input x$g_fbe_cli_output`;
	
	open(OUTPUT, $g_fbe_cli_output);
	my @data = <OUTPUT>;
	close(OUTPUT);
	
	return @data;
	
}

# run the array of commands
sub run_fbe_cli_cmds
{
	my $cmdref = shift;
	
	my @cmds = @$cmdref;
	
	open(FILE, ">$g_fbe_cli_input");
	print FILE "acc -m 1\n";
	print_output("acc -m 1\n");
	foreach my $cmd (@cmds) {
		print FILE "$cmd\n";
		print_output("$cmd\n");
	}
	close(FILE);
	
	my $garbage = `fbecli z$g_fbe_cli_input x$g_fbe_cli_output`;	
	
	open(OUTPUT, $g_fbe_cli_output);
	my @data = <OUTPUT>;
	close(OUTPUT);
	
	return @data;
}

# print to the window and print to the log
sub print_output
{
	my $string = shift;
	print $string;
	print $LOG $string;
}

# get the test directory in which we want to run the test
sub get_test_dir
{
	my $dir = "wa_".strftime("%Y_%m_%d_%H_%M_%S", localtime);
	return create_dir(getcwd(), $dir);

}

# create file with the specified name and path
sub create_file
{
	my $dir = shift;
	my $file = shift;
	return File::Spec->catfile($dir, $file);
}

# create directory with the specified name and path
sub create_dir
{
	my $parent = shift;
	my $dir = shift;
	my $new_dir = File::Spec->catdir($parent, $dir);
	mkdir $new_dir;
	return $new_dir;
	
}


# print the usage of the script
sub print_usage {
	print "\nUSAGE: \n\n";
	print "write_amplification_test.pl -d <b_e_s> -d <b_e_s> ...\n";
	print "\t\t -cycles <natural number>        - number of random io cycles to run after precondition\n";
	print "\t\t -clean <0|1>		    - clean the config\n";
	print "\t\t -io_time <0|1>		    - io time per cycle in seconds\n";
	print "\t\t -p_time <0|1>		    - precondition time in seconds\n";
	
	print "\n";
	exit(1);
}


1;