package mcr_test::platform_shim::platform_shim_general;

use strict;
use warnings;


use Automatos::ParamValidate qw(validate_pos validate :types validate_with :callbacks);
use Automatos::Util::Units;

use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_raid_group;

use Exporter 'import';
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};


sub platform_shim_general_convert_size
{
    return Automatos::Util::Units::convertSize(@_);
}

sub platform_shim_general_get_number_from_size
{
    return Automatos::Util::Units::getNumber(@_); 
}


1;