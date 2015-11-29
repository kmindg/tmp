package mcr_test::test_util::test_util_general;

use strict;
use warnings;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use List::Util qw( shuffle );
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;

sub test_util_general_n_choose_k_array
{
    my ($array_ref, $k) = @_;
    my @array = @{$array_ref};
    my @shuffled_indexes = shuffle(0.. $#array);
    
    my @picked_indexes = @shuffled_indexes[ 0.. $k-1];
     
    my @picked = @array[@picked_indexes];
    return @picked;
}

sub test_util_general_shuffle_array
{
    my @temp = @_;
    
    my @shuffled_indexes = shuffle(0.. $#temp);
    return @temp[@shuffled_indexes];
}

my $test_start_time;
sub test_util_general_test_start {
   $test_start_time = time();
}

sub test_util_general_log_test_time{
   my ($self) = @_; 
   my $end = time();
   my $seconds = ($end - $test_start_time);
   my $minutes = $seconds / 60;
   $self->platform_shim_test_case_log_info("***********************************");
   $self->platform_shim_test_case_log_info("Total Run time was $minutes minutes\n");
   $self->platform_shim_test_case_log_info("***********************************");
}

sub test_util_general_set_target_sp
{
    my ($tc, $sp) = @_;
    my $device = $tc->{device};
    my $name = uc(platform_shim_sp_get_property($sp, 'name'));
    $tc->logInfo("Set target SP to: $name");
    return platform_shim_device_set_target_sp($device, $sp);
}

1;
