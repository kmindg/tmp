#!/usr/bin/perl
use strict;
use Data::Dumper;
use XML::Simple;

sub KILO                        () { (1024 ** 1) }
sub MEGA                        () { (1024 ** 2) }
sub GIGA                        () { (1024 ** 3) }
sub TERA                        () { (1024 ** 4) }
sub PETA                        () { (1024 ** 5) }

my $failed      = 0;
my $chunk_size  = 0x800; #blocks, 1.0 MB
$ENV{PATH} .= ":.";
$ENV{LD_LIBRARY_PATH} .= ":.";

my $xml         = `fbe_psl_info.exe $ARGV[0]`;
$xml =~ s/^[^\<]*//;
if(!$ARGV[0]) {
   print $xml;
   exit(1);
}
my $ref         = XMLin($xml);

#print Dumper($ref);
#exit;

print "\n\n";
print "========================================================================\n";
print "    Checking Regions\n";
print "========================================================================\n";
REGION:
foreach my $region (@{$ref->{region}}) {
    print "\n$region->{region_name}\n";
    CheckAlignment(
        $region, $chunk_size,
        qw(starting_block_address size_in_blocks)
    );

    CheckRegionForOverlaps($region);

    my $size = DisplaySize(hex $region->{size_in_blocks}, 1);

    print "    Capacity $size\n";
}

print "\n\n";
print "========================================================================\n";
print "    Checking LUNs\n";
print "========================================================================\n";
foreach my $lun (@{$ref->{lun}}) {
    print "\n$lun->{lun_number}, $lun->{lun_name}\n";

    my $data_disks = GetDataDisksForLUN($lun);
        
    my $element_size = $data_disks * 0x80;

    CheckAlignment(
        $lun, $element_size,
        qw(raid_group_address_offset internal_capacity)
    );

    CheckAlignment(
        $lun, $chunk_size,
        qw(external_capacity)
    );

    CheckAlignment(
        $lun, $chunk_size * $data_disks,
        qw(raid_group_address_offset internal_capacity)
    );

    CheckLunForOverlaps($lun);

    my $region      = GetRGForLUN($lun);
    my $disk_offset = hex($region->{starting_block_address}) + (hex($lun->{raid_group_address_offset}) / $data_disks);
    print  "    Data Disks           $data_disks\n";
    printf("    Disk Starting Offset 0x%08X Blocks\n", $disk_offset);
    printf("    RG Starting Offset   0x%08X Blocks\n\n", hex($lun->{raid_group_address_offset}));

    my $int_size = DisplaySize(hex $lun->{internal_capacity}, 1);
    my $ext_size = DisplaySize(hex $lun->{external_capacity}, 1);
    my $object_size = DisplaySize(hex($lun->{internal_capacity}) - hex($lun->{external_capacity}), 1);
    print "    Internal Capacity $int_size\n";
    print "    External Capacity $ext_size\n";
    print "    Object Size       $object_size\n";

    if($data_disks > 1) {
        print "\n";
        my $int_size_pf = DisplaySize(hex($lun->{internal_capacity}) / $data_disks, 1);
        my $ext_size_pf = DisplaySize(hex($lun->{external_capacity}) / $data_disks, 1);
        my $object_size_pf = DisplaySize((hex($lun->{internal_capacity}) - hex($lun->{external_capacity})) / $data_disks, 1);
        print "    Internal Capacity Per FRU $int_size_pf\n";
        print "    External Capacity Per FRU $ext_size_pf\n";
        print "    Object Size Per FRU       $object_size_pf\n";
    }
}

print "\n\n";
print "========================================================================\n";
print "    Checking Start of Userspace\n";
print "========================================================================\n";
my @sou;
foreach my $fru (0..15) {
    $sou[$fru] = hex($ref->{start_of_userspace}->{fru}->{$fru}->{content});
}
#print Dumper(\@sou);
foreach my $fru (0..3) {
    if($sou[$fru] != $sou[0]) {
        print "ERROR: Start of Userspace on FRU $fru does not match FRU 0!\n";
        $failed++;
    }
}
foreach my $fru (4..15) {
    if($sou[$fru] != $sou[4]) {
        print "ERROR: Start of Userspace on FRU $fru does not match FRU 4!\n";
        $failed++;
    }
}

print "\n\n";
if($failed) {
    print "$failed issue(s) encountered during validation!\n";
    exit(1);
}
else {
    print "Validation of the Private Space Layout succeeded!\n";
}

#############################################################################################
#
# Subroutines
#
#############################################################################################

sub GetRGForLUN {
    my($lun)    = @_;

    foreach my $region (@{$ref->{region}}) {
        if(($region->{region_type} eq 'RAID_GROUP') && ($region->{raid_group_id} == $lun->{raid_group_id})) {
            return($region);
        }
    }
    print Dumper($lun);
    die("Couldn't find a matching RAID Group");
}

sub GetDataDisksForLUN {
    my($lun)    = @_;

    my $region  = GetRGForLUN($lun);

    if($region->{raid_type} eq "RAID 1") {
        return(1);
    }
    elsif($region->{raid_type} eq "RAID 1 RAW") {
        return(1);
    }
    elsif($region->{raid_type} eq "RAID 3") {
        return($region->{number_of_frus} - 1);
    }
    elsif($region->{raid_type} eq "RAID 5") {
        return($region->{number_of_frus} - 1);
    }
    else  {
        die("Unhandled RAID Type: $region->{raid_type}");
    }

}

sub CheckAlignment {
    my($struct, $align_to, @fields) = @_;
    
    #print "Checking $name for proper alignment\n";
    
    foreach my $field (@fields) {
        my $value = hex($struct->{$field});
        
        if(($value % $align_to) != 0) {
            print "    ERROR: $field is not properly aligned to $align_to blocks!\n";
            $failed++;
        }
    }
}

sub CheckLunForOverlaps {
    my($lun) = @_;

    CANDIDATE:
    foreach my $candidate (@{$ref->{lun}}) {
        if($lun->{lun_number} == $candidate->{lun_number}) {
            next CANDIDATE;
        }
        if($lun->{raid_group_id} != $candidate->{raid_group_id}) {
            #print "    Skipping candidate in a different RG: $candidate->{lun_name}\n";
            next CANDIDATE;
        }

        my $lun_start    = hex($lun->{raid_group_address_offset});
        my $lun_end      = hex($lun->{raid_group_address_offset}) + hex($lun->{internal_capacity}) - 1;

        my $candidate_start    = hex($candidate->{raid_group_address_offset});
        my $candidate_end      = hex($candidate->{raid_group_address_offset}) + hex($candidate->{internal_capacity}) - 1;

        if(
           (($candidate_start >= $lun_start) && ($candidate_start <= $lun_end))
           ||
           (($candidate_end >= $lun_start) && ($candidate_end <= $lun_end))
           ||
           (($lun_start >= $candidate_start) && ($lun_start <= $candidate_end))
           ||
           (($lun_end >= $candidate_start) && ($lun_end <= $candidate_end))
        ) {
            print "    ERROR: Overlaps with $candidate->{lun_name}\n";
            $failed++;
        }
    }
}

sub CheckRegionForOverlaps {
    my($region) = @_;

    CANDIDATE:
    foreach my $candidate (@{$ref->{region}}) {
        if(! ref $candidate->{fru_id}) {
            $candidate->{fru_id} = [$candidate->{fru_id}];
        }
        if(! ref $region->{fru_id}) {
            $region->{fru_id} = [$region->{fru_id}];
        }
        if($region->{region_id} == $candidate->{region_id}) {
            next CANDIDATE;
        }
        my $frus_in_common;
        if(($region->{number_of_frus} eq 'ALL') || ($candidate->{number_of_frus} eq 'ALL')) {
            $frus_in_common = 1;
        }
        else  {
            foreach my $fru (@{$region->{fru_id}}) {
                my $match = grep {$_ == $fru} @{$candidate->{fru_id}};
                if($match) {
                    $frus_in_common++;
                }
            }
        }
        if(! $frus_in_common) {
            #print "    Skipping candidate with no FRUs in common: $candidate->{region_name}\n";
            next CANDIDATE;
        }

        my $region_start    = hex($region->{starting_block_address});
        my $region_end      = hex($region->{starting_block_address}) + hex($region->{size_in_blocks}) - 1;

        my $candidate_start    = hex($candidate->{starting_block_address});
        my $candidate_end      = hex($candidate->{starting_block_address}) + hex($candidate->{size_in_blocks}) - 1;

        if(
           (($candidate_start >= $region_start) && ($candidate_start <= $region_end))
           ||
           (($candidate_end >= $region_start) && ($candidate_end <= $region_end))
           ||
           (($region_start >= $candidate_start) && ($region_start <= $candidate_end))
           ||
           (($region_end >= $candidate_start) && ($region_end <= $candidate_end))
        ) {
            print "    ERROR: Overlaps with $candidate->{region_name}\n";
            $failed++;
        }
    }
}

sub DisplaySize {
        my($size, $blocks) = @_;

    my $unit = "B";
    my $divisor = 1;
    my $fmt = "%d";
    if($blocks) {
        $divisor = 512;
        $size = $size * $divisor;
        $unit = "Blocks";
        $fmt = "0x%08X"
    }
        
        if(int($size/TERA) > 0) {
                return sprintf("%.2f TB ($fmt %s)", $size/TERA, $size / $divisor, $unit);
        }
        elsif(int($size/GIGA) > 0) {
                return sprintf("%.2f GB ($fmt %s)", $size/GIGA, $size / $divisor, $unit);
        }
        elsif(int($size/MEGA) > 0) {
                return sprintf("%.2f MB ($fmt %s)", $size/MEGA, $size / $divisor, $unit);
        }
        elsif(int($size/KILO) > 0) {
                return sprintf("%.2f KB ($fmt %s)", $size/KILO, $size / $divisor, $unit);
        }
        else {
                return sprintf("$fmt %s", $size / $divisor, $unit);
        }
}
