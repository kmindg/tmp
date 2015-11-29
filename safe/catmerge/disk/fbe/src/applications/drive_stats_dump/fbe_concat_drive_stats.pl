#!/usr/bin/perl

# *********************************************************************
#  Copyright (C) EMC Corporation 2015
#  All rights reserved.
#  Licensed material -- property of EMC Corporation
# *********************************************************************
#
# *********************************************************************
#  FILE: fbe_concat_drive_stats.pl
# *********************************************************************
# 
#  DESCRIPTION:
#         This script will append the FastCache over-provisioning information
#         to the existing binary drive stats file (drive_stats_most_recent.bin).
#         A header, with a specific data variant ID is first written.  This
#         is then followed by the FctCli output.
# 

use strict;
use warnings;

# This script does the following
# 1. Verify the binary file exists
# 2. Get cli output
# 3. Open the binary file for append.
# 4. Append header
# 5. Append Data, which includes output len, followed by the string output.

my $file = "/opt/safe/c/DUMPS/drive_stats_most_recent.bin";

my $cli_output = `FctCli.exe -c 5`;

if (! -f $file)
{
    print "$file not found\n";
    exit 1;
}

open(my $fh, ">>", $file) or die "Failed to open file $file: $!";
binmode($fh);

# Add header.  
#    bytes 0..3  ID = "FCOP"
#    bytes 4..5  Len of Data.
#    bytes 6..11 collection timestamp 
#    bytes 12    Data starts

print $fh "FCOP";   # Data Variant ID

my $len = length($cli_output);
print $fh pack('S>',$len);   # 2 bytes big-endian

# note, timestamp will contain year as num years since 1900, and month will
# be index from 0..11.  The drive stats dump parser convert this correctly.
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =localtime;
print $fh pack('c',$year);
print $fh pack('c',$mon);
print $fh pack('c',$mday);
print $fh pack('c',$hour);
print $fh pack('c',$min);
print $fh pack('c',$sec);

# Data
print $fh $cli_output;

close($fh);

