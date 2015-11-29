#!/usr/bin/perl
use strict;
use Data::Dumper;
use XML::Simple;

sub KILO                        () { (1024 ** 1) }
sub MEGA                        () { (1024 ** 2) }
sub GIGA                        () { (1024 ** 3) }
sub TERA                        () { (1024 ** 4) }
sub PETA                        () { (1024 ** 5) }


$ENV{PATH} .= ":.";
$ENV{LD_LIBRARY_PATH} .= ":.";

my $xml         = `fbe_psl_info.exe $ARGV[0]`;
$xml =~ s/^[^\<]*//;

if(!$ARGV[0]) {
   print $xml;
   exit(1);
}

my $ref         = XMLin($xml);

my %starting_lbas;
foreach my $region (@{$ref->{region}}) {
        my $addr = hex($region->{starting_block_address});
        my $end = hex($region->{starting_block_address}) + hex($region->{size_in_blocks});
        $starting_lbas{$addr} = 1;
        $starting_lbas{$end} = 1;
}


my @sou;
foreach my $fru (0..15) {
    $sou[$fru] = hex($ref->{start_of_userspace}->{fru}->{$fru}->{content});
        $starting_lbas{$sou[$fru]} = 1;
}

my @fru_regions;
my @fru_luns;
foreach my $fru (0 .. 4) {
        push(@{$fru_regions[$fru]}, "   FRU $fru");
        foreach my $saddr (sort {$a <=> $b} keys(%starting_lbas)) {
                my $region_start_match = 0;
                my $region_end_match = 0;
                REGION:
                foreach my $region (@{$ref->{region}}) {
                        if(! ref $region->{fru_id}) {
                                $region->{fru_id} = [$region->{fru_id}];
                        }
                        if(($region->{number_of_frus} ne 'ALL') && (! grep { $_ == $fru } @{$region->{fru_id}}) ) {
                                next REGION;
                        }
                        my $addr = hex $region->{starting_block_address};
                        my $eaddr = hex($region->{starting_block_address}) + hex($region->{size_in_blocks});

                        if($addr == $saddr) {
                                $fru_regions[$fru] ||= [];
                my $rg_info;
                if($region->{raid_group_id}) {
                    $rg_info = "RG $region->{raid_group_id}, ";
                }
                                push(@{$fru_regions[$fru]},
                                        "--------------------------",
                                        "$rg_info$region->{region_name}",
                                        "Start " . DisplaySize($saddr, 1),
                                        "Size  " . DisplaySize(hex($region->{size_in_blocks}), 1),
                                        "$region->{region_type}",
                                );
                                $region_start_match++;
                        }
                        elsif($eaddr == $saddr) {
                                $region_end_match++;
                        }
                }
                if((! $region_start_match) && (!($sou[$fru] <= $saddr))) {
                        if($region_end_match) {
                                push(@{$fru_regions[$fru]},
                                        "--------------------------",
                                        "(Unallocated)",
                                        #"Start " . DisplaySize($saddr, 1),
                                        "",
                                        "",
                                        "",
                                );
                        }
                        else {
                                push(@{$fru_regions[$fru]},
                                        "",
                                        "",
                                        "",
                                        "",
                                        "",
                                );
                        }
                }
                elsif($sou[$fru] == $saddr) {
                        push(@{$fru_regions[$fru]},
                                "==========================",
                                "Start of Userspace",
                                DisplaySize($saddr, 1),
                        )
                }
                foreach my $lun (@{$ref->{lun}}) {
                        my $region      = GetRGForLUN($lun);
                        my $data_disks  = GetDataDisksForLUN($lun);
                        my $addr                = hex($region->{starting_block_address}) + (hex($lun->{raid_group_address_offset}) / $data_disks);
                        if($addr == $saddr) {
                                
                        }
                }
        }
}

my @aggregate;
my $counter;
foreach my $fru (0 .. 4) {
        $counter = 0;
        foreach my $string (@{$fru_regions[$fru]}) {
                my $pad = 26 - length($string);
                $string .= ' ' x $pad;
                $aggregate[$counter] .= "|$string";
                $counter++;
        }
}

foreach my $line (@aggregate) {
        print "$line|\n";
}

my %rg_addr_offsets;
foreach my $lun (@{$ref->{lun}}) {
        my $start = hex($lun->{raid_group_address_offset});
        my $end = $start + hex($lun->{internal_capacity});
        $rg_addr_offsets{$lun->{raid_group_id}}{$start} = 1;
        $rg_addr_offsets{$lun->{raid_group_id}}{$end} = 1;
}

RGID:
foreach my $rg_id (sort { $a <=> $b } keys %rg_addr_offsets) {
        print "\n\n";
        print "****************************************\n";
        print "**** RAID Group $rg_id\n";
        print "****************************************\n\n";
        ADDR:
        foreach my $addr (sort { $a <=> $b } keys %{$rg_addr_offsets{$rg_id}}) {
                my $lun_start_match;
                my $lun_end_match;
                LUN:
                foreach my $lun (@{$ref->{lun}}) {
                        my $start = hex($lun->{raid_group_address_offset});
                        my $end = $start + hex($lun->{internal_capacity});
                        
                        if($lun->{raid_group_id} != $rg_id) {
                                next LUN;
                        }
                        if($start == $addr) {
                                $lun_start_match++;
                                my $region      = GetRGForLUN($lun);
                                my $data_disks  = GetDataDisksForLUN($lun);
                                my $raw_addr    = hex($region->{starting_block_address}) + ($start / $data_disks);
                                print "--------------------------------\n";
                                print "LUN $lun->{lun_number}, $lun->{lun_name}\n\n";
                                print "RG Offset         " . DisplaySize($start) . "\n";
                                print "Disk Offset       " . DisplaySize($raw_addr) . "\n";
                                print "Internal Capacity " . DisplaySize(hex($lun->{internal_capacity})) . "\n";
                                print "External Capacity " . DisplaySize(hex($lun->{external_capacity})) . "\n";
                                print "\n";
                        }
                        elsif($end == $addr) {
                                $lun_end_match++;
                        }
                }
                if($lun_end_match && (!$lun_start_match)) {
                        print "--------------------------------\n";
                        print "(Unallocated)\n";
                        print "\n";
                }
        }
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

sub DisplaySize {
        my($size) = @_;

    my $unit    = "Blks";
    my $divisor = 512;
    my $fmt     = "0x%08X";
        
        $size *= $divisor;
        
        if(int($size/TERA) > 0) {
                return sprintf("$fmt(%.2fTB)", $size / $divisor, $size/TERA);
        }
        elsif(int($size/GIGA) > 0) {
                return sprintf("$fmt(%.2fGB)", $size / $divisor, $size/GIGA);
        }
        elsif(int($size/MEGA) > 0) {
                return sprintf("$fmt(%.2fMB)", $size / $divisor, $size/MEGA);
        }
        elsif(int($size/KILO) > 0) {
                return sprintf("$fmt(%.2fKB)", $size / $divisor, $size/KILO);
        }
        else {
                return sprintf("$fmt", $size / $divisor);
        }
}
