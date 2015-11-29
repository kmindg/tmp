package mcr_test::test_util::test_util_dest;

use strict;
use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use mcr_test::platform_shim::platform_shim_disk;
use mcr_test::platform_shim::platform_shim_test_case;
use mcr_test::platform_shim::platform_shim_dest;

sub test_util_dest_stop($$)
{
    my ($self, $sp) = @_;
    $self->platform_shim_test_case_log_info("Stopping DEST, SP: ".
                                            platform_shim_sp_get_property($sp, 'name'));
    platform_shim_dest_stop($sp);
}

sub test_util_dest_start($$)
{
    my ($self, $sp) = @_;

    $self->platform_shim_test_case_log_info("Starting DEST, SP: ".
                                            platform_shim_sp_get_property($sp, 'name'));
    platform_shim_dest_start($sp);
    my $state = test_util_dest_get_state($self, $sp);
    if ( $state !~ /Started/i ) {
        platform_shim_test_case_throw_exception('DEST state is \'' . $state . '\'' );
    }
}

sub test_util_dest_get_state($$)
{
    my ($self, $sp) = @_;
    $self->platform_shim_test_case_log_info("Get DEST state, SP: ".
                                            platform_shim_sp_get_property($sp, 'name'));

    return platform_shim_dest_get_state($sp);
}

sub test_util_dest_clear($$)
{
    my ($self, $sp) = @_;
    $self->platform_shim_test_case_log_info("Clear DEST errors, SP: ".
                                            platform_shim_disk_get_property($sp, 'name'));
    platform_shim_dest_clean_records($sp);
}


sub test_util_dest_inject_media_error
{
    my $self = shift;
    my $disk = shift;
    my $sp = shift;

    my %params = platform_shim_test_case_validate(
        @_,
        {
            'lba_start' => { type => SCALAR },
            'lba_end' => { type => SCALAR },
            'error_code' => { type => SCALAR, default => '0x031100' },
            'error_type' => { type => SCALAR, default=>'scsi' },
            'opcode' => { type => SCALAR, optional => 1 },
            'error_insertion_count' => { type => SCALAR, default => 1 },
        }
    );
    $params{object} = $disk;

    $self->platform_shim_test_case_log_info("Inject Error, disk: "
                                            .platform_shim_disk_get_property($disk, 'id')
                                            .", Lba: [$params{lba_start}, $params{lba_end}]"
                                            .", Opcode: $params{opcode}, type: $params{error_type}");
    platform_shim_dest_add_record($sp, %params);
}

1;
