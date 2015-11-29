package mcr_test::platform_shim::platform_shim_raid_group;

use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};

use Automatos::Util::Units;
use Automatos::Util::RaidType;
use Automatos::Util::RaidGroup;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_disk;
 
use strict;

# * Indicates static property
# Get - Supported only by getProperty()
# Set - Supported only by setProperty()
# Both - Both set/getProperty()
#                                              /- Major Wrappers-\ /-    Engineering Wrappers     -\
#  Field                             | Type    | REST | UEMCLI | TC | FBECLI | FBEAPIX |C4AminTool| Details
# =============================================================================================================
# * id                                | integer |      |        |    | Get    |         | Get      |   
# * luns                              | arrayref|      |        |    | Get    | Get     | Get      |   
# * disks                             | arrayref|      |        |    | Get    | Get     |          |   
# * fbe_id                            | integer |      |        |    | Get    | Get     |          |   
# * is_private                        | boolean |      |        |    | Get    | Get     |          |   
# * is_system_raid_group              | boolean |      |        |    | Get    | Get     |          |
# power_saving                      | boolean |      |        |    | Set    |         |          |
# in_power_saving_mode              | boolean |      |        |    | Get    |         |          |
# power_savings_idle_time           | string  |      |        |    | Both   |         |          |
# power_savings_maximum_raid_latency_time|string|    |        |    | Get    |         |          |
# background_operation_state        | boolean |      |        |    | Set    |         |          |
# background_operation_code         | string  |      |        |    | Set    |         |          |
# media_error_injected_count        | integer |      |        |    | Get    |         |          |    
# blocks_remapped_count             | integer |      |        |    | Get    |         |          |    
# error_injected_count              | integer |      |        |    | Get    |         |          |    
# validation_calls_count            | integer |      |        |    | Get    |         |          |    
# total_size                        | size    |      |        |    |        | Get     |          | 
# total_capacity                    | size    |      |        |    |        | Get     |          |       
# element_size                      | size    |      |        |    |        | Get     |          |    
# exported_block_size               | size    |      |        |    |        | Get     |          | 
# imported_block_size               | size    |      |        |    |        | Get     |          |       
# lun_align_size                    | size    |      |        |    |        | Get     |          |    
# optimal_block_size                | size    |      |        |    |        | Get     |          |   
# error_verify_checkpoint           | string  |      |        |    |        | Get     |          |    
# raid_type                         | string  |      |        |    |        | Get     | Get      |    
# raid_group_width                  | integer |      |        |    |        | Get     |          |      
# readonly_verify_checkpoint        | string  |      |        |    |        | Get     |          |    
# readwrite_verify_checkpoint       | string  |      |        |    |        | Get     |          |    
# sectors_per_stripe                | integer |      |        |    |        | Get     |          |    
# strip_count                       | integer |      |        |    |        | Get     |          |    
# fbe_state                         | string  |      |        |    |        | Get     |          |    
# paged_metadata_start_lba          | string  |      |        |    |        | Get     |          |    
# paged_metadata_size               | size    |      |        |    |        | Get     |          |      
# journal_log_start_pba             | string  |      |        |    |        | Get     |          |    
# journal_log_physical_size         | size    |      |        |    |        | Get     |          |    
# rebuild_checkpoint                | string  |      |        |    |        | Get     |          |    
# rebuild_checkpoints               | hashref |      |        |    |        | Get     |          |
# rebuild_logging_flags             | hashref |      |        |    |        | Get     |          |        
# rebuilt_percent                   | integer |      |        |    |        | Get     |          |    
# metadata_verify_pass_count        | integer |      |        |    |        | Get     |          |    
# metadata_element_state            | string  |      |        |    |        | Get     |          |    
# power_saving                      | boolean |      |        |    |        | Both    |          |    
# in_power_saving_mode              | boolean |      |        |    |        | Get     |          |    
# hibernation_time                  | string  |      |        |    |        | Get     |          |      
# time_since_last_io                | string  |      |        |    |        | Get     |          |    
# configured_idle_time              | string  |      |        |    |        | Get     |          |    
# * disk_uids                       | arrayref|      |        |    |        |         | Get      |   
# free_size                         | size    |      |        |    |        |         | Get      |   
# user_size                         | size    |      |        |    |        |         | Get      |   
# maximum_contiguous_size           | size    |      |        |    |        |         | Get      |   
# physical_size                     | size    |      |        |    |        |         | Get      |   
# * group_number                      | integer |      |        |    |        |         | Get      |   
# * uid                               | string  |      |        |    |        |         | Get      |   
# percent_expanded                  | integer |      |        |    |        |         | Get      |   
# raid_label                        | string  |      |        |    |        |         | Get      | 
# 'rekey_checkpoint'                | string  |                             | Get     | Get      |

sub platform_shim_raid_group_get_property
{
	my ($raid_group, $property) = @_;
	
	return $raid_group->getProperty($property); 	
}

sub platform_shim_raid_group_create
{
	my ($device, $params_ref) = @_;
	my %params = %$params_ref;
	
	return $device->createRaidGroup(%params);
}

sub platform_shim_raid_group_destroy
{
	my ($raid_group) = @_;
	
	$raid_group->remove();
}

our %VALID_WIDTHS = (
    disk => [1],
    r0 => [3..16],
    r1 => [2],
    r1_0 => [2,4,6,8,10,12,14,16],
    r3 => [5,9],
    r5 => [3..16],
    r6 => [4,6,8,10,12,14,16]
    );
$VALID_WIDTHS{r10} = $VALID_WIDTHS{r1_0};
$VALID_WIDTHS{id} = $VALID_WIDTHS{disk};
sub platform_shim_raid_group_get_raid_type_valid_widths
{
  return %VALID_WIDTHS;	
}


sub platform_shim_raid_group_get_disk
{
	my ($raid_group) = @_;
	
	return $raid_group->getDisk();	
}

sub platform_shim_raid_group_get_downstream_mirrors
{
    my ($raid_group) = @_;
    my $raid_type = platform_shim_raid_group_get_property($raid_group, 'raid_type');
    my @mirror_fbe_ids = ();
    
    if ($raid_type eq $Automatos::Util::RaidType::RAID10)
    {
        my $device = $raid_group->getDevice(); 
        my %params = (object => $raid_group);
        my ($ret_hash) = $device->dispatch('deviceGetDownstreamObject', %params);
        @mirror_fbe_ids = split(/\s+/, $ret_hash->{parsed}{downstream_object_list});
    }
    
    return \@mirror_fbe_ids;
}

sub platform_shim_raid_group_get_lun
{
    my ($raid_group) = @_;
    
    my $device = $raid_group->getDevice();
    if (platform_shim_device_is_unified($device))
    {
        return $device->find(type => 'backendLun',
                              criteria => {raid_group => $raid_group->getProperty('id')});
    }
    
    return $raid_group->getLun();
}

sub platform_shim_raid_group_mark_dirty
{
	my ($rg) = @_;
	$rg->markDirty();
}

sub platform_shim_raid_group_verify_swap
{
    my ($raid_group, $disk, $spare) = @_;
    # Verify raid group config
    Automatos::Util::RaidGroup::verifySwap(raid_group => $raid_group, 
                                           source => $disk, 
                                           destination => $spare);
}

sub platform_shim_raid_group_get_raidgroup
{
	my ($raid_group) = @_;
	
	return ($raid_group->getRaidGroup());	
}

sub platform_shim_raid_group_set_raid_group_debug_flags
{
	my ($sp, $raid_group, $raid_group_debug_flags, $raid_library_debug_flags) =  @_;
    my $status = 0;
    my $rg_object_id = platform_shim_raid_group_get_property($raid_group, 'fbe_id');
    my $rg_object_id_string = '0x' . sprintf "%X", $rg_object_id;
    my %params = (object_fbe_id => $rg_object_id_string,);
    my $debug_flag_one = '-debug_flags 0x' . hex($raid_group_debug_flags) . ' ';
    $params{debug_flag_one} = $debug_flag_one;
    my $debug_flag_two = '-library_flags 0x' . hex($raid_library_debug_flags) . ' ';
    $params{debug_flag_two} = $debug_flag_two;

    #print "set raid group debug flags for: " . $rg_object_id . " " . $rg_object_id_string . " " . $debug_flag_one . ": 0x" . hex($raid_group_debug_flags) . "\n";
    #print "set raid library debug flags for: " . $rg_object_id . " " . $rg_object_id_string . " " . $debug_flag_two . ": 0x" . hex($raid_library_debug_flags) . "\n";
    
	my $info;
    eval {
        ($info) = $sp->dispatch('raidGroupSetDebugFlags', %params);
    };
    if (my $e = Exception::Class->caught()) {
        my $msg = ref($e) ? $e->full_message() : $e;
        print "Failed to set raid group debug flags \n" . $msg;
        $status = -1;
	}
    return $status;
}


sub platform_shim_raid_group_map_lba
{
    my ($raid_group, $lba) = @_;

    return $raid_group->mapLba(lba => $lba);   
}


sub platform_shim_raid_group_get_rg_info ($$)
{
    my ($sp, $object_id) = @_;
    my $sp_name = platform_shim_sp_get_short_name($sp);

    my @cmd = (
        'fbe_api_object_interface_cli.exe',
        '-k', '-' . $sp_name,
        '-sep', '-rg', 'getRaidgroupInfo', $object_id
    );

    my %hash;
    platform_shim_sp_execute_command($sp, \@cmd, \%hash);

    my %rg_info;
    $rg_info{rg_object_id} =                   $hash{'RG object ID'};
    $rg_info{rg_type} =                        $hash{'RG Type'};
    $rg_info{rg_width} =                       $hash{'RG Width'};
    $rg_info{rg_state} =                       $hash{'RG State'};
    $rg_info{lun_align_size} =                 $hash{'LUN Align Size'};
    $rg_info{luns_in_rg} =                     $hash{'Lun(s) in RG'};
    $rg_info{overall_capacity} =               $hash{'Overall Capacity'};
    $rg_info{element_size} =                   $hash{'Element Size'};
    $rg_info{sectors_per_stripe} =             $hash{'Sectors per stripe'};
    $rg_info{stripe_count} =                   $hash{'Stripe Count'};
    $rg_info{exported_block_size} =            $hash{'Exported Block Size'};
    $rg_info{imported_block_size} =            $hash{'Imported Block Size'};
    $rg_info{optimal_block_size} =             $hash{'Optimal Block Size'};
    $rg_info{rw_verify_checkpoint} =           $hash{'RW verify checkpoint'};
    $rg_info{ro_verify_checkpoint} =           $hash{'RO verify checkpoint'};
    $rg_info{error_verify_checkpoint} =        $hash{'Error verify checkpoint'};
    $rg_info{paged_metadata_start_lba} =       $hash{'Paged Metadata Start LBA'};
    $rg_info{paged_metadata_size} =            $hash{'Paged Metadata Capacity'};
    $rg_info{metadata_element_state} =         $hash{'Metadata Element State'};
    $rg_info{metadata_verify_pass_count} =     $hash{'Metadata Verify Pass Count'};
    $rg_info{journal_log_start_pba} =          $hash{'Journal log start PBA'};
    $rg_info{journal_log_physical_capacity} =  $hash{'Journal log Physical Capacity'};
    #$rg_info{} =                              $hash{'Rebuild Chkpt for 0_0_8'};
    #$rg_info{} =                              $hash{'Rebuild Chkpt for 2_0_18'};
    #$rg_info{} =                              $hash{'Rebuild logging flag 0_0_8'};
    #$rg_info{} =                              $hash{'Rebuild logging flag 2_0_18'};
    #$rg_info{} =                              $hash{'Drives associated with this RG'};

    return \%rg_info
}


sub platform_shim_raid_group_wait_for_ready_state
{
    my ($device, $raid_group_ref) = @_;

    $device->waitForPropertyValue(objects => $raid_group_ref,
                                  property => 'fbe_state',
                                  value => 'READY',
                                  timeout => '5 M');
}


sub platform_shim_raid_group_wait_for_failed_state
{
	my ($device, $raid_group_ref) = @_;
	
	$device->waitForPropertyValue(objects => $raid_group_ref,
                                  property => 'fbe_state',
                                  value => 'FAIL',
                                  timeout => '2 M');
}
sub platform_shim_raid_group_get_info
{
    my ($tc, $rg, $rg_id) = @_;
    my $device = $tc->{device};
    
    my $sp = platform_shim_device_get_target_sp($device);
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "rginfo -rg $rg_id", \%hash)};
    my $string = $ret_hash{stdout};
    my @lines = split /\n/, $string;
    my $capacity;
    my %rg_info_hash = ();
    my @rb_chkpt;
    my $index = 0;
    foreach my $line (@lines) {
       #print $line . "\n";
       if ($line =~ /Rebuild checkpoint\[\d+\].+(0x[0123456789abcdef]+)/){
          my $chkpt = Math::BigInt->from_hex($1);
          push @rb_chkpt, $chkpt;
          printf("RG: %u checkpoint [%u] 0x%x\n", $rg_id, $index, $chkpt);
          $index++;
       } elsif ($line =~ /user Capacity:.+(0x[0123456789abcdef]+)/){
          my $capacity = Math::BigInt->from_hex($1);
          $rg_info_hash{capacity} = $capacity;
          printf("RG: %u capacity 0x%x\n", $rg_id, $capacity);
       }
    }
    $rg_info_hash{rebuild_chkpt} = \@rb_chkpt;
    return \%rg_info_hash;
}

sub platform_shim_raid_group_get_logical_capacity 
{
   my ($tc, $rg, $rg_id) = @_;
   my $rg_info_hash_ref = platform_shim_raid_group_get_info($tc, $rg, $rg_id);
   return $rg_info_hash_ref->{capacity};
}

sub platform_shim_raid_group_power_savings_set_state_enabled
{
    my ($rg) = @_;

    my $device = $rg->getDevice();
#    my %params = (object => $rg,
#                  state => 1);
#    $device->dispatch('raidGroupSetPowerSaving', %params);

    my $sp = platform_shim_device_get_target_sp($device);
    my $rg_id = platform_shim_raid_group_get_property($rg, "id");
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -rg $rg_id -state enable", \%hash)};
}

sub platform_shim_raid_group_power_savings_set_state_disabled
{
    my ($rg) = @_;

    my $device = $rg->getDevice();
#    my %params = (object => $rg,
#                  state => 0);
#    $device->dispatch('raidGroupSetPowerSaving', %params);

    my $sp = platform_shim_device_get_target_sp($device);
    my $rg_id = platform_shim_raid_group_get_property($rg, "id");
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -rg $rg_id -state disable", \%hash)};

}

# sub platform_shim_raid_group_power_savings_show
# 
# Returns the following hash ref  (example)
#    {  
#       'power_savings_maximum_raid_latency_time' => '120 S',
#       'in_power_saving_mode' => 1,
#       'id' => '3',
#       'power_savings_idle_time' => '1800 S'
#    }
#       
sub platform_shim_raid_group_power_savings_show
{
    my ($rg) = @_;

    my $device = $rg->getDevice();
    my %params = (object => $rg);
    my $rg_id = platform_shim_raid_group_get_property($rg, "id");

    my ($ret_hash) = $device->dispatch('raidGroupShowPowerSaving', %params);
    return $ret_hash->{parsed}->{$rg_id};
}

sub platform_shim_raid_group_power_savings_get_max_latency_time
{
    my ($rg) = @_;

    my ($ret_hash) = platform_shim_raid_group_power_savings_show($rg);
    return $ret_hash->{power_savings_maximum_raid_latency_time};
}

sub platform_shim_raid_group_power_savings_is_saving
{
    my ($rg) = @_;

    my ($ret_hash) = platform_shim_raid_group_power_savings_show($rg);
    return $ret_hash->{in_power_saving_mode};
}

sub platform_shim_raid_group_power_savings_get_idle_time
{
    my ($rg) = @_;

    my ($ret_hash) = platform_shim_raid_group_power_savings_show($rg);
    return $ret_hash->{power_savings_idle_time};
}

sub platform_shim_raid_group_power_savings_set_idle_time
{
    my ($rg, $time) = @_;

    my $device = $rg->getDevice();

    my $time_sec = Automatos::Util::Units::getNumber(Automatos::Util::Units::convertTime($time, 'S'));

    my $sp = platform_shim_device_get_target_sp($device);
    my $rg_id = platform_shim_raid_group_get_property($rg, "id");
    my %hash = ();
    my %ret_hash = %{platform_shim_sp_execute_fbecli_command($sp, "green -rg $rg_id -idle_time $time_sec", \%hash)};
}


#todo:  hib value doesn't look correct
sub platform_shim_raid_group_power_savings_get_hibernation_time
{
	my ($rg) = @_;
	
    return $rg->getProperty('hibernation_time');
}

1;
