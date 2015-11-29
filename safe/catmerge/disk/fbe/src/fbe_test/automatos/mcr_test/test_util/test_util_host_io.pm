package mcr_test::test_util::test_util_host_io;

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
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_test_case;
use mcr_test::platform_shim::platform_shim_host;
use mcr_test::platform_shim::platform_shim_enclosure;

use Automatos::ParamValidate qw(validate validate_with :callbacks :types);
our $iox_timeout;
our $perfstats_timeout;



# Method: test_util_slf_set_timeout
# Set timeout period for global use in current package for 'perfstats' command and IOX
# 
# Input:
# A hash specifying the following key/value pairs
# 
# $perfstats_timeout => $ - Integer specifying amount of time in seconds to wait for read and
#                            write counter to increase through FbeCli perfstats command.
# $iox_timeout => $ - Integer specifying amount of time in seconds to wait till
#                     there is increment in IOX total block count
# 
# Returns:
# *none*
# 
# Exceptions:
# - <Automatos::Exception::Base>
#

sub test_util_slf_set_timeout {
    my ($tc) = shift(@_);

    #Get the Parameter names and values
    my %params = platform_shim_test_case_get_parameter($tc);
    $perfstats_timeout = 3600;
    $iox_timeout = 3600;
}







# Method: test_util_io_get_io_params
# Gets IO params.
# 
# Input:
# *None*
# 
# Returns:
# A hash specifying the following key/value pairs
# 
# luns => \@ - ArrayRef to <Automatos::Component::Lun::Emc::Vnx> objects
# sp => $ - <Automatos::Device::Host::Sp::Vnx> Object to force IO to go to.
# 
# Exceptions:
# *None*
#

sub test_util_io_get_io_params
{
    my $tc = shift;

    # get the RAID group
    my ($rg_obj) = platform_shim_raid_group_get_raidgroup($tc->{device});

    # get the LUN
    my ($lun_obj) = platform_shim_raid_group_get_lun($rg_obj);

    # get the SP owner of LUN
    my $owning_sp = platform_shim_lun_get_property($lun_obj, 'current_owner');

    if ($owning_sp =~ /A/i) {
        $owning_sp = platform_shim_host_get_host_object($tc->{spa});
    }
    else {
        $owning_sp = platform_shim_host_get_host_object($tc->{spb});
    }

    my %io_params = (sp => $owning_sp,
                    luns => [$lun_obj]);

    return (%io_params);
}







# Method: test_util_io_start_io
# Start IO on specified lun/s with specified IO options
# 
# Input:
# A hash specifying the following key/value pairs
# 
# luns => \@ - ArrayRef to <Automatos::Component::Lun::Emc::Vnx> objects
# sp => $ - <Automatos::Device::Host::Sp::Vnx> Object to force IO to go to.
# 
# Returns:
# $iox_session - $ - <Automatos::Io::Block::Iox> Object
# 
# Exceptions:
# *None*
#

sub test_util_io_start_io
{
    my $tc = shift;

    my %params = @_;

    my %io_params = (io_mode => 'WC',
                    io_size => 256,
                    io_pattern => 'SAD',
                    transfer_count => 0,
                    threads => 4,
                    pass_count => 'C',
                    seek_type => 'S',
                    panic_sps => 1,
                    sp => $params{sp},
                    luns => $params{luns});

    # create IOX session
    $tc->platform_shim_test_case_log_info("Starting IO to lun(s)");
    my $iox_session = platform_shim_host_iox_block_session($tc->{host});
	platform_shim_host_iox_init($iox_session);

    # start IOX
    platform_shim_host_iox_start($iox_session, \%io_params);

    return $iox_session;

}


#
# Method: test_util_io_check_io
#   Check IO errors.
# 
# Input:
# A hash specifying the following key/value pairs
# 
# luns => \@ - ArrayRef to <Automatos::Component::Lun::Emc::Vnx> objects
# iox_session => $ - <Automatos::Io::Block::Iox> Object
# 
# Returns:
# *None*
# 
# Exceptions:
# - <Automatos::Exception::Base> - Errors Found in IO
#

sub test_util_io_check_io
{

    my $tc = shift;

    my %params = @_;

    my @lun_objs = @{$params{luns}};
    my $iox_session_obj = $params{iox_session};

    my $count = 1;
    my %blocks_count = ();
    my $io_increment = 0;
    my $err_str = '';
    my $num_of_luns = scalar @lun_objs;
    my $wait_until = time() + $iox_timeout;
    $tc->platform_shim_test_case_log_info('Checking IO status. Timeout is '. $iox_timeout . ' seconds' );

    while(time() < $wait_until ) {
        sleep 1;
        # Get IOX status object. getStatus method checks for DMC
        my $status_obj = platform_shim_host_iox_getstatus($iox_session_obj);
        my $log_str = "\t\t" . 'Count '. $count . "\n";
        $err_str = "\t\tIO seems to be stopped on ";
        
        foreach my $lun (@lun_objs) {
            my $lun_name = platform_shim_lun_get_property($lun, 'name');
            my %hash_lunstatus = (lun => $lun, start_offset => 0);
            my %lun_status = platform_shim_lun_get_lun_status($status_obj, \%hash_lunstatus);

            if ($count == 1) {
                $blocks_count{$lun_name}{'previous_total_blocks'} = $lun_status{total_blocks};
            } elsif ($blocks_count{$lun_name}{'previous_total_blocks'} < $lun_status{total_blocks}) {
               $io_increment++;
            } else {
                $err_str .= " $lun_name";
            }

            $log_str .= "\t" x 7 .$lun_name . " => " .$lun_status{total_blocks}. "\n";

            $blocks_count{$lun_name}{'previous_total_blocks'} = $lun_status{total_blocks};

            if (($lun_status{active_threads} == 0) ||
                ($lun_status{err_hard} > 0 ) || ($lun_status{err_soft} > 0)) {
                    my $msg = "LUN $lun_name :: IO Errors detected!\n";
                    if ($lun_status{err_hard}) {
                        $msg .= 'Hard Errors : '. $lun_status{err_hard} ."\n";
                    }
                    if ($lun_status{err_soft}) {
                        $msg .= 'Soft Errors : '. $lun_status{err_soft} ."\n";
                    }
                    if (!$lun_status{active_threads}) {
                        $msg .= 'Active Threads : '. $lun_status{active_threads} ."\n";
                    }
                    platform_shim_test_case_throw_exception("Errors Found in IO for $lun_name!\n" . $msg);
            }

        }
        $tc->platform_shim_test_case_log_info($log_str);
        $count++;
        if ($num_of_luns == $io_increment) {
            last;
        }
        $io_increment = 0;
        $err_str = '';
    }

    if ($num_of_luns == $io_increment) {
        $tc->platform_shim_test_case_log_info('IO Status is OK');
    } else {
        platform_shim_test_case_throw_exception($err_str);
    }
}


# Method: test_util_io_stop_io
# Stops IO.
# 
# Input:
# A hash specifying the following key/value pairs
# 
# luns => \@ - ArrayRef to <Automatos::Component::Lun::Emc::Vnx> objects
# iox_session => $ - <Automatos::Io::Block::Iox> Object
# 
# Returns:
# *None*
# 
# Exceptions:
# *None*
# 

sub test_util_io_stop_io
{

    my $tc = shift;

    my %params = @_;

    $tc->platform_shim_test_case_log_info("Stopping IO to luns.");
	my %hash_stop_iox = (luns => $params{luns});
    platform_shim_host_iox_stop($params{iox_session}, \%hash_stop_iox);

}


1; 
