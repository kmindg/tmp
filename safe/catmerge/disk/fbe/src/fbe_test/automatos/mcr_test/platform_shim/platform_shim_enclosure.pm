package mcr_test::platform_shim::platform_shim_enclosure;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_raid_group;
use mcr_test::platform_shim::platform_shim_test_case;
use strict;

sub platform_shim_enclosure_get_property
{
	my ($encl, $property) =  @_;

	return $encl->getProperty($property); 	
}

sub platform_shim_enclosure_get_enclosure_summary
{
	my ($encl, $property) =  @_;

	return $encl->getEnclosureSummary(); 	
}

sub platform_shim_enclosure_induce_slf_failure
{
	my ($encl, $hash_ref) =  @_;
	my %data_hash = %$hash_ref;
	return $encl->induceSingleLoopFailure(%data_hash); 	
}

sub platform_shim_enclosure_find
{
	my ($encl, $hash_ref) =  @_;
	my %data_hash = %$hash_ref;
	return ($encl->find(%data_hash)); 	
}

sub platform_shim_enclosure_get_enclosure
{
    my ($encl) = @_;
	return ($encl->getEnclosure());
}

	
1;
