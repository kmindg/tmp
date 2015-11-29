#!/usr/bin/perl

use Cwd;
use File::Spec;
#use Time::localtime;
use POSIX qw(strftime);
use Getopt::Long qw(GetOptions);

use strict;
use warnings;

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
			 
my $fbe_cli_input = "fbe_cli_input.txt";
my $fbe_cli_output = "fbe_cli_output.txt";
my $passthru_cmd = "fbe_api_object_interface_cli.exe -k -a -phy -phydrive passthru ";
my $CDB_OUT = "PASS_THRU_DATA_IN.BIN";

our $LOG;
our $test_dir = get_test_dir();;
our $test_log_dir = create_dir($test_dir, "cli_logs");;
our $unmap_summary_file = create_file($test_dir, "unmap_summary.csv");
our $log_file = create_file($test_dir, "unmap_log.txt");
open($LOG, ">$log_file");


my @user_disks = ();
my $run_benchmark = 0;

GetOptions(
  "d=s" => \@user_disks,
  "r"=> \$run_benchmark,
  "h" => \&print_usage
);

print "run benchmark: $run_benchmark\n";

my $temp = get_flash_disks();
my @disks = @{$temp};

if (scalar(@user_disks) > 0 && $user_disks[0] ne "all") {
	my @filtered = ();
	foreach my $disk (@disks)
	{
		if (grep {$_ eq $disk->{location}} @user_disks) 
		{
			push @filtered, $disk;	
		}
	}
	@disks = @filtered;
}

startup();

foreach my $disk (@disks) {
	print $disk->{location}." \t ".$disk->{vendor}."\t".$disk->{tla}."\n";
}
write_background_pattern(\@disks);
foreach my $disk (@disks) {
   	print_output("\n====================================\n");
   	print_output($disk->{location}." \t ".$disk->{vendor}."\t".$disk->{tla}."\n");
   	print_output("====================================\n");
   	get_log31($disk);
   	get_vpd_page_b2($disk);
   	get_vpd_page_b0($disk);
   	send_unmap($disk);
   	send_write_same_with_unmap($disk);
   	# unmap unmapped blocks
   	send_unmap($disk, 1);
   	send_write_same_with_unmap($disk, 1);
}

if ($run_benchmark) {
	verify_rdgen_unmap(@disks);
	run_unmap_benchmark(@disks);
} else {
	foreach my $disk (@disks)
	{
		$disk->{status}->{rdgen_unmap} = -1;	
	}
}
print_summary(@disks);
cleanup();



			 
sub print_summary
{
	my @disks = @_;
	
	#my $file = create_file($test_dir, $unmap_summary_file);
	open(SUMMARY, ">$unmap_summary_file");
	print SUMMARY "TLA,DRIVETYPE,FIRMWARE,LOCATION,LOG31,VPD_B0,VPD_B2,UNMAP,WS_UNMAP,UNMAP UNMAPPED,WS_UNMAP UNMAPPED, RDGEN UNMAP\n";
	print_output("\n\nSUMMARY\n\n");
	foreach my $disk (sort {$a->{tla} cmp $b->{tla}} @disks) {
		print_output($disk->{location}." \t ".$disk->{vendor}."\t".$disk->{tla}."\t".$disk->{firmware}."\n");
		print_output("FAILURES: ".join(", ", @{$disk->{failures}})."\n");
		print_output("\n");
		
		my $drive_type = $tla_drivetype_hash{$disk->{tla}} || "";
		my $string = join ",", ($disk->{tla}, $drive_type, $disk->{firmware}, $disk->{location}, status_to_string($disk->{status}->{log31}),
		                        status_to_string($disk->{status}->{vpd_b0}), $disk->{vpd_b2}, 
		                        status_to_string($disk->{status}->{unmap}), status_to_string($disk->{status}->{ws_unmap}), 
		                        status_to_string($disk->{status}->{unmap_unmapped}), status_to_string($disk->{status}->{ws_unmap_unmapped}), 
		                        status_to_string($disk->{status}->{rdgen_unmap}));
		
		my $test_results = 
		print SUMMARY $string."\n";
	
	}
	
	close(SUMMARY);
	
}

sub status_to_string
{
	my $status = shift;
	
	if ($status == 0)
	{
		return "OK";
	}
	elsif ($status == 1)
	{
		return "FAIL";	
	} 
	elsif ($status == 2)
	{
		return "FAIL";
	}
	elsif ($status == -1) 
	{
		return "N/A";	
	}
		
}

sub run_fbe_cli_cmd
{
	my $cmd = shift;
	my $log_handle = shift;
	
	open(FILE, ">$fbe_cli_input");
	print FILE "acc -m 1\n";
	print FILE "$cmd\n";
	print $log_handle "acc -m 1\n" if ($log_handle);
	print $log_handle "$cmd\n" if ($log_handle);
	close(FILE);
	
	my $garbage = `fbecli z$fbe_cli_input x$fbe_cli_output`;
	
	open(OUTPUT, $fbe_cli_output);
	my @data = <OUTPUT>;
	close(OUTPUT);
	
	return @data;
	
}

sub run_fbe_cli_cmds
{
	my $cmdref = shift;
	my $log_handle = shift;
	
	my @cmds = @$cmdref;
	
	open(FILE, ">$fbe_cli_input");
	print FILE "acc -m 1\n";
	print $log_handle "acc -m 1\n" if ($log_handle);
	foreach my $cmd (@cmds) {
		print FILE "$cmd\n";
		print $log_handle "$cmd\n" if ($log_handle);
	}
	close(FILE);
	
	my $garbage = `fbecli z$fbe_cli_input x$fbe_cli_output`;	
	
	open(OUTPUT, $fbe_cli_output);
	my @data = <OUTPUT>;
	close(OUTPUT);
	
	return @data;
}

sub print_output
{
	my $string = shift;
	print $string;
	print $LOG $string;
}

sub get_flash_disks
{
	my @output = run_fbe_cli_cmd("pvdinfo -list");
	my %pvd_hash = ();
	foreach my $line (@output)
	{
		if ($line =~ /flash/i) {
			my @fields = split /\s+/, $line;
			my $location = $fields[1];
			my $pvd_id= $fields[0];
			#print "$location = $pvd_id\n";
			$location =~ s/\s//ig;
			$pvd_hash{$location} = $pvd_id;
		}
	}
	
	@output = run_fbe_cli_cmd("pdo -abr");
	my @flash = ();

	foreach my $line (@output)
	{
		if ($line =~ /flash/i) {
			chomp($line);
			my @fields = split /\|/, $line;
			my $location = $fields[3];
			my $vendor = $fields[4];
			my $product_id = $fields[5];
			my $firmware_rev = $fields[6];
			my $tla = $fields[8];
			$tla =~ s/\s//ig;
			$location =~ s/\s//ig;
			my %disk = (location => $location, vendor => $vendor, tla => $tla, firmware => $firmware_rev, product_id => $product_id );
			
			my ($bus, $enclosure, $slot) = split /\_/, $location;
			my $pvd_location = sprintf("%02d", $bus)."_".sprintf("%02d", $enclosure)."_".sprintf("%02d", $slot);
			#print "$pvd_location\n";
			$disk{pvd_object_id} = $pvd_hash{$pvd_location};
			
			my @failures = ();
			$disk{failures} = \@failures;
			
			my %status = (log31 => 0, vpd_b0 => 0, vpd_b2 => 0, unmap => 0, ws_unmap => 0, unmap_unmapped => 0, ws_unmap_unmapped => 0);
			$disk{status} = \%status;
			
			push @flash, \%disk;
		}
	}
	
	return \@flash;

}

sub get_log31 
{
	my $disk = shift;
	my $cdbfile = "LOG_SENSE_31_CDB.BIN";
	my $cmd = $passthru_cmd .$disk->{location}." --cdb_file ".$cdbfile;
	my $output = `$cmd`;	
	
	if ($output =~ /check/i) {
		error_msg("log 31 fail", $disk);
		$disk->{status}->{log31} = 1;
	} else {
	
		parse_log31($CDB_OUT);
	}


}

sub parse_log31
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	
	print_output("\nLOG 31\n");
	print_output("-------------------------------------\n");
	my @FIELD = qw(MAX_ALL_CHANNELS MAX_CHANNEL MIN_CHANNEL AVG_CHANNEL);

	open(INFILE, "<", $file);
	binmode(INFILE);
	my $data;
	# parse log header
	read(INFILE, $data, 4);
	my @log_header = unpack 'C*', $data;
	my $page = $log_header[0] & 0x3f; 
	my $subpage = $log_header[1];
	my $log_len = ($log_header[2] << 8) + $log_header[3];
	
	my $log31 = sprintf("page:%x sub:%x \n", $page, $subpage);
	
	my $remaining = $log_len;
	# parse parameters
	while ($remaining > 0)
	{
	    read(INFILE, $data, 4);   #param header
	    $remaining -= 4;
	    my @param_header = unpack 'C*', $data;
	
	    my $param = ($param_header[0] << 8) + $param_header[1];
	    my $param_len = $param_header[3];
	
	    read(INFILE, $data, $param_len);  #param data
	    $remaining -= $param_len;
	    my @param_data = unpack 'C*', $data;
	
	    my $result = 0;
	    foreach my $d(@param_data)
	    {
	        $result = ($result << 8) + $d;
	    }
	
	    my $channel = ($param & 0x0ff0) >> 4;
	    my $field = $param & 0x000f;
	    
	    if ($field == 0)
	    {
	        $log31 .= sprintf ("param:%x              field:%s data:0x%x \n", $param, $FIELD[$field], $result);
	    }
	    elsif ($field >= 1 && $field <= 3)
	    {
	        $log31 .= sprintf ("param:%x channel:0x%02x field:%-16s data:0x%x \n", $param, $channel, $FIELD[$field], $result);
	    }
	    elsif ($field == 0xe)
	    {
	        $log31 .= sprintf ("param:%x              field:%-16s data:0x%x \n", $param, "POH", $result);
	    }
	    elsif($field == 0xf)
	    {        
	        $log31 .= sprintf ("param:%x              field:%-16s data:0x%x \n", $param, "EOL_PE_CYCLES", $result);
	    }
	    else
	    {
	        $log31 .= sprintf ("param:%x              field:%-16s data:0x%x \n", $param, "UNKNOWN", $result);
	    }
	}
	
	
	close(INFILE);
	print_output($log31);
	return $log31;
}

sub get_vpd_page_b0 {
	my $disk = shift;
	my $cdbfile = "VPD_B0_CDB.BIN";
	my $cmd = $passthru_cmd .$disk->{location}." --cdb_file ".$cdbfile;
	my $output = `$cmd`;	
	if ($output =~ /check/i) {
		error_msg("vpd b0 fail", $disk);
		$disk->{status}->{vpd_b0} = 1;
	} else {
		parse_vpd_page_b0($CDB_OUT, $disk);
	}
}

sub parse_vpd_page_b0
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my $disk = shift;
	
	print_output("\nVPD B0\n");
	print_output("-------------------------------------\n");
	my @FIELD = qw(MAX_ALL_CHANNELS MAX_CHANNEL MIN_CHANNEL AVG_CHANNEL);

	open(INFILE, "<", $file);
	binmode(INFILE);
	my $data;
	my $offset = 4;
	my $n= 0;
	# parse header
	$n = read(INFILE, $data, 4);
	
	my @log_header = unpack 'C*', $data;
	my $page = $log_header[1] & 0xFF; 
	my $page_len = ($log_header[2] << 8) + $log_header[3];
	
	my $vpd_b0 = sprintf("page:%x length:%x \n", $page, $page_len);
	my @fields = ();
	
	#while (($n = read(INFILE, my $temp, 4)) != 0) {
	#
	#  push @fields, unpack('H*', $temp)."\n";
	#}
	read(INFILE, $data, 4);
	read(INFILE, my $max_xfer_length, 4);
	read(INFILE, $data, 8);
	read(INFILE, my $max_unmap_lba_count, 4);
	read(INFILE, my $max_unmap_block_descriptor_count, 4);
	read(INFILE, my $optimal_unmap_granularity, 4);
	read(INFILE, my $unmap_granularity_alignment, 4);
	read(INFILE, my $max_write_same_length, 8);

	$max_xfer_length = hex(unpack 'H*', $max_xfer_length);
	$max_unmap_lba_count = hex(unpack 'H*', $max_unmap_lba_count);
	$max_unmap_block_descriptor_count = hex(unpack 'H*', $max_unmap_block_descriptor_count);
	$optimal_unmap_granularity = hex(unpack 'H*', $optimal_unmap_granularity);
	$unmap_granularity_alignment = hex(unpack 'H*', $unmap_granularity_alignment) & 0x7FFFFFFF;
	$max_write_same_length = hex(unpack 'H*', $max_write_same_length);
	
	if (!($max_xfer_length || $max_unmap_lba_count || $max_unmap_block_descriptor_count || $optimal_unmap_granularity ||
	      $unmap_granularity_alignment || $max_write_same_length) && $disk)
	{
		&error_msg("invalid vpd", $disk);
		$disk->{status}->{vpd_b0} = 2;
	}
	
	
	$vpd_b0 .= "max transfer length: ".sprintf("0x%x", $max_xfer_length)."\n";
	$vpd_b0 .= "max_unmap_lba_count: ".sprintf("0x%x", $max_unmap_lba_count)."\n";
	$vpd_b0 .= "max_unmap_block_descriptor_count: ".sprintf("0x%x", $max_unmap_block_descriptor_count)."\n";
	$vpd_b0 .= "optimal_unmap_granularity: ".sprintf("0x%x", $optimal_unmap_granularity)."\n";
	$vpd_b0 .= "unmap_granularity_alignment: ".sprintf("0x%x", $unmap_granularity_alignment)."\n";
	$vpd_b0 .= "max_write_same_length: ".sprintf("0x%x", $max_write_same_length)."\n";
	
	close(INFILE);
	print_output($vpd_b0);
	return $vpd_b0;

}

sub get_vpd_page_b2 {
	my $disk = shift;
	my $cdbfile = "VPD_B2_CDB.BIN";
	my $cmd = $passthru_cmd .$disk->{location}." --cdb_file ".$cdbfile;
	my $output = `$cmd`;	
	if ($output =~ /check/i) {
		error_msg("vpd b2 fail", $disk);
		$disk->{status}->{vpd_b2} = 1;
	} else {
		parse_vpd_page_b2($CDB_OUT, $disk);
	}
}

sub parse_vpd_page_b2
{
	my $file = shift or die "Usage: $0 <bin_file> $!\n";
	my $disk = shift;
	
	print_output("\nVPD B2\n");
	print_output("-------------------------------------\n");
	
	open(INFILE, "<", $file);
	binmode(INFILE);
	read(INFILE, my $data, 5); # throw away the header and other junk
	read(INFILE, my $support, 1); # byte 5 indicates unmap support
	
	$support = hex(unpack 'H*', $support);
	
	my $vpd_b2 = "support: ".sprintf("0x%x", $support)."\n";
	$vpd_b2 .= "unmap: " . ($support & 0x80)."\n";
	$vpd_b2 .= "unmap ws 10: " . ($support & 0x20)."\n";
	$vpd_b2 .= "unmap ws 16: " . ($support & 0x40)."\n";
	
	$disk->{vpd_b2} = sprintf("0x%x", $support) || 0;
	
	close(INFILE);
	print_output($vpd_b2);
	return $vpd_b2;
}

sub send_write_same_with_unmap
{
	my $disk = shift;
	my $redo = shift || 0;
	
	my $cdbfile = "WS_UNMAP_CDB.BIN";
	my $data_out_file = "WS_DATA_OUT.BIN";
	my $write_same_pba = 0x20000;
	my $blocks_to_verify = 0x10;
	
	my $cmd = $passthru_cmd .$disk->{location}." --cdb_file ".$cdbfile." --data_out_file $data_out_file";
	my $output = `$cmd`;	
	
	if ($redo)
	{
		print_output("Write Same with UNMAP");
	} else{
		print_output(" Write Same with UNMAP OF UNMAPPED");
	}
	print_output("-------------------------------------\n");
	
	if ($output =~ /check/i) {
		error_msg("write same with unmap fail", $disk);
		if ($redo)
		{
			$disk->{status}->{ws_unmap_unmapped} = 1;
		} else {
			$disk->{status}->{ws_unmap} = 1;
		}
	} else {
	
		print_output("+++ send of write same with unmap succeeded\n");
		verify_write_same_with_unmap($disk, $write_same_pba, $blocks_to_verify, $redo);
	}
	
}

sub verify_write_same_with_unmap
{
	my $disk = shift;
	my $pba = shift;
	my $blocks = shift;
	my $redo = shift || 0;
	
	my $cmd = "ddt r ".$disk->{location}." ".sprintf("%x", $pba)." ".sprintf("%x", $blocks)."\n";
	my @output = run_fbe_cli_cmd($cmd);
	
	my @flash = ();

	foreach my $line (@output)
	{
		if ($line =~ /error|failed/i) {
			if ($redo)
			{
				$disk->{status}->{ws_unmap_unmapped} = 1;
			} else {
				$disk->{status}->{ws_unmap} = 1;
			}
			return &error_msg("verify write same unmap failed", $disk);
		}
		if ($line =~ m/0\:\s+(.*)/i) {
			my @fields = split(/\s+/, $1);
			if ($line =~ /200\:/) {
				if ($fields[0] ne "00000000") {
					print $fields[0]." \n";
					
					if ($redo)
					{
						$disk->{status}->{ws_unmap_unmapped} = 3;
					} else {
						$disk->{status}->{ws_unmap} = 3;
					}
					return &error_msg("verify write same unmap failed metadata", $disk);
				}
			} else {
				for (my $index = 0; $index <scalar(@fields) ; $index++) {
					if ($fields[$index] ne "00000000") {
						print $fields[$index]." -- $index\n";
						
						if ($redo)
						{
							$disk->{status}->{ws_unmap_unmapped} = 2;
						} else {
							$disk->{status}->{ws_unmap} = 2;
						}
						
						return &error_msg("verify write_same unmap failed nonzero", $disk);
					}
				}
			}
		}
	}
	
}

sub send_unmap
{
	my $disk = shift;
	my $redo = shift || 0;
	my $cdbfile = "UNMAP_CDB.BIN";
	my $data_out_file = "UNMAP_DATA_OUT.BIN";
	my $unmap_pba = 0x10000;
	my $blocks_to_verify = 0x10;
	
	my $cmd = $passthru_cmd .$disk->{location}." --cdb_file ".$cdbfile." --data_out_file $data_out_file";
	my $output = `$cmd`;	
	
	if ($redo)
	{
		print_output("UNMAP");
	} else{
		print_output("UNMAP OF UNMAPPED");
	}
	print_output("-------------------------------------\n");
	
	if ($output =~ m/check/is) {
		error_msg("unmap fail", $disk);
		if ($redo)
		{
			$disk->{status}->{unmap_unmapped} = 1;
		} else {
			$disk->{status}->{unmap} = 1;
		}
		
	} else {
	
		print_output("+++ send unmap succeeded\n");
		&verify_unmap($disk, $unmap_pba, $blocks_to_verify, $redo);
	}
	
}

sub verify_unmap
{
	my $disk = shift;
	my $pba = shift;
	my $blocks = shift;
	my $redo = shift || 0;
	
	my $cmd = "ddt r ".$disk->{location}." ".sprintf("%x", $pba)." ".sprintf("%x", $blocks)."\n";
	my @output = run_fbe_cli_cmd($cmd);
	
	my @flash = ();

	foreach my $line (@output)
	{
		if ($line =~ /error|failed/i) {
			
			if ($redo)
			{
				$disk->{status}->{unmap_unmapped} = 1;
			} else {
				$disk->{status}->{unmap} = 1;
			}
			return &error_msg("verify unmap failed", $disk);
			
		}
		if ($line =~ m/0\:\s+(.*)/i) {
			my @fields = split(/\s+/, $1);
			for (my $index = 0; $index <scalar(@fields) ; $index++) {
				if ($fields[$index] ne "00000000") {
					print $fields[$index]." -- $index\n";
					if ($redo)
					{
						$disk->{status}->{unmap_unmapped} = 2;
					} else {
						$disk->{status}->{unmap} = 2;
					}
					return &error_msg("verify unmap failed nonzero", $disk);
				}
			}
		}
	}
	
}

sub error_msg
{	
	my $msg = shift;
	my $disk = shift || undef;
	
	$msg = uc($msg);
	print_output("ERROR: $msg\n");
	
	push @{$disk->{failures}}, $msg;
	
	return;
}

sub write_background_pattern
{
	
	my $array_ref = shift;
	
	print_output("writing background pattern to drives\n");
	my $base_cmd = "rdgen -object_id 0x%x -package_id sep -constant -seq -start_lba 0 -max_lba 40000 w 0 1 80";
	foreach my $disk (@{$array_ref}) 
	{
		my $cmd = sprintf($base_cmd, hex($disk->{pvd_object_id}));
		run_fbe_cli_cmd($cmd);

	}
	sleep(30);
	
	run_fbe_cli_cmd("rdgen -term");
	
}

sub is_unmap_supported
{
	my $disk = shift;
	#return 1;
	return !$disk->{status}->{rdgen_unmap}; #!($disk->{status}->{unmap} || $disk->{status}->{ws_unmap});
}


sub precondition_drives
{
	my $array_ref = shift;
	
	print_output("precondition drives with random 8k writes\n");
	
	my $base_cmd = "rdgen -object_id 0x%x -package_id sep -constant -align -min_lba 0 max_lba 400000 w 0 1 10";
	foreach my $disk (@{$array_ref}) 
	{
		my $cmd = sprintf($base_cmd, hex($disk->{pvd_object_id}));
		run_fbe_cli_cmd($cmd);

	}
	
	# run io for 4 minutes
	sleep(4*60);
	
	run_fbe_cli_cmd("rdgen -term");
	sleep(4);
}

sub get_test_dir
{
	my $dir = strftime("%Y_%m_%d_%H_%M_%S", localtime);
	return create_dir(getcwd(), $dir);

}

sub create_file
{
	my $dir = shift;
	my $file = shift;
	return File::Spec->catfile($dir, $file);
}

sub create_dir
{
	my $parent = shift;
	my $dir = shift;
	my $new_dir = File::Spec->catdir($parent, $dir);
	mkdir $new_dir;
	return $new_dir;
	
}

sub run_command
{
	my $cmd = shift;
	print_output("Run Command: $cmd");
	system($cmd);
}

sub start_neit
{
	run_command("net start newneitpackage");
}

sub get_rba_file_name
{
	my $test = shift;
	
	return create_file($test_log_dir, "$test.ktr");

}

sub start_rba
{
        my $test = shift;
	my $rba_file = &get_rba_file_name($test);
	my $start_cmd = "rba.exe -o $rba_file -t pdo -t pvd";
	run_command($start_cmd);
}

sub stop_rba
{
	my $stop_rba_command = "rba.exe -c -t off";
	run_command($stop_rba_command);
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
	            "rdgen -term",
		    "rdgen -reset_stats");
	run_fbe_cli_cmds(\@cmds);

}

sub disable_background_ops
{
	run_fbe_cli_cmd("bgod -all");
}

sub enable_background_ops
{
	run_fbe_cli_cmd("bgoe -all");
}

sub startup
{
	stop_rdgen();
	disable_background_ops();
	start_neit();
	init_rdgen();
}

sub cleanup
{
        stop_rdgen();
	enable_background_ops();
	
	close($LOG);
}

sub verify_rdgen_unmap
{
	my @disks =  @_;
	my @cmds = ();
	
	foreach my $disk (@disks) 
	{
		my $cmd = "rdgen -object_id ".$disk->{pvd_object_id}." -package_id sep -perf -seq -constant -align -start_lba 0 u 0 1 80";
		push @cmds, $cmd;
	}
	
	my @cmd_out = run_fbe_cli_cmds(\@cmds);
	my @output = run_fbe_cli_cmd("rdgen -d");
	my @pvd_ids = ();
	
	foreach my $line (@output) {
		#print "line : $line\n";
				if ($line =~ /object_id\: (\d+)\s/)
				{
					my $pvd_id = sprintf("0x%x", $1);
					print_output(" rdgen successful for $pvd_id\n");
					
					push @pvd_ids, $pvd_id;
				} 
		
	}
	
	foreach my $disk (@disks)
	{
		if (grep {$_ eq $disk->{pvd_object_id}} @pvd_ids)
		{
			print_output(" rdgen successful for ".$disk->{location}."\n");
			$disk->{status}->{rdgen_unmap} = 0;
		}
		else
		{
			print_output(" rdgen unsuccessful for ".$disk->{location}."\n");
			$disk->{status}->{rdgen_unmap} = 1;	
		}	
	}
	
}

sub run_unmap_benchmark
{
	my @disks = @_;
	my @num_threads = (1,2,4,8,10);
	my @data_sizes = (40,80,100,400,800,1000,2000,4000);
	
	my $test_id = 1;
	
	foreach my $data_size (@data_sizes)
	{
		foreach my $num_thread (@num_threads)
		{
		
			precondition_drives(\@disks);
			
			my @cmds = ();
			my $test_name = "ws_unmap_XFER_".$data_size."_QD_".$num_thread;
			my $rdgen_log_in = create_file($test_log_dir, "rdgen_".$test_name."_in.txt");
			my $rdgen_log_out = create_file($test_log_dir, "rdgen_".$test_name."_out.txt");
			
			print_output("Running test $test_name\n");
			
			open(my $rdgen_in, ">$rdgen_log_in");
			open(my $rdgen_out, ">$rdgen_log_out");
			foreach my $disk (@disks) 
			{
				next if (!is_unmap_supported($disk));
				my $cmd = "rdgen -object_id ".$disk->{pvd_object_id}." -package_id sep -perf -seq -constant -align -start_lba 0 u 0 $num_thread $data_size";
				push @cmds, $cmd;
				#print $rdgen_in $cmd."\n";
			
			}
			
			my @cmd_out = run_fbe_cli_cmds(\@cmds, $rdgen_in);
			
			sleep(2);
			start_rba($test_name);
			
			# let rdgen run for awhile
			sleep(30);
			
			my @output = run_fbe_cli_cmd("rdgen -d");
			my %result =  ();
			my %rate = ();
			my %avg_resp_time = ();
			my %max_resp_time = ();
			my $pvd_id = 0;
			foreach my $line (@output) {
				#print "line : $line\n";
				if ($line =~ /object_id\: (\d+)\s/)
				{
					$pvd_id = sprintf("0x%x", $1);
					print_output(" getting results for $pvd_id\n");
					$result{$pvd_id}->{rate} = "0.0";
					$result{$pvd_id}->{avg_resp_time} = "0.0";
					$result{$pvd_id}->{max_resp_time} = "0.0";
				} 
				elsif ($line =~ /rate\: (\d+\.\d+) avg resp tme\: (\d+\.\d+) max resp tme\: (\d+\.\d+)/ig)
				{
					$result{$pvd_id}->{rate} = $1;
					$result{$pvd_id}->{avg_resp_time} = $2;
					$result{$pvd_id}->{max_resp_time} = $3;
					
				}
			
			}
			
			foreach my $disk (@disks) {

				if (!is_unmap_supported($disk)) {
					$disk->{unmap_tests}{$test_id}->{test_name} = $test_name;
					$disk->{unmap_tests}{$test_id}->{rate} = "0.0";
					$disk->{unmap_tests}{$test_id}->{avg_resp_time} = "0.0";
					$disk->{unmap_tests}{$test_id}->{max_resp_time} = "0.0";
				} else {
					$disk->{unmap_tests}{$test_id} = $result{$disk->{pvd_object_id}};
					$disk->{unmap_tests}{$test_id}->{test_name} = $test_name;
					
					print_output($disk->{location}." rate: ".$disk->{unmap_tests}{$test_id}->{rate}."\n");
				}
				
				
			}
			
			print $rdgen_out join("\n", @cmd_out)."\n".join("\n", @output);
			
			
			stop_rba();
			stop_rdgen();
			
			close($rdgen_in);
			close($rdgen_out);
			
			sleep(2);
			$test_id++;
		}
	}
	
	print_test_results_by_type(@disks);
}

sub print_test_results_by_type
{
	my @disks = @_;
	
	my @test_ids = keys %{$disks[0]->{unmap_tests}};
	
	foreach my $type ("rate", "avg_resp_time", "max_resp_time") {
		my $file = create_file($test_dir, "$type.csv");
		open(FILE, ">$file");
		print FILE "location,tla";
		foreach my $test_id (sort {$a <=> $b} @test_ids) {
			print FILE ",".$disks[0]->{unmap_tests}{$test_id}->{test_name};
		}
		print FILE "\n";
		
		foreach my $disk (sort {$a->{tla} cmp $b->{tla}} @disks) {
			next if (!is_unmap_supported($disk));
			print FILE $disk->{location}.",".$disk->{tla}.",";
			foreach my $test_id (sort {$a <=> $b} @test_ids) {
				print FILE $disk->{unmap_tests}{$test_id}->{$type}.",";
				
			}
			print FILE "\n";
		}
		
		close(FILE);
	}

}

# print the usage of the script
sub print_usage {
	print "\nUSAGE: \n\n";
	print "ssd_test.pl -d <b_e_s> -d <b_e_s> ...\n";
	print "ssd_test.pl -d all\n";
	
	print "\n";
	exit(1);
}		 
			 			 
			 
1;
			 
