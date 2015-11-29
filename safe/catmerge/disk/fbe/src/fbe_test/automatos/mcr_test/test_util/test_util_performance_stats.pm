package mcr_test::test_util::test_util_performance_stats;

use warnings;
use strict;

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
use mcr_test::platform_shim::platform_shim_host;
use mcr_test::platform_shim::platform_shim_enclosure;
use mcr_test::test_util::test_util_io;
use mcr_test::test_util::test_util_general;

use Params::Validate;

our $perfstats_timeout = 180;
our @lun_tags = ('lun_stpc', 'lun_wblk', 'lun_wiops');
our @pdo_tags = ('pdo_wblk', 'pdo_wiops');

sub test_util_performance_stat_get_lun_tags
{
    my $tc = shift;
    return @lun_tags;
}

sub test_util_performance_stat_set_lun_tags
{
    my $tc  = shift;
    @lun_tags = @_;
}

sub test_util_performance_stat_get_pdo_tags
{
    my $tc = shift;
    return @pdo_tags;
}
sub test_util_performance_stat_set_pdo_tags
{
    my $tc = shift;
     @pdo_tags = @_;
}


# Method: test_util_performance_stats_check_io_counters 
# Check IO counters are incrementing on the SPs.
# 
# Input:
# An optional hash specifying the following key/value pairs
# 
# slf_sp => $ - SP <Automatos::Component::Sp::Emc::Vnx> object.
#           If specified, the method will check that IO counters are incementing on peer SP.
# 
# Returns:
# *None*
# 
# Exceptions:
# *None*
#

sub test_util_performance_stats_check_io_counters 
{
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;

    my @luns = ();
    if ($params{luns})
    {
        @luns = @{$params{luns}};
    }
    else
    {
        my @raid_groups = platform_shim_raid_group_get_raidgroup($tc->{device});
        my @rg_luns = map {platform_shim_raid_group_get_lun($_)} @raid_groups;
        push @luns, @rg_luns;
    }

    if ($params{slf_sp}) {
        my @sps = platform_shim_device_get_sp($tc->{device});
        my $slf_sp_name = platform_shim_sp_get_property($params{slf_sp}, 'name');
        my ($active_sp) = grep {platform_shim_sp_get_property($_, 'name') ne $slf_sp_name} @sps; 
        my ($passive_sp) = grep {platform_shim_sp_get_property($_, 'name') eq $slf_sp_name} @sps; 
    
        # Check IO has started from SP A. Now we have to make sure IO is progressing through only 
        # active and not the passive
        test_util_performance_stats_check_lun_io_counter_status ($tc, { 'active_sp' => $active_sp,
                                                                       'passive_sp' => $passive_sp,
                                                                       'luns' => \@luns});
    }
    elsif ($params{rdgen_sp}) 
    {
        my @sps = platform_shim_device_get_sp($tc->{device});
        my $rdgen_sp_name = platform_shim_sp_get_property($params{rdgen_sp}, 'name');
        my ($active_sp) = grep {platform_shim_sp_get_property($_, 'name') eq $rdgen_sp_name} @sps; 
        my ($passive_sp) = grep {platform_shim_sp_get_property($_, 'name') ne $rdgen_sp_name} @sps; 
    
        # Check IO has started from SP A. Now we have to make sure IO is progressing through only 
        # active and not the passive
        test_util_performance_stats_check_lun_io_counter_status ($tc, { 'active_sp' => $active_sp,
                                                                       'passive_sp' => $passive_sp,
                                                                       'luns' => \@luns,
                                                                        'single_sp' => $params{single_sp}});                                                           
    }
    else
    {   # for iox case
        foreach my $lun (@luns)
        {
            # get the SP owner of LUN
            my $owning_sp = platform_shim_lun_get_property($lun, 'current_owner');
            my $active_sp = ($owning_sp =~ /A/i) ? $tc->{spa} : $tc->{spb};
            my $passive_sp = ($owning_sp =~ /A/i) ? $tc->{spb} : $tc->{spa};  
            # Check IO has started from SP A. Now we have to make sure IO is progressing through only 
            # active and not the passive
            test_util_performance_stats_check_lun_io_counter_status ($tc, { 'active_sp' => $active_sp,
                                                                            'passive_sp' => $passive_sp,
                                                                            'luns' => [$lun]});                                                              
        }
    }

    
}

#
# Method: test_util_performance_stats_check_io_counter_status
# Check IO counters are incrementing on primary sp and not on secondary sp.
#
#  Input:
# A hash specifying the following key/value pairs
# 
# active_sp => $ - SP <Automatos::Component::Sp::Emc::Vnx> object.
#                    IO counters increment will be checked on this SP
# passive_sp => $ - SP <Automatos::Component::Sp::Emc::Vnx> object.
#                    IO counters stopped will be checked on this SP
# luns => \@ - An array reference of <Automatos::Component::Lun:Emc::Vnx> objects
# 
# Returns:
# *None*
# 
# Exceptions:
# - <Automatos::Exception::Base>
#

sub test_util_performance_stats_check_lun_io_counter_status {
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;

    test_util_performance_stats_check_io_counters_incrementing($tc,
                                                    {'tag' => \@lun_tags,
                                                     'sp'  => $params{active_sp},
                                                     'luns' => $params{luns}});

    if (!$params{single_sp})
    {
        test_util_performance_stats_check_io_counters_stopped($tc, {'tag' => \@lun_tags,
                                                     'sp'  => $params{passive_sp},
                                                     'luns' => $params{luns}});
    }
    return;
}



# Method: test_util_performance_stats_check_io_counters_incrementing
# Check IO countres returned from 'perfstats' command are incrementing
# 
# Input:
# A hash specifying the following key/value pairs
# 
# tag => \@ - Array reference of tags for which IO counters to check
# sp   => $ - <Automatos::Component::Sp::Emc::Vnx> object.
# lun => $ - An <Automatos::Component::Lun:Emc::Vnx> object
# 
# Returns:
# *None*
# 
# Exceptions:
# - <Automatos::Exception::Base>
# 

sub test_util_performance_stats_check_io_counters_incrementing {
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;

    my %last_performance_stats = ();
    my $count = 1;
    my $wait_until = time() + $perfstats_timeout;
    my $is_all_incrementing = 1;
    
    $params{tag} ||= \@lun_tags;
    my %stats_hash = ('package' => 'sep',
                       tag => $params{tag});
    my @luns = @{$params{luns}};                       
    if (scalar(@luns) ==1 )
    {
        $stats_hash{lun} = $luns[0];
    }                       

    # Set SP through which 'perfstats' command needs to be executed.
    test_util_general_set_target_sp($tc, $params{sp});
    $tc->platform_shim_test_case_log_info("\tBelow IO counters should be incremented from ". uc (platform_shim_sp_get_property($params{sp},'name')) .
                   " (timeout is $perfstats_timeout seconds)");
    
    my %init_performance_stats = platform_shim_device_get_performance_statistics_summed($tc->{device}, \%stats_hash);

    test_util_performance_stats_print_io_counter( $tc, {'stat' => \%init_performance_stats,
                                                        'count' => $count,
                                                        'luns' => \@luns} );
                                                        
    #Increment of IO counters will be checked at max timeout specified by user
    while(time() < $wait_until) {
        sleep 5;
        $count++;       
        my %performance_stats = platform_shim_device_get_performance_statistics_summed($tc->{device}, \%stats_hash);

        test_util_performance_stats_print_io_counter( $tc, {'stat' => \%performance_stats,
                                                            'count' => $count, 
                                                            'luns' => \@luns} );

        $is_all_incrementing = 1;
        foreach my $lun (@luns) {
            my $id = platform_shim_lun_get_property($lun, "fbe_id");
            foreach my $io_count (sort keys %{$performance_stats{$id}}) {
                if ($performance_stats{$id}{$io_count} <=
                    $init_performance_stats{$id}{$io_count}) 
                {
                    $is_all_incrementing = 0;
                }
            }
        }
        
        if ($is_all_incrementing > 0 ) {
            last;
        } 
        
    }

    if ($is_all_incrementing == 0) {
        platform_shim_test_case_throw_exception('IO counters are not incrementing on ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    }
    else {
        $tc->platform_shim_test_case_log_info('IO counters are incrementing on ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    }
}

# Method: test_util_performance_stats_check_io_counters_stopped
# Check IO countres returned from 'perfstats' command are stopped
# 
# Input:
# A hash specifying the following key/value pairs
# 
# tag => \@ - Array reference of tags for which IO counters to check
# sp   => $ - <Automatos::Component::Sp::Emc::Vnx> object.
# lun => $ - An <Automatos::Component::Lun:Emc::Vnx> object
# 
# Returns:
# *None*
# 
# Exceptions:
# - <Automatos::Exception::Base>
#

sub test_util_performance_stats_check_io_counters_stopped {
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;

    my %last_performance_stats = ();
    my $io_increment = 0;
    $params{tag} ||= \@lun_tags;
    
    my %stats_hash = ('package' => 'sep',
                       tag => $params{tag});
    my @luns = @{$params{luns}};                       
    if (scalar(@luns) ==1 )
    {
        $stats_hash{lun} = $luns[0];
    }   

    # Set SP through which 'perfstats' command needs to be executed.
    test_util_general_set_target_sp($tc, $params{sp});

    $tc->platform_shim_test_case_log_info("\tBelow IO counters should not be incrementing on ". uc (platform_shim_sp_get_property($params{sp}, 'name')) .
                   " (timeout is $perfstats_timeout seconds)");
    my $count = 1;
    my $wait_until = time() + $perfstats_timeout;
    #Stop of IO counters will be checked at max timeout specified by user.
    while( time() < $wait_until ) {
        sleep 5;
        $io_increment = 0;
                          
        my %performance_stats = platform_shim_device_get_performance_statistics_summed($tc->{device}, \%stats_hash);

        test_util_performance_stats_print_io_counter($tc, { 'stat' => \%performance_stats,
                                                            'count' => $count,
                                                            'luns' => \@luns} );

        if ( $count >= 2 ) {
            foreach my $lun (@luns) {
                my $id = platform_shim_lun_get_property($lun, "fbe_id");
                if (exists $last_performance_stats{$id}) {
                    foreach my $io_count (sort keys %{$performance_stats{$id}}) {
                        if (exists $last_performance_stats{$id}{$io_count}) {
                            if ($performance_stats{$id}{$io_count} eq
                                $last_performance_stats{$id}{$io_count}) {
                            } else {
                                $io_increment++;
                            }
                        }
                    }
                }
            }

            if ($io_increment == 0) {
                last;
            }

        }
        $count++;

        %last_performance_stats = %performance_stats;
    }

    if ($io_increment > 0) {
        platform_shim_test_case_throw_exception('IO counters are still incrementing from ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    } else {
        $tc->platform_shim_test_case_log_info('IO counters are not incrementing on ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    }
}


# Method: test_util_io_print_io_counter
# Print the IO counters from perfstats command
# 
# Input:
# *None*
# 
# Returns:
# *None*
# 
# Exceptions:
# *None*
#

sub test_util_performance_stats_print_io_counter {
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;
    my $stats = $params{'stat'};
    my $count = $params{'count'};
    my $str = "";

    if ($params{disks} && scalar(@{$params{disks}}) > 1)
    {
        foreach my $disk (@{$params{disks}})
        {
            #my $fbe_id = platform_shim_disk_get_property($disk, "pdo_fbe_id");
            my $fbe_id = platform_shim_disk_get_property($disk, 'id');
            my $id = platform_shim_disk_get_property($disk, "id");
            $str.= "\t" x 5 ."DISK $id \n";
            foreach my $io_count (sort keys %{$$stats{$fbe_id}}) {
                $str .=  "\t" x 7 ."$io_count : $$stats{$fbe_id}{$io_count}\n";
            }
        }
    }
    elsif ($params{luns} && scalar(@{$params{luns}}) > 1)
    {
        foreach my $lun (@{$params{luns}})
        {
            my $id = platform_shim_lun_get_property($lun, "id");
            my $fbe_id = platform_shim_lun_get_property($lun, "fbe_id");
            $str.= "\t" x 5 ."LUN $id fbe_id: $fbe_id\n";
            foreach my $io_count (sort keys %{$$stats{$fbe_id}}) {
                $str .=  "\t" x 7 ."$io_count : $$stats{$fbe_id}{$io_count}\n";
            }
        }
    }
    else {
        foreach my $id (sort keys %{$stats}) {
            $str.= "\t" x 5 ."Object $id\n";
            foreach my $io_count (sort keys %{$$stats{$id}}) {
                $str .=  "\t" x 7 ."$io_count : $$stats{$id}{$io_count}\n";
            }
        }
    }

    $tc->platform_shim_test_case_log_info("\t\t" . 'Count '. $count . "\n" . $str);

}


sub test_util_performance_stats_check_disk_io_counters 
{
    my ($tc, $params_ref) = @_;
    my %params = %$params_ref;

    my @disks = @{$params{disks}};

    if ($params{slf_sp}) {
        my @sps = platform_shim_device_get_sp($tc->{device});
        my $slf_sp_name = platform_shim_sp_get_property($params{slf_sp}, 'name');
        my ($active_sp) = grep {platform_shim_sp_get_property($_, 'name') ne $slf_sp_name} @sps; 
        my ($passive_sp) = grep {platform_shim_sp_get_property($_, 'name') eq $slf_sp_name} @sps; 
    
        # Check IO has started from SP A. Now we have to make sure IO is progressing through only 
        # active and not the passive
       test_util_performance_stats_check_disk_io_counters_incrementing ($tc, { 'sp' => $active_sp,
                                                                               'disks' => \@disks, 
                                                                               'tag' => \@pdo_tags});
    }
    elsif ($params{rdgen_sp})
    {
        my @sps = platform_shim_device_get_sp($tc->{device});
        my $rdgen_sp_name = platform_shim_sp_get_property($params{rdgen_sp}, 'name');
        my ($active_sp) = grep {platform_shim_sp_get_property($_, 'name') eq $rdgen_sp_name} @sps; 
        my ($passive_sp) = grep {platform_shim_sp_get_property($_, 'name') ne $rdgen_sp_name} @sps; 
    
        # Check IO has started from SP A. Now we have to make sure IO is progressing through only 
        # active and not the passive
        test_util_performance_stats_check_disk_io_counters_incrementing ($tc, { 'sp' => $active_sp,
                                                                               'disks' => \@disks, 
                                                                               'tag' => \@pdo_tags});
        
        test_util_performance_stats_check_disk_io_counters_stopped ($tc, { 'sp' => $passive_sp,
                                                                           'disks' => \@disks,
                                                                           'tag' => \@pdo_tags});
    }
    else 
    {

        my %hash = ();
        my @sps = platform_shim_device_get_sp($tc->{device});
        foreach my $sp (@sps)
        {
            my $sp_name = lc(platform_shim_sp_get_property($sp, "name"));
            $hash{$sp_name}{sp} = $sp;
            $hash{$sp_name}{disks} = [];
            ($hash{$sp_name}{passive_sp}) = grep {platform_shim_sp_get_property($_, "name") ne $sp_name} @sps;
        }
        
        my @spa_disks = ();
        my @spb_disks = ();
        
        foreach my $disk (@disks)
        {
            
            # get the SP owner of PVD
            my $active_sp = platform_shim_device_get_active_sp($tc->{device}, {object=> $disk});
            my $active_sp_name = platform_shim_sp_get_property($active_sp, "name");
            push @{$hash{$active_sp_name}{disks}}, $disk;                                                 
        }
        
        foreach my $sp_name (keys %hash)
        {
            if (scalar(@{$hash{$sp_name}{disks}}))
            {
                test_util_performance_stats_check_disk_io_counters_incrementing ($tc, { 'sp' => $hash{$sp_name}{sp},
                                                                                        'disks' => $hash{$sp_name}{disks},
                                                                                        'tag' => \@pdo_tags});
                test_util_performance_stats_check_disk_io_counters_stopped ($tc, { 'sp' => $hash{$sp_name}{passive_sp},
                                                                                   'disks' => $hash{$sp_name}{disks},
                                                                                   'tag' => \@pdo_tags});
            }
        }
      

    }

    
}

# params: tags, sp, disks
sub test_util_performance_stats_check_disk_io_counters_incrementing
{
    my ($tc, $params_ref) = @_;
    
    my %params = %$params_ref;
    my @disks = @{$params_ref->{disks}};
    my %last_perf_stats = ();
    my $count = 1;
    my $wait_until = time() + $perfstats_timeout;
    my $io_increment = 1;
    $params_ref->{tag} ||= \@pdo_tags;
    my %stats_hash = (tag => $params_ref->{tag});
    if (scalar(@disks) == 1)
    {
        $stats_hash{disk} = $disks[0];
    }
    
    $tc->platform_shim_test_case_log_info("\tBelow IO counters should be incremented from ". uc (platform_shim_sp_get_property($params{sp},'name')) .
                   " (timeout is $perfstats_timeout seconds)");
    # Set SP through which 'perfstats' command needs to be executed.
    test_util_general_set_target_sp($tc, $params{sp});
    my %init_perf_stats = %{test_util_performance_stats_get_disk_performance_stats($tc, \%stats_hash)};
    test_util_performance_stats_print_io_counter($tc, { 'stat' => \%init_perf_stats,
                                                        'count' => $count,
                                                        'disks' => \@disks} );

    
    while( time() < $wait_until ) {
        $io_increment = 1;
        $count++;
        my $perf_stats = test_util_performance_stats_get_disk_performance_stats($tc, \%stats_hash);
        test_util_performance_stats_print_io_counter($tc, { 'stat' => $perf_stats,
                               'count' => $count,
                                                        'disks' => \@disks} );

        foreach my $disk (@disks)
        {
            #my $pdo_fbe_id = platform_shim_disk_get_property($disk, 'pdo_fbe_id');
            my $pdo_fbe_id = platform_shim_disk_get_property($disk, 'id');
            foreach my $io_count (sort keys %{$perf_stats->{$pdo_fbe_id}}) {
                if ($perf_stats->{$pdo_fbe_id}{$io_count} <=
                    $init_perf_stats{$pdo_fbe_id}{$io_count}) 
                {
                    $io_increment = 0;
                } 
            }
        }
                
        if ($io_increment > 0) {
            last;
        }
        
 
        sleep(2);
    }
    
     if ($io_increment == 0) {
        platform_shim_test_case_throw_exception('IO counters are not incrementing from ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    } else {
        $tc->platform_shim_test_case_log_info('IO counters are incrementing on ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    }
   
}

# params: tags, sp, disks
sub test_util_performance_stats_check_disk_io_counters_stopped
{
    my ($tc, $params_ref) = @_;
    
    my %params = %$params_ref;
    my @disks = @{$params{disks}};
    my %last_perf_stats = ();
    $params_ref->{tag} ||= \@pdo_tags;
    my %stats_hash = (tag => $params_ref->{tag});
    
    if (scalar(@disks) == 1)
    {
        $stats_hash{disk} = $disks[0];
    }
    
    # Set SP through which 'perfstats' command needs to be executed.
    test_util_general_set_target_sp($tc, $params{sp});
    $tc->platform_shim_test_case_log_info("\tBelow IO counters should not be incrementing on ". uc (platform_shim_sp_get_property($params{sp}, 'name')) .
                   " (timeout is $perfstats_timeout seconds)");

    my $count = 1;
    my $io_increment = 0;
    my $wait_until = time() + $perfstats_timeout;
    while( time() < $wait_until ) {
        $io_increment = 0;
        my $perf_stats = test_util_performance_stats_get_disk_performance_stats($tc, \%stats_hash);
        test_util_performance_stats_print_io_counter($tc, { 'stat' => $perf_stats,
                               'count' => $count, 
                                'disks' => \@disks} );
        if ($count >= 2) {
            foreach my $disk (@disks)
            {
                my $id = platform_shim_disk_get_property($disk, 'id');
                if (exists $last_perf_stats{$id}) {
                    foreach my $io_count (sort keys %{$perf_stats->{$id}}) {
                        if (exists $last_perf_stats{$id}{$io_count}) {
                            if ($perf_stats->{$id}{$io_count} eq
                                $last_perf_stats{$id}{$io_count}) 
                            {
                                    
                            } else {
                                    $io_increment++;
                            }
                        }
                    }
                }
    
            }
                
            if ($io_increment == 0) {
                    last;
            }
        }
        $count++;
        %last_perf_stats = %$perf_stats;
        sleep(2);
    }
    
     if ($io_increment > 0) {
        platform_shim_test_case_throw_exception('IO counters are still incrementing from ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    } else {
        $tc->platform_shim_test_case_log_info('IO counters are not incrementing on ' . uc(platform_shim_sp_get_property($params{sp}, 'name')));
    }
   
}

sub test_util_performance_stats_get_disk_performance_stats
{
    my ($tc, $params_ref) = @_;
    
    my %stats_hash = %$params_ref;
    $stats_hash{package} = 'pp';
    $stats_hash{by_location} = 1;
                          
    my %performance_stats = platform_shim_device_get_performance_statistics_summed($tc->{device}, \%stats_hash);
    return \%performance_stats;
    
}


sub test_util_performance_stats_enable_sep_pp
{
    my $tc = shift;
  
    # Enable performance statistics on both SPs
    foreach my $sp (platform_shim_device_get_sp($tc->{device})){
        test_util_general_set_target_sp($tc, $sp);
        platform_shim_device_set_performance_statistics($tc->{device}, {state => 1, package => 'sep'});
        platform_shim_device_set_performance_statistics($tc->{device}, {state => 1, package => 'pp'});
        platform_shim_device_clear_performance_statistics($tc->{device}, {});  
        $tc->platform_shim_test_case_log_info('Performance statistics Enabled on ' . uc (platform_shim_sp_get_property($sp, 'name')));
    }
}

sub test_util_performance_stats_disable_sep_pp
{
    my $tc = shift;
  
    # Enable performance statistics on both SPs
    foreach my $sp (platform_shim_device_get_sp($tc->{device})){
        test_util_general_set_target_sp($tc, $sp);
        platform_shim_device_set_performance_statistics($tc->{device}, {state => 0, package => 'sep'});
        platform_shim_device_set_performance_statistics($tc->{device}, {state => 0, package => 'pp'});
       
        $tc->platform_shim_test_case_log_info('Performance statistics Disabled on ' . uc (platform_shim_sp_get_property($sp, 'name')));
    }
}


1;
