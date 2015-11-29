package mcr_test::platform_shim::platform_shim_dest;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use strict;
use warnings;
use Automatos::Exception;


=begin nd

Start DEST on a sp

=cut
sub platform_shim_dest_start($)
{
    my ($sp) = @_;

    $sp->dispatch('diskDestStart');
}

=begin nd

Stop DEST on a sp

=cut
sub platform_shim_dest_stop($)
{
    my ($sp) = @_;

    $sp->dispatch('diskDestStop');
}

=begin nd

Get DEST state

=cut
sub platform_shim_dest_get_state($)
{
    my ($sp) = @_;
    my $cmd = 'diskDestShow';

    my $dest_info;
    ($dest_info) = $sp->dispatch($cmd);
    return $dest_info->{parsed}{state};
}


=begin nd

Clean DEST record

=cut
sub platform_shim_dest_clean_records($)
{
    my ($sp) = @_;
    $sp->dispatch('diskDestClean');
}

=begin nd

Add a DEST record.

=cut
sub platform_shim_dest_add_record
{
    my $sp = shift;
    my $cmd = 'diskDestAdd';
    # TODO: Add validate here
    # See: Automatos/Framework/Dev/lib/Automatos/Wrapper/Tool/FbeCli/Common/Disk.pm, diskDestAdd
    my %param = (@_);
    $sp->dispatch($cmd, %param);
}

=begin nd

Get DEST records

=cut
sub platform_shim_dest_get_records($)
{
    my ($sp) = @_;
    my $cmd = 'diskDestRecordShow';
    my $dest_info;
    ($dest_info) = $sp->dispatch($cmd);
    return %{$dest_info->{parsed}};
}


1;
