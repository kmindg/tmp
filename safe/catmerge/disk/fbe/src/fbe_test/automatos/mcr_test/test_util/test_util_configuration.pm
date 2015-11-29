package mcr_test::test_util::test_util_configuration;

use strict;
use warnings;

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
use mcr_test::test_util::test_util_general;
use mcr_test::test_util::test_util_disk;
use mcr_test::platform_shim::platform_shim_general;
use Automatos::Util::Compare qw(:block);
use Params::Validate;

my $g_log = mcr_test::platform_shim::platform_shim_test_case->platform_shim_test_case_get_logger();


#our @g_redundant_raid_types = qw(r1 r3 r5 r6 r10);
our @g_raid_types = qw(r0 r1 r3 r5 r6 r10);
our @g_raid_types_plus_id = qw(id r0 r1 r3 r5 r6 r10);
our @g_parity_raid_types = qw(r3 r5 r6);
our @g_striper = qw(r0 r10);
our @g_mirror = qw(r1);

our %g_unity_raid_type_map = ("r5" => 1,
                              "disk" => 2,
                              "r1" => 3,
                              "r0" => 4,
                              "r3" => 5,
                              "r10" => 7,
                              "r6" => 10);

sub test_util_configuration_create_redundant_raid_group
{
   my ($tc, $params_ref) = @_;

    # Get the unbound disks.
    $tc->platform_shim_test_case_log_info('Gathering disks to use for Raid Group...');

    my %rg_params =(state      =>"unused",
    					  vault 	 => 0,
                          disk_count => 5,
                          kind       => ['sas'],
                          technology => ['normal']);
                          
    my @disks = platform_shim_raid_group_get_disk($tc->{device}, \%rg_params);

    test_util_configuration_create_raid_group($tc, 
    							{disks     => \@disks,
                                 raid_type => $params_ref->{raid_type}});
}

sub test_util_configuration_create_raid_group
{
	my ($tc, $params_ref) = @_;
	
	my $raid_type = $params_ref->{raid_type};
	my @disks = @{$params_ref->{disks}};
	
	my $rg = platform_shim_raid_group_create($tc->{device}, 
    										 {disks     => \@disks,
                                              raid_type => $raid_type});
    $tc->platform_shim_test_case_log_info('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' .platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');  
                   
    @disks = platform_shim_raid_group_get_disk($rg);
    foreach my $disk (@disks) {
        $tc->platform_shim_test_case_log_info("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
    } 
                                                          		   
    return $rg;	
	
}

sub test_util_configuration_create_lun_size
{
	my ($tc, $raid_group, $size) = @_;
        
    my $raid_type = platform_shim_raid_group_get_property($raid_group, 'raid_type'); 
    my $width = platform_shim_raid_group_get_property($raid_group, 'raid_group_width');
    if ($width == 1) {
       $raid_type = "disk";
    }
    my %lun_params = (size  => $size,
                     lun_count => 1,
                     raid_type => $raid_type);


    if (platform_shim_device_is_unified($tc->{device}))
    {
        my $block_count = platform_shim_general_convert_size($size, "BC");
        $block_count = platform_shim_general_get_number_from_size($block_count);
        %lun_params = (#size  => $size,
                       #   lun_count => 1,
                       block_count => $block_count,
                       #raid_type => platform_shim_raid_group_get_property($raid_group, 'raid_type'));
        );
    }    
    
    
    # Create the LUN.
    my @lun_objects = platform_shim_lun_bind($raid_group, \%lun_params);
     my $lun = shift @lun_objects;
    $tc->platform_shim_test_case_log_info('Created LUN ' . platform_shim_lun_get_property($lun, 'id') .
                 ' with a size of ' . platform_shim_lun_get_property($lun, 'size'));
    
    return $lun;
}

sub test_util_configuration_create_lun_no_initial_verify
{
	my ($tc, $raid_group, $size) = @_;

    my %lun_params = (size  => $size,
                     lun_count => 1,
                     raid_type => platform_shim_raid_group_get_property($raid_group, 'raid_type'),
                     initial_verify => 0);
                     
    if (platform_shim_device_is_unified($tc->{device}))
    {
        my $block_count = platform_shim_general_convert_size($size, "BC");
        $block_count = platform_shim_general_get_number_from_size($block_count);
        %lun_params = (block_count => $block_count);
    }

	# Create the LUN.
    my @lun_objects = platform_shim_lun_bind($raid_group, \%lun_params);
    my $lun = shift @lun_objects;
    $tc->platform_shim_test_case_log_info('Created LUN ' . platform_shim_lun_get_property($lun, 'id') .
                 ' with a size of ' . platform_shim_lun_get_property($lun, 'size'));
    
    return $lun;
}


sub test_util_configuration_create_lun
{
	my $tc = shift;
	my $raid_group = shift;
	my %params = platform_shim_test_case_validate(@_,
                             {size => { type => SCALAR , default => '100MB'},
                              lun_count => { type => SCALAR , default => 1 },
                              initial_verify => { type => SCALAR , default => 0 },
                             });
    
    
    if (platform_shim_device_is_unified($tc->{device}))
    {
        my $block_count = platform_shim_general_convert_size($params{size}, "BC");
        $block_count = platform_shim_general_get_number_from_size($block_count);
        my $lun_count = $params{lun_count};
        %params = (block_count => $block_count);
        my $lun;
        foreach my $cnt (1..$lun_count)
        {
            # Create the LUN.
            my @lun_objects = platform_shim_lun_bind($raid_group, \%params);
            $lun = shift @lun_objects;
            $tc->platform_shim_test_case_log_info('Created LUN ' . platform_shim_lun_get_property($lun, 'id') .
                     ' with a size of ' . platform_shim_lun_get_property($lun, 'size'));
        
                
        }
        
        return $lun;
    } 
    else
    {
        $params{raid_type} = platform_shim_raid_group_get_property($raid_group, 'raid_type');
        # Create the LUN.
        my @lun_objects = platform_shim_lun_bind($raid_group, \%params);
        my $lun = shift @lun_objects;
        $tc->platform_shim_test_case_log_info('Created LUN ' . platform_shim_lun_get_property($lun, 'id') .
                 ' with a size of ' . platform_shim_lun_get_property($lun, 'size'));
    
        return $lun; 
    }
}



#############################################
#
# Note: Need to rework
#
#############################################


###############
# GLOBALS
###############

our %g_drive_config;
# keeping r1_0 out for now since the rebuild hook needs to be added to the mirror
# not the striper.
our @g_redundant_raid_types = qw(r5 r3 r1 r6); #qw(r5 r3 r1_0 r1 r6);
our @g_nonredundant_raid_types = qw(r0);


=for nd

Method: getRedundantRaidTypes
Returns an array of redundant raid types

Input:
*None*

Returns:
@redundantRaidTypes

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub get_redundant_raid_types
{
    return @g_redundant_raid_types;
}

=for nd

Method: fetchDiskInfo
Get all the disks in the array

Input:
A hash containing the following key/value pairs

device => $ - An Automatos::Device::Storage::Emc::Vnx::Block object
vault => $ - *Optional* A boolean for using vault (1) drives or not (2)
update => $ - *Optional* A boolean to update the list of drives
disks => $ - *Optional* A reference to an array of Automatos::Component::Disk objects to use

Returns:
*None*

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_fetch_disk_info
{
    my $tc = shift;
   
    my %params = platform_shim_test_case_validate(@_,
                             {vault => { type => SCALAR , default => 0 },
                              update => { type => SCALAR , default => 0 },
                              disks => { type => ARRAYREF, optional => 1 },
                              b_zeroed_only => { type => BOOLEAN , default => 0 },
                              b_spindown_capable_only => { type => BOOLEAN , default => 0 },
                             });
	my $device = $tc->{device};
    my %raid_group_params =(state => "unused",
                          vault => $params{vault});
    
    my @disks = ();
    if ($params{disks}) {
        @disks = @{$params{disks}};
        $params{update} = 1;
    } else {                     
        @disks = platform_shim_device_get_disk($device, \%raid_group_params);
    }
    
    if (!%g_drive_config || $params{update}) {
	    %g_drive_config = ();
	    my $disk_info_ref = test_util_disk_get_pvd_info_hash($tc);	    

	    # sort the disks by drivetype
	    foreach my $disk (@disks) {
	        if ($params{b_zeroed_only} && 
	            $disk_info_ref->{platform_shim_disk_get_property($disk, 'id')}{zero_percent} != 100) {
	            next;
	        } 
            if ($params{b_spindown_capable_only})
            {
                my $active_sp = platform_shim_device_get_active_sp($device, {});
                if (! platform_shim_disk_power_savings_is_capable($active_sp, $disk))
                {
                    next;
                }
            }
	        my $tier = test_util_configuration_drive_type_to_tier(kind=> platform_shim_disk_get_property($disk, 'kind'), 
                                                                  technology => platform_shim_disk_get_property($disk, 'technology'));                
            if ($tier) {                                                                                                                                       
	           push (@{$g_drive_config{$tier}}, $disk);
            } else {
                $tc->platform_shim_test_case_log_warn("Disk ".platform_shim_disk_get_property($disk, 'id').
                			" kind: ".platform_shim_disk_get_property($disk,'kind').
                            " technology: ".platform_shim_disk_get_property($disk,'technology'));
            }    
	    }
    } 
    
    return %g_drive_config;
}

=for nd

Method: tierToDriveType
return an array of drive types that belong to a given drive tier

Input:
A hash containing the following key/value pairs

tier => $ - A drive tier

Returns:
@drivetypes

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub tier_to_drive_type
{
    my %params = platform_shim_test_case_validate(@_,
                             {tier => { type => SCALAR } 
                             }); 

    my %mapping = test_util_configuration_get_performance_tiers(); 
    
    return @{$mapping{$params{tier}}};            
    
}

=for nd

Method:getPerformanceTiers
Returns a hash for the mapping of drive tier to drive types

Input:
*None*

Returns:
%mapping

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_get_performance_tiers
{
    my %mapping = ();
    
    $mapping{'FBE_PERFORMANCE_TIER_ZERO'} = [{kind => "SAS", technology => "FLASH"}];     
    $mapping{'FBE_PERFORMANCE_TIER_ONE'} = [{kind => "NL_SAS", technology => "NORMAL"}];    
    $mapping{'FBE_PERFORMANCE_TIER_TWO'} = [{kind => "SAS", technology => "NORMAL"}];    
    $mapping{'FBE_PERFORMANCE_TIER_THREE'} = [{kind => "SATA", technology => "FLASH"}];    
    $mapping{'FBE_PERFORMANCE_TIER_FOUR'} = [{kind => "SATA", technology => "NORMAL"}];  
    $mapping{'FBE_PERFORMANCE_TIER_FIVE'} = [{kind => "SAS", technology => "FAST_VP"}]; 
    
    return %mapping;
}

=for nd

Method: driveTypeToTier
Returns the drive tier given a drive type

Input:
A hash containing the following key/value pairs
    kind => $ - The kind property of a disk
    technology => $ - The technology property of a disk

Returns:
$ - performance tier

Exceptions:
-<Automatos::Exception::Base>

Note: combine this and the top one or remove one
=cut 

sub test_util_configuration_drive_type_to_tier
{
    my %params = platform_shim_test_case_validate(@_,
                             {kind => { type => SCALAR },
                              technology => { type => SCALAR , default => "NORMAL"},   
                             }); 
                             
    my %mapping = ();
    
    $mapping{SAS}{FLASH} = 'FBE_PERFORMANCE_TIER_ZERO';
    $mapping{NL_SAS}{NORMAL} = 'FBE_PERFORMANCE_TIER_ONE';
    $mapping{SAS}{NORMAL} =  'FBE_PERFORMANCE_TIER_TWO';
    $mapping{SATA}{FLASH} = 'FBE_PERFORMANCE_TIER_THREE';
    $mapping{SATA}{NORMAL} = 'FBE_PERFORMANCE_TIER_FOUR';
    $mapping{SAS}{FAST_VP} = 'FBE_PERFORMANCE_TIER_FIVE';

    return $mapping{uc($params{kind})}{uc($params{technology})};                  
}

=for nd

Method:getPerformanceTierRating
Returns a hash for the mapping of drive tier to drive types

Input:
*None*

Returns:
%mapping

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_get_performance_tier_rating
{
    my %mapping = ();
    
    $mapping{'FBE_PERFORMANCE_TIER_ZERO'} = 1;     
    $mapping{'FBE_PERFORMANCE_TIER_ONE'} = 4;    
    $mapping{'FBE_PERFORMANCE_TIER_TWO'} = 3;    
    $mapping{'FBE_PERFORMANCE_TIER_THREE'} = 2;    
    $mapping{'FBE_PERFORMANCE_TIER_FOUR'} = 5;  
    $mapping{'FBE_PERFORMANCE_TIER_FIVE'} = 6; 
    
    return %mapping;
}


=for nd

Method: createRedundantConfig
Create a random redundant configuration

Input:
A hash containing the following key/value pairs

device => $ - An Automatos::Device::Storage::Emc::Vnx::Block object
min_disks => $ - *Optional* The minimum disks to use 
max_disks => $ - *Optional* The maximum disks to use
vault => $ - *Optional* A boolean for using vault (1) drives or not (2)
technology => $ - *Optional* A particular type of disk technology to use
sort_function => \& - *Optional* A function to sort the selected disks by
filter => \& - *Optional* A function to filter the disk configuration hash
reserve => *Optional* The number of disks to reserve. Default is 1

Returns:
$raid_group - Automatos::Component::RaidGroup object

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_create_redundant_config
{
    my ($tc) = shift;
    my %params = platform_shim_test_case_validate(@_,
                             {
                              min_disks => { type => SCALAR , default => 0 },
                              max_disks => { type => SCALAR , default => 16 },   
                              vault => { type => SCALAR , default => 0 },
                              technology => {type => SCALAR , default => "normal" },
                              kind => { type => SCALAR , optional => 1 },
                              sort_function => { type => CODEREF, optional => 1},
                              reserve => { type => SCALAR, default => 1 },
                              filter => { type => CODEREF, optional => 1},
                              tier_sort => {type => CODEREF, optional => 1, default => test_util_configuration_by_random_tier()}, 
                              b_zeroed_only => { type => BOOLEAN , default => 0 },
                             });
                             
    my $rand_index = int(rand(scalar(@g_redundant_raid_types)));
    $params{raid_type} = $g_redundant_raid_types[$rand_index];
    return test_util_configuration_create_random_raid_type_config($tc, \%params);
}

=for nd

Method: createRandomRaidTypeConfig
Create a random redundant configuration based on raid type

Input:
A hash containing the following key/value pairs

device => $ - An Automatos::Device::Storage::Emc::Vnx::Block object
raid_type => $ - A particular raid type to use for creating the raid group
min_disks => $ - *Optional* The minimum disks to use 
max_disks => $ - *Optional* The maximum disks to use
vault => $ - *Optional* A boolean for using vault (1) drives or not (2)
kind => $ - *Optional* A particular kind of disk to use
technology => $ - *Optional* A particular type of disk technology to use
sort_function => \& - *Optional* A function to sort the selected disks by
filter => \& - *Optional* A function to filter the disk configuration hash
reserve => *Optional* The number of disks to reserve. Default is 1

Returns:
$raid_group - Automatos::Component::RaidGroup object

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_create_random_raid_type_config
{
    my $tc = shift;
    
    my %params = platform_shim_test_case_validate(@_,
                             {raid_type => { type => SCALAR },
                              min_disks => { type => SCALAR , optional => 1 },
                              max_disks => { type => SCALAR , optional => 1 },
                              vault => { type => SCALAR , default => 0 },
                              technology => {type => SCALAR , default => "normal" },
                              kind => { type => SCALAR , optional => 1 },
                              sort_function => { type => CODEREF, optional => 1},
                              reserve => { type => SCALAR, default => 1 },
                              filter => { type => CODEREF, optional => 1},
                              tier_sort => {type => CODEREF, optional => 1, default => test_util_configuration_by_random_tier()},
                              b_zeroed_only => { type => BOOLEAN , default => 0 },
                             });
    
    my $raid_type = delete $params{raid_type};

    my $device = $tc->{device};

    #Automatos::Util::Configuration::fetchDiskInfo(device => $blockDev, vault => $params{vault});
    my %drive_config = test_util_configuration_fetch_disk_info($tc, {vault => $params{vault}, b_zeroed_only => $params{b_zeroed_only}});
    
    $tc->platform_shim_test_case_log_debug("Create a Random RG config of RAID type $raid_type");
    # randomly select DriveType to use based on the selected raidType min Width
    if (exists($params{filter})) {
        $tc->platform_shim_test_case_log_debug("Applying filter to drive_config");
        %drive_config = $params{filter}(\%drive_config);   
    }
    
    my @available_tiers = keys %drive_config;
    my %valid_widths = platform_shim_raid_group_get_raid_type_valid_widths();
    my @raid_type_widths = @{$valid_widths{$raid_type}};
    if (exists($params{min_disks})) {
        @raid_type_widths = grep {$_ >= $params{min_disks}} @raid_type_widths;
        delete $params{min_disks};
    } 
    
    if (exists($params{max_disks})) {
        @raid_type_widths = grep {$_ <= $params{max_disks}} @raid_type_widths;
        delete $params{max_disks};
    } 
    
    # Consider drives we want to reserve for sparing
    my $min_width = $raid_type_widths[0];
    $min_width += $params{reserve} if ($params{reserve});
    my @valid_tiers = grep {scalar(@{$drive_config{$_}}) >= $min_width} @available_tiers;

    if (scalar(@valid_tiers) == 0) {
        platform_shim_test_case_throw_exception("There are not enough of any drive type to create a raid group");
    }
    
    my $selected_tier = $params{tier_sort}(@valid_tiers);
    if ($params{kind} && $params{technology}) {
        $selected_tier = test_util_configuration_drive_type_to_tier(kind => $params{kind}, 
                                                              			   technology => $params{technology});
    }
    
    
    # randomly select the number of drives to use but save number reserved for sparing
    my $num_drive_type = scalar(@{$drive_config{$selected_tier}});
    $num_drive_type -= $params{reserve} if ($params{reserve});
    my @possible_widths = grep {$num_drive_type >= $_} @raid_type_widths;
    my $num_drives = $possible_widths[int(rand(scalar(@possible_widths)))];
         
    if (exists($params{sort_function})) {       
        @{$drive_config{$selected_tier}} = $params{sort_function}(@{$drive_config{$selected_tier}});
    } else {
        @{$drive_config{$selected_tier}} = sort {platform_shim_disk_get_property($a, 'capacity') <=> platform_shim_disk_get_property($b, 'capacity')} 
                                         @{$drive_config{$selected_tier}};
    }         
    
    my @disks = @{$drive_config{$selected_tier}};
    @disks = splice(@disks, 0, $num_drives);
    
    if (platform_shim_device_is_unified($device))
    {
        $raid_type = $g_unity_raid_type_map{$raid_type};
    }
    
    # create the raid groups with the random disks selected
    my $rg = platform_shim_raid_group_create( $device,
    															 {disks     => \@disks,
                                              					  raid_type => $raid_type});                                        
    $tc->platform_shim_test_case_log_debug('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' . platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');  
                   
    @disks = platform_shim_raid_group_get_disk($rg);
    foreach my $disk (@disks) {
        $tc->platform_shim_test_case_log_debug("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
    } 
                                             
    return $rg;

}

=for nd

Method: createNonRedundantConfig
Create a random nonredundant configuration 

Input:
A hash containing the following key/value pairs

device => $ - An Automatos::Device::Storage::Emc::Vnx::Block object
vault => $ - A boolean for using vault (1) drives or not (2)

Returns:
$raid_group - Automatos::Component::RaidGroup object

Exceptions:
-<Automatos::Exception::Base>

=cut 

sub test_util_configuration_create_non_redundant_config
{
    my $tc = shift;
    my %params = platform_shim_test_case_validate(@_,
                              {vault => { type => SCALAR , default => 0 }
                             });
    
    my $device = $tc->{device};
    my %valid_widths = platform_shim_raid_group_get_valid_widths();
    my $num_drives = $valid_widths{"r0"}[0];
    
    # Get the unbound disks.
    $tc->platform_shim_test_case_log_debug('Gathering disks to use for Raid Group...');

    my %drive_config = test_util_configuration_fetch_disk_info(%params);   
    my @available_tiers = keys %drive_config;  
    my @valid_tiers = grep {scalar(@{$drive_config{$_}}) >= $num_drives} @available_tiers;

    if (scalar(@valid_tiers) == 0) {
        platform_shim_test_case_throw_exception("There are not enough of any drive type to create a raid group");
    }
    
    my $selected_tier = $valid_tiers[int(rand(scalar(@valid_tiers)))];                                          
    my @disks = @{$drive_config{$selected_tier}};
    @disks = splice(@disks, 0, $num_drives);
    
    my $rg = platform_shim_raid_group_create($device, 
    										 (disks => \@disks,
                                              raid_type => 'r0'));
                                             
    $tc->platform_shim_test_case_log_debug('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' . platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');
    
    foreach my $disk (@disks) {
        $tc->platform_shim_test_case_log_debug("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
    }
    
    return $rg;
}



my %g_params_hash = (min_raid_groups => { type => SCALAR, default => '1' },
                               max_raid_groups => { type => SCALAR, default => '5' }, 
                               spares        => { type => SCALAR, default => 1 },
                               raid_types    => { type => ARRAYREF, default => \@g_raid_types},
                               disks => { type => ARRAYREF, optional => 1 },
                               vault => { type => SCALAR , default => 0 },
                               b_expand => { type => BOOLEAN , default => 1 },
                               b_zeroed_only => { type => BOOLEAN , default => 0 },
                               b_spindown_capable_only => { type => BOOLEAN, default => 0},
                      );


sub test_util_configuration_create_random_config
{
    my $tc = shift;
    my $device = $tc->{device};
    my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
                              
    my %disk_params = (b_zeroed_only => $params{b_zeroed_only},
                       b_spindown_capable_only => $params{b_spindown_capable_only});
                        
    if (defined $params{disks}) {
        $disk_params{disks} = $params{disks};
    }
    my %drive_config = test_util_configuration_fetch_disk_info($tc, \%disk_params);
    my @available_tiers = keys %drive_config;
    my %valid_widths = platform_shim_raid_group_get_raid_type_valid_widths();
    my $spares = $params{spares}; # number of disks we need to reserve for each raid group
    
    # start the process of creating a config
    my $num_raid_groups = $params{min_raid_groups} + int(rand($params{max_raid_groups} - $params{min_raid_groups}));
    my @raid_params = ();
    my @selected_raid_types = test_util_general_n_choose_k_array($params{raid_types}, $num_raid_groups);


    if ($params{vault})
    {
        my @system_disks = platform_shim_device_get_system_disk($device);
        my @sorted_system_disks = sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @system_disks;
        my @spare_disks = splice(@sorted_system_disks, scalar(@sorted_system_disks) - $spares , $spares);
        my $system_disk = $sorted_system_disks[$#sorted_system_disks];
        my $selected_tier = test_util_configuration_drive_type_to_tier(kind=> platform_shim_disk_get_property($system_disk, 'kind'), 
                                                                  technology => platform_shim_disk_get_property($system_disk, 'technology'));

        # If the system disk are 300GB or less then disable the use of system disks 
        my $system_disk_min_gb = platform_shim_disk_get_property($system_disk, "capacity") / 1000;  
        if (!platform_shim_device_is_unified($device)  &&
             ($system_disk_min_gb <= 300)                 ) {
            $tc->platform_shim_test_case_log_info("Rockies system disk capacity: $system_disk_min_gb GB too small for user rg");
            $params{vault} = 0;
        } else {
            #$tc->logInfo("$selected_tier size 1: ".scalar(@{$drive_config{$selected_tier}}));
            my @possible = grep {platform_shim_disk_get_property($_, "capacity") >= platform_shim_disk_get_property($system_disk, "capacity")} @{$drive_config{$selected_tier}};
            @possible = sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @possible;
            my $max_width = scalar(@sorted_system_disks) + scalar(@possible);
        
            my @valid_raid_types = grep {$_ ne 'r0' && @{$valid_widths{$_}}[0] <= $max_width} @selected_raid_types;
            my $vault_raid_type = $valid_raid_types[0];
            $tc->platform_shim_test_case_log_info("Selected vault raid type $vault_raid_type");
            @selected_raid_types = grep {$_ ne $vault_raid_type} @selected_raid_types;
            my $min_width = @{$valid_widths{$vault_raid_type}}[0];
            my @rg_disks = (); 
            if ($min_width <= scalar(@sorted_system_disks))
            {
                @rg_disks = splice(@sorted_system_disks, 0, $min_width);
            }
            else
            {
                my @disks_to_add = splice(@possible, 0, $min_width - scalar(@sorted_system_disks));
                @rg_disks = (@sorted_system_disks, @disks_to_add);
                my %disks_used = map {platform_shim_disk_get_property($_, "id") => 1} @disks_to_add;
                @{$drive_config{$selected_tier}} = grep {!defined $disks_used{platform_shim_disk_get_property($_, "id")}} @{$drive_config{$selected_tier}};
                #$tc->logInfo("$selected_tier size 2: ".scalar(@{$drive_config{$selected_tier}}));
            }
        
            $tc->platform_shim_test_case_log_info("Selected ".scalar(@rg_disks)." for vault rg");
            my %rg_params = (raid_type => $vault_raid_type,
                         disks => \@rg_disks, 
                         is_vault => 1);
            push @raid_params, \%rg_params;
        }
    }
  
    $tc->platform_shim_test_case_log_info("selected ".scalar(@selected_raid_types)." raid types");
    # first create the minimum config
    foreach my $raid_type (@selected_raid_types) 
    {
        my %rg_params = (raid_type => $raid_type);
        my @raid_type_widths = @{$valid_widths{$raid_type}};
        my $min_width = $raid_type_widths[0];
        my @valid_tiers = grep {scalar(@{$drive_config{$_}}) >= ($min_width + $spares)} @available_tiers;
        if (scalar(@valid_tiers) == 0)
        {            
            #platform_shim_test_case_throw_exception("Insufficient disks to create configuration. raid_type:$raid_type min_width:$min_width");
            $tc->platform_shim_test_case_log_info("Insufficient disks to create configuration. raid_type:$raid_type min_width:$min_width");
            next;
        }
        
        my $selected_tier = $valid_tiers[int(rand(scalar(@valid_tiers)))];       
        #$tc->logInfo("$selected_tier size 3: ".scalar(@{$drive_config{$selected_tier}}));
        my @disks = sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @{$drive_config{$selected_tier}};
        my @chosen_disks = splice(@disks, 0, $min_width);
        my @spare_disks = splice(@disks, scalar(@disks) - $spares , $spares);
        foreach my $spare (@spare_disks)
        {
            my $spare_id = platform_shim_disk_get_property($spare, 'id');
            $tc->platform_shim_test_case_log_info("Saving $spare_id for spare");
        }
        
        $tc->platform_shim_test_case_log_info("selected ".scalar(@chosen_disks)." min disks for $raid_type");
        @{$drive_config{$selected_tier}} = @disks;
        #$tc->logInfo("$selected_tier size 4: ".scalar(@{$drive_config{$selected_tier}}));
        
        $rg_params{disks} = \@chosen_disks;
        push @raid_params, \%rg_params;
    }
    
    # then randomly expand to wide widths
    if ($params{b_expand}) {
        my $b_expand = int(rand(2));
        foreach my $rg_param_ref (@raid_params) 
        {
            if ($rg_param_ref->{is_vault})
            {
                # We allow raid groups that contain vault drives to be expanded
                delete $rg_param_ref->{is_vault};  
                my @disks = @{$rg_param_ref->{disks}};
                my $technology = platform_shim_disk_get_property($disks[0], 'technology');
                my $kind = platform_shim_disk_get_property($disks[0], 'kind');
                my $tier = test_util_configuration_drive_type_to_tier(kind => $kind, 
                                                                      technology => $technology);
                my @possible = @{$drive_config{$tier}};
                my @tier_disks = sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @possible;
                my @raid_type_widths = @{$valid_widths{$rg_param_ref->{raid_type}}};
                my $target_disks = $raid_type_widths[scalar(@raid_type_widths) -1];
                $target_disks -= scalar(@disks);
                next if ($target_disks > scalar(@tier_disks));
                my $index = 0;
                foreach my $disk (@tier_disks)
                {
                    if (platform_shim_disk_get_property($disk, "capacity") >=  platform_shim_disk_get_property($disks[0], "capacity"))
                    {
                        last;
                    }
                    else 
                    {
                        $index++;
                    }
                }
                my $num_avail = scalar(@tier_disks) - $index;
                next if ($target_disks > $num_avail); 
                my @disks_to_add = splice(@tier_disks, $index, $target_disks);
                @{$drive_config{$tier}} = @tier_disks;
                push @disks, @disks_to_add;
                $tc->platform_shim_test_case_log_info("adding ".scalar(@disks_to_add)."  disks to vault rg ". $rg_param_ref->{raid_type});
                $rg_param_ref->{disks} = \@disks;
                $b_expand = 0;    
                
            } 
            elsif ($b_expand && $rg_param_ref->{raid_type} ne 'r10')
            {
                my @disks = @{$rg_param_ref->{disks}};
                my $technology = platform_shim_disk_get_property($disks[0], 'technology');
                my $kind = platform_shim_disk_get_property($disks[0], 'kind');
                my $tier = test_util_configuration_drive_type_to_tier(kind => $kind, 
                                                                      technology => $technology);
                my @tier_disks = @{$drive_config{$tier}};
                my $num_avail = scalar(@tier_disks);
                my @raid_type_widths = @{$valid_widths{$rg_param_ref->{raid_type}}};
                my $target_disks = $raid_type_widths[scalar(@raid_type_widths) -1];
                if ($num_avail && $num_avail>= $target_disks) 
                {
                    
                    my @disks_to_add = splice(@tier_disks, 0, $target_disks - scalar(@disks));
                    @{$drive_config{$tier}} = @tier_disks;
                    push @disks, @disks_to_add;
                    $tc->platform_shim_test_case_log_info("adding ".scalar(@disks_to_add)."  disks to rg ". $rg_param_ref->{raid_type});
                    $rg_param_ref->{disks} = \@disks;
                    $b_expand = 0;        
                }
            } 
            else 
            {
                $b_expand = 1;
            }
        }
    }
    
    my @rgs = ();
    foreach my $rg_param_ref (@raid_params) 
    {
        if (platform_shim_device_is_unified($device)) {
            $rg_param_ref->{raid_type} = $g_unity_raid_type_map{$rg_param_ref->{raid_type}};
        }
        my $rg = platform_shim_raid_group_create( $tc->{device}, $rg_param_ref);                                        
        $tc->platform_shim_test_case_log_info('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' . platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');  
        my @rg_disks = platform_shim_raid_group_get_disk($rg);
        foreach my $disk (@rg_disks) {
             $tc->platform_shim_test_case_log_info("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
        } 
        
        
        push @rgs, $rg;                   
    }

    # Must retrun at least one RG else there is nothing for the test case to test.
    if (scalar(@rgs) == 0)
    {
        $tc->platform_shim_test_case_log_info("Insufficient disks to create at least one configuration");
    }
    
    return \@rgs;
    
}

sub test_util_configuration_create_all_raid_types
{
     my $tc = shift;
     my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
     
     $params{min_raid_groups} = scalar(@g_raid_types);
     $params{max_raid_groups} = scalar(@g_raid_types);

     return test_util_configuration_create_random_config($tc, \%params);
}
sub test_util_configuration_create_all_raid_types_reserve
{
     my $tc = shift;
     my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
     
     $params{min_raid_groups} = scalar(@g_raid_types);
     $params{max_raid_groups} = scalar(@g_raid_types); 
     $params{spares} = 2; 
     
     return test_util_configuration_create_random_config($tc, \%params);
}
sub test_util_configuration_create_r1_only
{
     my $tc = shift;
     my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
     
     $params{min_raid_groups} = scalar(@g_mirror);
     $params{max_raid_groups} = scalar(@g_mirror);
     $params{vault} = 0;
     $params{raid_types} = \@g_mirror;
     return test_util_configuration_create_random_config($tc, \%params);
}
sub test_util_configuration_create_all_raid_types_with_system_drives
{
     my $tc = shift;
     my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
     
     $params{min_raid_groups} = scalar(@g_raid_types);
     $params{max_raid_groups} = scalar(@g_raid_types);
     $params{vault} = 1;

     return test_util_configuration_create_random_config($tc, \%params);
}

sub test_util_configuration_create_limited_raid_types_reserve
{
   my $tc = shift;

   # Pick R0, R10 and Randomly pick from R5, R3, R6
   my @raid_types = qw(r0 r10);
   my $rand_index = int(rand(scalar(@g_parity_raid_types)));
   my $parity_type = $g_parity_raid_types[$rand_index];
   push @raid_types, $parity_type;

   my %params = ( min_raid_groups => scalar(@raid_types),
                  max_raid_groups => scalar(@raid_types),
                  raid_types => \@raid_types,
                  #min_raid_groups => '2',
                  #max_raid_groups => '2', 
                  spares => 2);
   return test_util_configuration_create_random_config($tc, \%params);
}
sub test_util_configuration_create_all_parity_types_reserve
{
     my $tc = shift;
     my %params = ( min_raid_groups => scalar(@g_parity_raid_types),
                    max_raid_groups => scalar(@g_parity_raid_types),   
                   #min_raid_groups => '1',
                   #max_raid_groups => '1', 
                   spares => 2,
                   raid_types => \@g_parity_raid_types);
                   
     $tc->logInfo("create ".scalar(@g_parity_raid_types)." ".join(", ", @g_parity_raid_types));
     return test_util_configuration_create_random_config($tc, \%params);
}
sub test_util_configuration_create_limited_raid_types_with_system_drives
{
   my $tc = shift;

   # Pick R0, R10 and Randomly pick from R5, R3, R6
   my @raid_types = qw(r0 r10);
   my $rand_index = int(rand(scalar(@g_parity_raid_types)));
   my $parity_type = $g_parity_raid_types[$rand_index];
   push @raid_types, $parity_type;

   my %params = ( min_raid_groups => scalar(@raid_types),
                  max_raid_groups => scalar(@raid_types),
                  raid_types => \@raid_types,
                  #min_raid_groups => '2',
                  #max_raid_groups => '2', 
                  vault => 1);
   return test_util_configuration_create_random_config($tc, \%params);
}

sub test_util_configuration_create_one_random_raid_group
{
     my $tc = shift;
     my %params = platform_shim_test_case_validate(@_, \%g_params_hash);
     
     $params{min_raid_groups} = 1;
     $params{max_raid_groups} = 1;
     $params{spares} = 2;
     
     return test_util_configuration_create_random_config($tc, \%params);
}

sub test_util_configuration_create_raid_group_only_system_disks
{
    my $tc = shift;
    my %params = platform_shim_test_case_validate(@_,
                              {spares => { type => SCALAR , default => 1 }
                             });
    my $device = $tc->{device};

    my @valid_raid_types = qw(r5 r6 r10);
    my $raid_type = $valid_raid_types[int(rand(scalar(@valid_raid_types)))];
                   
    my @disks = platform_shim_device_get_system_disk($device);     
    
    if ($params{spares})
    {
        my %drive_config = test_util_configuration_fetch_disk_info($tc, {vault => 0});
        my $spares = $params{spares};
        my @disks = sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @disks;
        my $technology = platform_shim_disk_get_property($disks[$#disks], 'technology');
        my $kind = platform_shim_disk_get_property($disks[$#disks], 'kind');
        my $tier = test_util_configuration_drive_type_to_tier(kind => $kind, 
                                                                      technology => $technology);
        my @tier_disks =  sort {platform_shim_disk_get_property($a, "capacity") <=> platform_shim_disk_get_property($b, "capacity")} @{$drive_config{$tier}};
        
        if (scalar(@tier_disks) >= $spares &&
            (platform_shim_disk_get_property($tier_disks[scalar(@tier_disks) - $spares], "capacity") >=
             platform_shim_disk_get_property($disks[$#disks], "capacity")))
        {
            my @spare_disks = splice(@tier_disks, scalar(@tier_disks) - $spares , $spares);
            @{$drive_config{$tier}} = @tier_disks;     
            foreach my $spare (@spare_disks)
            {
                my $spare_id = platform_shim_disk_get_property($spare, 'id');
                $tc->platform_shim_test_case_log_info("Saving $spare_id for spare");
            }
        }
        else 
        {
            platform_shim_test_case_throw_exception("There are not enough valid spares for vault raid group");
        }
    }
    

    my $rg = platform_shim_raid_group_create( $tc->{device}, 
                                              {disks => \@disks, raid_type => $raid_type});                                        
    
    $tc->platform_shim_test_case_log_info('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' . platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');  

    foreach my $disk (@disks) {
        $tc->platform_shim_test_case_log_info("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
    } 
        
    return $rg;
}


sub test_util_configuration_get_fastest_disks
{
    my $tc = shift;
    my %params = platform_shim_test_case_validate(@_,
                              {num_disks        => { type => SCALAR, default => 5 },  
                               unzeroed_preferred => { type => BOOLEAN, default => 0 },
                              });
    
    
    my @chosen = (); 
    
    my %drive_config = test_util_configuration_fetch_disk_info($tc, {});
    my @tier_speed_order = ('FBE_PERFORMANCE_TIER_ZERO', 'FBE_PERFORMANCE_TIER_THREE', 
                            'FBE_PERFORMANCE_TIER_FIVE', 'FBE_PERFORMANCE_TIER_TWO',
                            'FBE_PERFORMANCE_TIER_ONE');
                            
    my $disk_info_ref = test_util_disk_get_pvd_info_hash($tc); 
                               
    foreach my $tier (@tier_speed_order)
    {
        # return array of sorted disks by speed
        if ($params{num_disks} eq 'all')
        {
            if ( $drive_config{$tier})
            {
                $tc->logInfo("$tier count: ".scalar(@{$drive_config{$tier}}));
                my @temp = sort {platform_shim_disk_get_property($a, 'capacity') <=> platform_shim_disk_get_property($b, 'capacity')} 
                                         @{$drive_config{$tier}};
                push @chosen, @temp;
            }
        }
        elsif ($drive_config{$tier} && 
            scalar(@{$drive_config{$tier}}) >= $params{num_disks}) 
        {
            if ($params{unzeroed_preferred})
            {
                $tc->platform_shim_test_case_log_info("before sort");
                
                my @temp = sort {$disk_info_ref->{platform_shim_disk_get_property($a, 'id')}{zero_percent} <=> $disk_info_ref->{platform_shim_disk_get_property($b, 'id')}{zero_percent}} 
                                             @{$drive_config{$tier}};
               $tc->platform_shim_test_case_log_info("after sort");
               
                @chosen = splice( @temp, 0, $params{num_disks});
                
                
            }
            else 
            {
                @chosen = splice( @{$drive_config{$tier}}, 0, $params{num_disks});
            }
            
            $tc->platform_shim_test_case_log_info("Selected tier $tier with disks");    
            map {$tc->platform_shim_test_case_log_info("Disk: ".platform_shim_disk_get_property($_, 'id'))} @chosen;
            return \@chosen;
        }
    }
    
    return \@chosen;

}

sub test_util_configuration_get_data_disks
{
     my $tc = shift;
     my $raid_group = shift;
     
     my $raid_type = platform_shim_raid_group_get_property($raid_group, 'raid_type');
     my $raid_width = platform_shim_raid_group_get_property($raid_group, 'raid_group_width');
     
     if ($raid_type =~ /^r1|r10$/)
     {
         return $raid_width/2;
     }
     
     if ($raid_type =~ /r6/)
     {
         return $raid_width - 2;
     }
     
     if ($raid_type =~ /r3|r5/)
     {
         return $raid_width - 1;
     }
     
     if ($raid_type =~ /r0|disk/)
     {
         return $raid_width;
     }
     
     $tc->platform_shim_test_case_throw_exception("invalid raid type? $raid_type $raid_width");
}


##############################################
# Filters and sorting for disk hash
##############################################

sub test_util_configuration_by_random_tier
{
    return sub {       
        return $_[int(rand(scalar(@_)))]; 
    };
    
}

sub test_util_configuration_by_faster_tier
{
    return sub {    
        
        my $match = $_[0];
        my %perf_tier = test_util_coniguration_get_performance_tier_rating();
        foreach my $tier (@_) {
            if ($perf_tier{$tier} < $perf_tier{$match})
            {
                $match = $tier;
            }
        }
        return $match;
    };
    
}

sub test_util_configuration_get_system_disks
{
    my $tc = shift;

	my $device = $tc->{device};
    my $num_system_drives = 0;
    my $last_system_drive_slot = 3;
    my @system_drives = ();

    $last_system_drive_slot = platform_shim_disk_get_last_system_drive_slot($tc);
    my %system_disk_params =(state => "unused",
                             vault => 1,
                             disk_count => ($last_system_drive_slot + 1),
                             buses => ['0_0']);
    @system_drives = platform_shim_device_get_disk($device, \%system_disk_params);
    foreach my $disk (@system_drives) {
        $tc->logInfo(" system disk: " . platform_shim_disk_get_property($disk, 'id'));
    }
    $num_system_drives = scalar(@system_drives);
    if ($num_system_drives < ($last_system_drive_slot + 1)) {
        platform_shim_test_case_throw_exception("Unexpected number of system drives: $num_system_drives");
    }
    return @system_drives;
}

sub test_util_configuration_recreate_raid_group_using_system_drives
{
	my $tc = shift;
	my $raid_group = shift;
    my %params = platform_shim_test_case_validate(@_,
                              {min_raid_groups => { type => SCALAR, default => '1' },
                               max_raid_groups => { type => SCALAR, default => '5' }, 
                               raid_type       => { type => SCALAR, default => 'random'}
                              });
                              
    my %drive_config = test_util_configuration_fetch_disk_info($tc, {update => 1});
    my $width = platform_shim_raid_group_get_property($raid_group, 'raid_group_width');
    my $raid_type = platform_shim_raid_group_get_property($raid_group, 'raid_type');
    my @system_disks = test_util_configuration_get_system_disks($tc);
    my @tier_disks = @system_disks;
    my $num_of_system_disks = scalar(@system_disks);
    my $system_disk_technology = platform_shim_disk_get_property($system_disks[0], 'technology');
    my $system_disk_kind = platform_shim_disk_get_property($system_disks[0], 'kind');
    my $additional_disk_required = 0;
    my @chosen_disks = ();
    my $smallest_system_drive_capacity = '0xFFFFFFFFFFFFFFFF';
    $smallest_system_drive_capacity = Math::BigInt->from_hex($smallest_system_drive_capacity);

    # Find the smallest system drive capacity
    foreach my $disk (@system_disks) {
        my $size = platform_shim_disk_get_property($disk, 'size');
        my $disk_capacity = Automatos::Util::Units::convertSize($size, 'BC');
        my $disk_id = platform_shim_disk_get_property($disk, 'id');
        $disk_capacity = Automatos::Util::Units::getNumber($disk_capacity);
        $disk_capacity = Math::BigInt->new($disk_capacity);
        #my $compare = Automatos::Util::Compare::lbaCmp($disk_capacity, $smallest_system_drive_capacity);
        if ($disk_capacity < $smallest_system_drive_capacity) {
            $tc->platform_shim_test_case_log_info("Disk: $disk_id has the smallest capacity: $disk_capacity");
            $smallest_system_drive_capacity = $disk_capacity;
        }
    }
    $tc->platform_shim_test_case_log_info("Smallest system disk capacity3: $smallest_system_drive_capacity");

    # We must have the `width' plus 1 drive for spare
    if (($width + 1) > $num_of_system_disks) {
        $additional_disk_required = ($width + 1) - $num_of_system_disks;
        $tc->platform_shim_test_case_log_info("Additional non-system disk required: $additional_disk_required");
        my $tier = test_util_configuration_drive_type_to_tier(kind => $system_disk_kind, 
                                                              technology => $system_disk_technology);
        my @tier_disks = @{$drive_config{$tier}};

        # The chosen disk must be at least the capacity of the smallest system disk
        my $num_avail = 0;
        foreach my $disk (@tier_disks) {
            my $size = platform_shim_disk_get_property($disk, 'size');
            my $disk_capacity = Automatos::Util::Units::convertSize($size, 'BC');
            my $disk_id = platform_shim_disk_get_property($disk, 'id');
            $disk_capacity = Automatos::Util::Units::getNumber($disk_capacity);
            $disk_capacity = Math::BigInt->new($disk_capacity);
            #my $compare = Automatos::Util::Compare::lbaCmp($disk_capacity, $smallest_system_drive_capacity);
            if ($disk_capacity >= $smallest_system_drive_capacity) {
                # The last required disk is reserved as a spare
                $num_avail++;
                if ($num_avail >= $additional_disk_required) {
                    last;
                }
                $tc->platform_shim_test_case_log_info("Adding: $disk_id with capacity: $disk_capacity to raid group with system disks");
                push @chosen_disks, $disk;
            }
        }
        if ($num_avail < $additional_disk_required) {
            # Simply leave the existing raid group in place.
            $tc->platform_shim_test_case_log_info("Re-bind using system drives tier: $tier requested: $additional_disk_required aval: $num_avail");
            return $raid_group;
        }
        foreach my $disk (@system_disks) {
            push @chosen_disks, $disk;
        }
    } else {
        @chosen_disks = splice(@system_disks, 0, $width);
    }     
    
    # Unbind the existing raid group
    platform_shim_raid_group_destroy($raid_group);
    
    # Create the raid group
    my $rg = platform_shim_raid_group_create( $tc->{device}, 
    										 {disks     => \@chosen_disks,
                                              raid_type => $raid_type});
    $tc->platform_shim_test_case_log_info('Created Raid Group ' . platform_shim_raid_group_get_property($rg, 'id') .
                   ' of type ' . platform_shim_raid_group_get_property($rg, 'raid_type') .
                   #' with size ' . platform_shim_raid_group_get_property($rg, 'raw_size') .
                   ' and disks ');  
    my @rg_disks = platform_shim_raid_group_get_disk($rg);
    foreach my $disk (@rg_disks) {
        $tc->platform_shim_test_case_log_info("   Disk ID: " . platform_shim_disk_get_property($disk, 'id'));
    }
    return $rg;
    
}


sub test_util_configuration_get_ndb_parameters
{
    my ($tc, $lun) = @_;
    my $device = $tc->{device};
    
    my $address_offset = platform_shim_lun_get_property($lun, 'address_offset');
    my $size_bc = platform_shim_lun_get_property($lun, "size");
    
    $tc->logInfo("ao: $address_offset $size_bc");
    
    my %params;
    if (platform_shim_device_is_unified($device)) 
    {
        my $c4_internal_id = platform_shim_lun_get_property($lun, "c4_internal_id");
        my $flare_lun_id = platform_shim_lun_get_property($lun, "id");
        my $uid = platform_shim_lun_get_property($lun, "uid");
        my $block_count = platform_shim_general_convert_size($size_bc, "BC");
        $block_count = platform_shim_general_get_number_from_size($block_count);
        
        $tc->logInfo("vnxe c4 $c4_internal_id flare $flare_lun_id uid $uid bc $block_count");
        
        %params = (c4_internal_id => $c4_internal_id,
                   flare_lun_id => $flare_lun_id, 
                   uid => $uid,
                   address_offset => $address_offset,
                   block_count => $block_count
                  );
        
    } 
    else
    {
        %params = (size => $size_bc, address_offset => $address_offset);
    }
    
    return %params;
    
}

sub test_util_configuration_get_per_lun_capacity {
   my ($self, $rg_config, $lun_capacity_gb, $luns_per_rg) = @_;

   my $rg_id = $rg_config->{raid_group_id};
   my $rg = $rg_config->{raid_group};
   # Initially we bind $luns_per_rg, but later we bind one more. Make sure we have capacity for all.
   my $requested_logical_capacity_gb = $lun_capacity_gb * $luns_per_rg;
   
   my $logical_capacity_available = platform_shim_raid_group_get_logical_capacity($self, $rg, $rg_id);
   my $logical_capacity_available_gb_bi = int($logical_capacity_available / 2097152);
   my $logical_capacity_available_gb = $logical_capacity_available_gb_bi->{value}->[0];

   # If the RG available space is not sufficient, then reduce the LUN capacities.
   if ($requested_logical_capacity_gb > $logical_capacity_available_gb) {
      my $available_lun_capacity_gb = int($logical_capacity_available_gb / $luns_per_rg);
      $self->platform_shim_test_case_log_info("Bind LUNs rg id: $rg_id capacity: $logical_capacity_available_gb GB" .
                                              " is less than desired: $requested_logical_capacity_gb GB");
      $self->platform_shim_test_case_log_info("          Reduce lun size from: $lun_capacity_gb GB" .
                                              " to: $available_lun_capacity_gb GB");
      $lun_capacity_gb = $available_lun_capacity_gb;
   }
   return $lun_capacity_gb;
}



1;
