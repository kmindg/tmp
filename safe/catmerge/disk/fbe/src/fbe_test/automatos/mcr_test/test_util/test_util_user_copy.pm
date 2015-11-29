package mcr_test::test_util::test_util_user_copy;

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
use mcr_test::platform_shim::platform_shim_device;

sub test_util_user_copy_find_valid_spares {
    my ($self, $disk) =  @_;

    # Get the unbound disks.

    my $source_id = platform_shim_disk_get_property($disk, 'id');
    $self->platform_shim_test_case_log_info('Gathering valid spare disks... for ' . $source_id);

    my @technology = (platform_shim_disk_get_property($disk, "technology"));
    my @kind       = (platform_shim_disk_get_property($disk, "kind"));
    my %rg_params = (state      => "unused",
                     technology => \@technology,
                     kind       => \@kind,
                     vault      => 0,);

    my @disks    = platform_shim_device_get_disk($self->{device}, \%rg_params);
    my $capacity = platform_shim_disk_get_property($disk,"size");
    $capacity =~ s/(\d+).*/$1/ig;
    my @valid = ();
    foreach my $d (@disks) {
       my $cap = platform_shim_disk_get_property($d,"size");
        $cap =~ s/(\d+).*/$1/ig;
        my $id = platform_shim_disk_get_property($d, 'id');
        my $health_state = 'OK';
        if (platform_shim_device_is_unified($self->{device})) {
            $health_state = platform_shim_disk_get_property($d, 'health_state');
        } 
        $self->platform_shim_test_case_log_info("found disk $id with health state $health_state capacity $cap");
        if (($health_state =~ /OK/i) &&
            ($cap >= $capacity)         ) {
           push @valid, $d;
           $self->platform_shim_test_case_log_info("disk $id is a valid spare");
           return @valid
        }
    }

    return @valid;
}
sub test_util_find_copy_disk_for_rg {
   my ($self, $rg) =  @_;
   my @rg_disks = platform_shim_raid_group_get_disk($rg);
                        
   my $src_disk;
   my $dest_disk;
   foreach my $disk (@rg_disks) {
      my @valid_spares = test_util_user_copy_find_valid_spares($self, $disk);
      if (scalar(@valid_spares) > 0) {
         $src_disk = $disk;
         $dest_disk = shift(@valid_spares);
         return (source => $src_disk, dest => $dest_disk);
      }
   }
   $self->platform_shim_test_case_log_info("did not find any suitable copy disks");
   platform_shim_test_case_throw_exception("no suitable spares found");
   return (source => undef, dest => undef);
}
sub test_util_find_copy_disk_for_rg_mock {
   my ($self, $rg) =  @_;
   $self->platform_shim_test_case_log_info("did not find any suitable copy disks");
   platform_shim_test_case_throw_exception("no suitable spares found");
   return (source => undef, dest => undef);
}

sub test_util_find_copy_disk {
   my ($self, $disk) =  @_;
                        
   my $src_disk;
   my $dest_disk;
   
   my @valid_spares = test_util_user_copy_find_valid_spares($self, $disk);
   if (scalar(@valid_spares) > 0) {
      $src_disk = $disk;
      $dest_disk = shift(@valid_spares);
      return ($dest_disk);
   }
   $self->platform_shim_test_case_log_info("did not find any suitable copy disks");
   return (undef);
}
sub test_util_find_copy_disk_mock {
   my ($self, $disk) =  @_;
   $self->platform_shim_test_case_log_info("did not find any suitable copy disks");
   return (undef);
}
sub test_util_start_copy_with_error {

  my ($self, $source_disk, $dest_disk) = @_;

  my $info;                                         
  eval {
    ($info) = platform_shim_disk_user_copy_to($source_disk, $dest_disk);

    if ( my $e = Exception::Class->caught() ) {

      my $msg = ref($e) ? $e->full_message() : $e;

      print "message received as exception: " . $msg;

      if ( ($msg =~ /call failed/) || 
       ($msg =~ /ran unsuccessfully/) ||
       ($msg =~ /copytodisk command failed/)|| 
       ($msg =~ /Error returned from Agent/) ||
           ($msg =~ /is already performing a copy/) ) {

         print "copy to call failed as expected";
         self->platform_shim_test_case_log_info("Copy To Call Failed as Expected.");
         self->platform_shim_test_case_log_property_info( "Parsed output ", $msg );
         ($info) = {OutPut => $msg};
      } else {
          platform_shim_test_case_throw_exception("Copy To Called Failed with Unknown Exception: ".$msg);
      }      
    } else {
        platform_shim_test_case_throw_exception("Copy-To initiated successfully for this negative test. This is a failure: " );
    }
  }
}
1; 