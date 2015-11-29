package mcr_test::platform_shim::platform_shim_host;

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

sub platform_shim_host_get_hostname
{
	my ($host) =  @_;

	return $host->getHostname(); 	
}

sub platform_shim_host_get_host_object
{
	my ($host) =  @_;

	return $host->getHostObject(); 	
}

sub platform_shim_host_reboot
{
	my ($host, $hash_data) =  @_;

	return $host->reboot(%$hash_data); 	
}

sub platform_shim_host_iox_block_session
{
	my ($host) =  shift;
	
	return $host->createIoxBlockSession();
}

sub platform_shim_host_iox_getstatus
{
	my ($host) =  shift;
	
	return $host->getStatus();
}

sub platform_shim_host_iox_init
{
	my ($host) =  shift;
	
	return $host->init();
}

sub platform_shim_host_iox_start
{
	my ($host, $hash_ref) =  @_;
	my %hash_data = %$hash_ref;
	
	return $host->start(%hash_data);
}

sub platform_shim_host_iox_stop
{
	my ($host, $hash_ref) =  @_;
	my %hash_data = %$hash_ref;
	
	return $host->stop(%hash_data);
}


1;
