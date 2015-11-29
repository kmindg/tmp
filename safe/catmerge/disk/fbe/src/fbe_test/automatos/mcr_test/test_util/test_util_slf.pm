package mcr_test::test_util::test_util_slf;

use warnings;
use strict;
#use base qw(Automatos::Test::Case);
#use Automatos::Test::Case;

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
use mcr_test::platform_shim::platform_shim_host;
use mcr_test::platform_shim::platform_shim_enclosure;
use mcr_test::test_util::test_util_io;
use mcr_test::test_util::test_util_host_io;
use mcr_test::test_util::test_util_general;

use Automatos::ParamValidate qw(validate validate_with :callbacks :types);


# =begin nd
#
# Method: test_util_slf_get_disk_enclosure 
# Get enclosure object <Automatos::Component::Enclosure::Emc::Vnx> from disk id.
# 
# Input:
# A hash specifying the following key/value pairs
# 
# disk => $ - Disk <Automatos::Component::Disk::Emc::Vnx> object.
# 
# Returns:
# %encl_params - % hash includes following keys
# 
# id => $ - Enclosure id
# obj => $ - <Automatos::Component::Enclosure::Emc::Vnx> object
# 
# Exceptions:
# *None*
# 

sub test_util_slf_get_disk_enclosure  {

    my ($tc) = shift @_;
    my %params = @_;
	
    my $encl_id = $tc->test_util_slf_parse_disk_name(disk => $params{disk},
                                      what => 'BE');
	my %hash_encl = (type => 'enclosure', criteria => {id => $encl_id});
    my ($encl_obj) = platform_shim_enclosure_find($tc->{device}, \%hash_encl);

    my %encl_params = (id => $encl_id, obj => $encl_obj);

    return (%encl_params);
}

# =begin nd
# 
# Method: test_util_slf_induce_slf
# Induce single loop failure fault in specified Enclosure
# 
# Input:
# A hash specifying the following key/value pairs
# 
# disk => $ - Disk <Automatos::Component::Disk::Emc::Vnx> object.
# sp   => $ - Sp <Automatos::Component::Sp::Emc::Vnx> object.
# 
# Returns:
# %encl_params - % hash includes following keys
# 
# id => $ - Enclosure id
# obj => $ - <Automatos::Component::Enclosure::Emc::Vnx> object
# fault => $ - <Automatos::Fault> Object
# 
# Exceptions:
# - <Automatos::Exception::Base>
#

sub test_util_slf_induce_enclosure_slf {

    my ($tc) = shift @_;

    my %params = @_;

    # get enclosure obj for the given disk
    my %encl_params = $tc->test_util_slf_get_disk_enclosure (disk => $params{disk});

    $tc->platform_shim_test_case_log_info('Inducing Single loop failure on '.
                    uc(platform_shim_sp_get_property($params{sp},'name')). ' side '.
                    'of enclosure '.$encl_params{id});

    my %hash_encl = (sp => $params{sp});
    $encl_params{fault} = platform_shim_enclosure_induce_slf_failure($encl_params{obj},\%hash_encl);

    # wait for 5 seconds before checking enclosure stats
    sleep 5;

    test_util_general_set_target_sp($tc,$params{sp});
    my %encl_stats = platform_shim_enclosure_get_enclosure_summary($tc->{device});

    if (!$encl_stats{$encl_params{id}}) {
        $tc->platform_shim_test_case_log_info('Single loop failure has been induced');
    }
    else {
        platform_shim_test_case_throw_exception('Failed to induce Single Loop Failure')
    }

    return %encl_params;
} 

# 
# 
# Method: restore_slf
# Restore single loop failure fault which was inserted on enclosure
# 
# Input:
# A hash specifying the following key/value pairs
# 
# id => $ - Enclosure id
# fault => $ - <Automatos::Fault> Object
# 
# Returns:
# *None*
# 
# Exceptions:
# - <Automatos::Exception::Base> - Failed to restore Single Loop Failure
# 

sub test_util_slf_restore_enclosure_slf {

    my ($tc) = shift @_;

    my %encl_params = @_;

    platform_shim_device_recover_fault($encl_params{fault});
	
    # wait for 5 seconds before checking enclosure stats
    sleep 5;
    my %encl_stats = platform_shim_enclosure_get_enclosure_summary($tc->{device});

    if ($encl_stats{$encl_params{id}}) {
        $tc->platform_shim_test_case_log_info('Single loop failure has been restored on encl '.$encl_params{id});
    }
    else {
        platform_shim_test_case_throw_exception('Failed to restore Single Loop Failure on encl '.$encl_params{id});
    }
}


sub test_util_slf_wait_for_slf
{
    my ($tc, $disks_ref) = @_;
    my $device = $tc->{device};
    my $timeout = '300 S';    # framework checks one disk at a time.   If there a a lot of disks this can take a while :(
    
    #my $is_slf = platform_shim_disk_get_property($disk, 'is_single_loop_failure');
    platform_shim_device_wait_for_property_value($device, 
                                                 {objects => $disks_ref, 
                                                  property => 'is_single_loop_failure', 
                                                  value => 1,
                                                  timeout => $timeout});
           
}

sub test_util_slf_wait_for_slf_cleared
{
    my ($tc, $disks_ref) = @_;
    my $device = $tc->{device};
    my $timeout = '60 S';
    
    #my $is_slf = platform_shim_disk_get_property($disk, 'is_single_loop_failure');
    platform_shim_device_wait_for_property_value($device, 
                                                 {objects => $disks_ref, 
                                                  property => 'is_single_loop_failure', 
                                                  value => 0,
                                                  timeout => $timeout});
           
}

sub test_util_slf_induce_disk_slf
{
    my ($tc, $disks_ref, $slf_sp) = @_;
    
    my @disks_to_fault = @{$disks_ref};
    test_util_general_set_target_sp($tc, $slf_sp);
    
    my @disk_faults = ();
    foreach my $disk_to_fault (@disks_to_fault)
    {
        my $disk_to_fault_id = platform_shim_disk_get_property($disk_to_fault, 'id');
        my $sp_name = platform_shim_sp_get_property($slf_sp, 'name');
        $tc->platform_shim_test_case_log_info("Start SLF on $disk_to_fault_id");
        push @disk_faults, platform_shim_disk_remove_on_sp($disk_to_fault, $slf_sp);
    }

    test_util_slf_wait_for_slf($tc, \@disks_to_fault);
    
    return \@disk_faults;
}



1;
