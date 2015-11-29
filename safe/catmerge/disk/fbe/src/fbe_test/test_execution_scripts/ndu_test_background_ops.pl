#
#####################################################################
# ndu_test_background_ops.pl                                        #
#                                                                   #
# 1) First this test unbinds any existing raid groups and then      #
#    binds new raid groups of each raid type.                       #
#    (see g_raid_types below):                                      #
#    Index Type     Background operations performed                 #
#    ----- -------  --------------------------------                #
#       0. RAID-0   - SLF, Paged metadata verify                    #
#       1. RAID-1   - SLF, Copy                                     #
#       2. RAID-10  - SLF, Copy                                     #
#       3. RAID-3   - SLF, Paged metadata verify                    #
#       4. RAID-10  - SLF, Rebuild                                  #
#       5. RAID-5   - SLF, Rebuild                                  #
#       6. RAID-6   - SLF, Rebuild                                  #
#      -----------                                                  #
#       7 - Total raid groups                                       #
#                                                                   #
# 2) Then I/O is started on the active SP.                          #
#                                                                   #
# 3) Then the following background operations are `started' (in the #
#    following order):                                              #
#       o SLF - Pull (1) drive on active SP                         #
#       o Paged metadata verify - Pull sufficient drive to go broken#
#       o Copy - Start either proactive or user copy                #
#       o Rebuild - Remove (1) drive and spare swaps in             #
#                                                                   #
# 4) Start NDU with a large `delay'.  The delay is executed before  #
#    the active SP is rebooted (i.e. still running old code).       #
#                                                                   #
# 5) Verify that the test complete:                                 #
#       o SLF - No I/O issues with I/O enabled. Re-insert on active #
#       o Paged metadata verify - Re-insert all drives wait for rdy #
#       o Copy - Wait for copy to complete                          #
#       o Rebuild - Wait for rebuild to complete                    #
#                                                                   #
# 6) Wait for NDU to complete                                       #
#                                                                   #
# 7) Stop I/O and validate no errors                                #
#                                                                   #
# Author: Ron Proulx                                                #
# Last updated: 10/01/2014                                          #
#                                                                   #
#####################################################################
our $invalid_position = 255;
our $invalid_disk = "255_255_255";
our $trigger_spare_short = 10;
our $trigger_spare_default = 300;
our $default_delay_before_primary_reboot = 360;
our $extended_delay_before_primary_reboot = 86400; # (60 * 60 * 24) - 1 day
our @g_raid_types = qw(r0 r1 r1_0 r3 r1_0 r5 r6);
our $choice = 0;
if ($choice != 99)
{
    my ($self) = @_;
    my %test = ();
    my $status = 0;

    # Get the array information
    ndu_get_params(\%test);

    # Get the operation
    ndu_help($test);

    while ($choice != 99) {
        print("\n\nEnter your choice (1 - 14) (50: Help, 99: Exit)):");
        $choice = <STDIN>;
        chomp($choice);
        $test{operation} = $choice;
        print("operation: $test{operation} \n");
    
        # Create raid groups and luns (destroy as required)
        if ($test{operation} == 1 || $test{operation} == 2) {
            # Create raid groups and LUNs
            $status = ndu_create_rgs_and_luns(\%test);
            if ($status != 0) {
                # @note We do NOT exit on failure
                ndu_log_error(\%test, $status);
            }
        }

        # Create storage group and export to host
        if ($test{operation} == 1 || $test{operation} == 3) {
            if ($test{b_rg_created} == 0) {
                print("Must create raid groups (option: 1 or 2) before starting I/O\n");
            } else {
                # Create storage group
                $status = ndu_create_storage_group(\%test);
                if ($status != 0) {
                    ndu_log_error(\%test, $status);
                }
            }
        }

        # Start I/O
        if ($test{operation} == 1 || $test{operation} == 4) {
            if ($test{b_rg_created} == 0) {
                print("Must create raid groups (option: 1 or 2) before starting I/O\n");
            } else {
                # Start the io
                $status = ndu_start_io(\%test);
                if ($status != 0) {
                    ndu_log_error(\%test, $status);
                }
            }
        }

        # Start the pre-NDU tests (does not wait for completion)
        if ($test{operation} == 1 || $test{operation} == 5) {
            if ($test{b_rg_created} == 0) {
                print("Must create raid groups (option: 1 or 2) before starting tests\n");
            } else {
                # Start the tests
                $status = ndu_start_pre_ndu_tests(\%test);
                if ($status != 0) {
                    ndu_log_error(\%test, $status);
                }
            }
        }

        # Start the NDU (does not wait for completion)
        if ($test{operation} == 1 || $test{operation} == 6) {
            # Start the NDU
            $status = ndu_start_ndu(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }


        # Get the NDU status
        if ($test{operation} == 7) {
            # Get the NDU status
            $status = ndu_get_ndu_status(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }

        # Start the during-NDU tests (does not wait for completion)
        if ($test{operation} == 1 || $test{operation} == 8) {
            if ($test{b_rg_created} == 0) {
                print("Must create raid groups (option: 1 or 2) before starting tests\n");
            } else {
                # Start the tests
                $status = ndu_start_during_ndu_tests(\%test);
                if ($status != 0) {
                    ndu_log_error(\%test, $status);
                }
            }
        }

        # Bring any raid groups that were purposefully broken back
        if ($test{operation} == 1 || $test{operation} == 9) {
            if ($test{b_rg_created} == 0) {
                print("Must create raid groups (option: 1 or 2) before starting tests\n");
            } else {
                # Bring broken raid groups back 
                $status = ndu_bring_broken_rgs_online(\%test);
                if ($status != 0) {
                    ndu_log_error(\%test, $status);
                }
            }
        }

        # Complete NDU
        if ($test{operation} == 1 || $test{operation} == 10) {
            # Complete (commit) NDU
            $status = ndu_complete_ndu(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }

        # Complete (i.e. wait for test to complete) the tests
        if ($test{operation} == 1 || $test{operation} == 11) {
            # Complete the tests
            $status = ndu_complete_tests(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }

        # Stop I/O
        if ($test{operation} == 1 || $test{operation} == 12) {
            # Stop I/O
            $status = ndu_stop_io(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }

        # Unbind luns and destroy raid groups
        if ($test{operation} == 1 || $test{operation} == 13) {
            # Unbind luns
            $status = ndu_destroy_luns_and_rgs(\%test);
            if ($status != 0) {
                ndu_log_error(\%test, $status);
            }
        }

        # Display debug info
        if ($test{operation} == 14) {
             ndu_display_debug_info(\%test);
        }

        # Help
        if ($test{operation} == 50) {
            ndu_help($test);
        }

    }

    exit(0);
} 

sub ndu_help 
{
    my ($test) = @_;
    print("\n\n");
    print("***********************************************\n");
    print("*            NDU Test Tool                    *\n");
    print("*                                             *\n");
    print("*     1: Execute ALL (bind, test, NDU, check) *\n");
    print("*     2: Execute bind only                    *\n");
    print("*     3: Execute create storage group only    *\n");
    print("*     4: Execute start I/O                    *\n");
    print("*     5: Execute start pre-NDU tests          *\n");
    print("*     6: Execute start NDU only               *\n");
    print("*     7: Get the NDU status                   *\n");
    print("*     8: Execute start during-NDU tests       *\n");
    print("*     9: Bring broken raid groups online      *\n");
    print("*    10: Execute complete NDU only            *\n");
    print("*    11: Execute complete test only           *\n");
    print("*    12: Execute stop I/O only                *\n");
    print("*    13: Unbind, destroy raid groups          *\n");
    print("*    14: Display debug information            *\n");
    print("*    50: Help                                 *\n");
    print("*    99: Exit                                 *\n");
    print("*                                             *\n");
    print("***********************************************\n");
}

sub ndu_get_params 
{
    my ($test_ref) = @_;
     
    print("Enter target SPA: ");
    my $spa = <STDIN>;
    chomp($spa);
    # @note `special' characters must be proceeded by \
    my @spb_subnets = split /\./, $spa;
    my $num_subnets = scalar(@spb_subnets);
    my $subnet_index = 0;
    my $spb = "";
    my $luns_per_rg = 32;
    #print(" $spa $num_subnets \n");
    $spb_subnets[($num_subnets - 1)] += 1;
    while ($subnet_index < $num_subnets) {
        $spb = $spb . $spb_subnets[$subnet_index];
        $subnet_index++;
        if ($subnet_index < $num_subnets) {
            $spb = $spb . ".";
        }
    }
    $test_ref->{spa} = $spa; 
    #print("target spa: $spa $test_ref->{spa} \n");  
    $test_ref->{spb} = $spb; 
    #print("target spb: $spb $test_ref->{spb} \n"); 
    
    print("\nIs this the Active SP (1=Yes, 0=No)?: ");                              
    my $b_is_spa_active = <STDIN>;
    chomp($b_is_spa_active);
    if ($b_is_spa_active) {
        $test_ref->{active_sp} = $spa;
        $test_ref->{active_sp_name} = "SPA";
        $test_ref->{passive_sp} = $spb;
        $test_ref->{passive_sp_name} = "SPB";
    } else {
        $test_ref->{active_sp} = $spb;
        $test_ref->{active_sp_name} = "SPB";
        $test_ref->{passive_sp} = $spa;
        $test_ref->{passive_sp_name} = "SPA";
    }

    print("\nEnter the array username: ");                              
    my $username = <STDIN>;
    chomp($username);
    $test_ref->{username} = $username;

    print("\nEnter the array password: ");                              
    my $password = <STDIN>;
    chomp($password);
    $test_ref->{password} = $password;

    print("Enter host IP: ");
    my $host_ip = <STDIN>;
    chomp($host_ip);
    $test_ref->{host_ip} = $host_ip;

    print("Enter host name: ");
    my $host_name = <STDIN>;
    chomp($host_name);
    $test_ref->{host_name} = $host_name;

    print("Enter the number of luns per rg (default: 32): ");
    my $luns_per_rg = <STDIN>;
    chomp($luns_per_rg);
    $test_ref->{luns_per_rg} = $luns_per_rg;

    # Initialize any other test values
    $test_ref->{ndu_delay_before_primary_reboot} = $default_delay_before_primary_reboot;
    $test_ref->{seconds_before_primary_is_rebooted} = $default_delay_before_primary_reboot;
    $test_ref->{b_rg_created} = 0;
    $test_ref->{b_io_started} = 0;
    $test_ref->{b_ndu_started} = 0;
    $test_ref->{rgs_ref} = ();
}

sub ndu_log_info 
{
    my ($msg) = @_;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time());
    $mon += 1;
    $year += 1900;
    my $Time_Stamp =" ${mon}/${mday}/${year} ${hour}:${min}:$sec. ";

    print $Time_Stamp . "INFO " . $msg . "\n";
}

sub ndu_display_rg_info
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $b_display_rg_info = 0;

    # Only if configured 
    if (!$test{b_rg_created}) {
        ndu_log_info("ndu_display_rg_info: raid groups never created");
        return;
    }

    # Display the raid group info
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $width = $rg->{width};
        my @disks = @{$rg->{disks}};
        my @luns = @{$rg->{luns}};

        # Only first line is log info
        ndu_log_info("ndu_display_rg_info: rg id: $rg_id raid type: $raid_type width: $width user capacity gb: $rg->{user_capacity_gb} num luns: $rg->{luns_per_rg}");

        # Display the disks
        print("Disks($rg->{drive_type}): ");
        my $position = 0;
        foreach my $disk (@disks) {
            print("$position:$disk ");
            $position++;
        }
        print("\n");

        # Display the luns
        print("LUNs: ");
        foreach my $lun (@luns) {
            print("$lun ")
        }
        print("\n");

        # Display the test parameters fo each test
        my $disk1 = $invalid_disk;
        my $disk2 = $invalid_disk;
        my $disk3 = $invalid_disk;
        print(" Verify Test: enabled: $rg->{b_test_verify_enabled} started: $rg->{b_test_verify_started} \n");
        print(" SLF Test: enabled: $rg->{b_test_slf_enabled} started: $rg->{b_test_slf_started} \n");
        if ($rg->{test_slf_position} != $invalid_position) {
            $disk1 = $disks[$rg->{test_slf_position}];
        }
        print("\t SLF position: $rg->{test_slf_position} disk: $disk1 \n");
        print(" Copy Test: enabled: $rg->{b_test_copy_enabled} started: $rg->{b_test_copy_started} \n");
        if ($rg->{test_copy_position} != $invalid_position) {
            $disk1 = $disks[$rg->{test_copy_position}];
        } else {
            $disk1 = $invalid_disk;
        }
        print("\t copy position: $rg->{test_copy_position} source disk: $disk1 \n");
        print(" Rebuild Test: enabled: $rg->{b_test_rebuild_enabled} started: $rg->{b_test_rebuild_started} \n");
        if ($rg->{test_rebuild_position_1} != $invalid_position) {
            $disk1 = $disks[$rg->{test_rebuild_position_1}];
        } else {
            $disk1 = $invalid_disk;
        }
        print("\t rebuild position1: $rg->{test_rebuild_position_1} disk1: $disk1 \n");
        if ($rg->{test_rebuild_position_2} != $invalid_position) {
            $disk2 = $disks[$rg->{test_rebuild_position_2}];
        } else {
            $disk2 = $invalid_disk;
        }
        print("\t rebuild position2: $rg->{test_rebuild_position_2} disk2: $disk2 \n");

        print(" Paged Verify Test: enabled: $rg->{b_test_paged_enabled} started: $rg->{b_test_paged_started} \n");
        if ($rg->{test_paged_position_1} != $invalid_position) {
            $disk1 = $disks[$rg->{test_paged_position_1}];
        } else {
            $disk1 = $invalid_disk;
        }
        print("\t paged position1: $rg->{test_paged_position_1} disk1: $disk1 \n");
        if ($rg->{test_paged_position_2} != $invalid_position) {
            $disk2 = $disks[$rg->{test_paged_position_2}];
        } else {
            $disk2 = $invalid_disk;
        }
        print("\t paged position2: $rg->{test_paged_position_2} disk2: $disk2 \n");
        if ($rg->{test_paged_position_3} != $invalid_position) {
            $disk3 = $disks[$rg->{test_paged_position_3}];
        } else {
            $disk3 = $invalid_disk;
        }
        print("\t paged position3: $rg->{test_paged_position_3} disk3: $disk3 \n");

        # Done with this riad group
        print("\n");
    }

    return;
}
#end ndu_display_rg_info

sub ndu_display_debug_info
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $status = 0;

    # Display all the debug info
    ndu_log_info("ndu_display_debug_info: Active SP: $test{active_sp_name} $test{active_sp}  Passive SP: $test{passive_sp_name} $test{passive_sp}");
    print("\t username: $test{username} password: $test{password}\n");
    print("\t host name: $test{host_name} ip: $test{host_ip} \n");
    print("\t rg created: $test{b_rg_created} io started: $test{b_io_started} ndu started: $test{b_ndu_started}\n");

    if ($test{b_rg_created}) {
        my $b_display_rg_info = 0;
        print("\t storage group: $test{storagegroup_name} \n");
        print("Do you want to display raid group info? (1=Yes: 0=No): ");
        $b_display_rg_info = <STDIN>;
        chomp($b_display_rg_info);
        if ($b_display_rg_info) {
            ndu_display_rg_info($test_ref);
        }
    }
    if ($test{b_io_started}) {
        print("\t I/O started on host: $test{host_name} ip: $test{host_ip}\n");
    }
    if ($test{b_ndu_started}) {
        print("\t NDU to: $test{to_package_name} delay: $test{ndu_delay_before_primary_reboot} seconds before primary reboot: $test{seconds_before_primary_is_rebooted} \n");
    }

    # Return $status
    return $status;
}
#end ndu_display_debug_info

sub ndu_log_error
{
    my ($test_ref, $status) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $function = $test{function};
    my $b_display_debug_info = 0;
    my $b_exit = 0;

    ndu_log_info("ndu_log_error: $function failed with status: $status");

    # Check if should dump debug info
    print("\n\nDo you want to display the debug info? (Yes=1 No=0):");
    $b_display_debug_info = <STDIN>;
    chomp($b_display_debug_info);

    # Display the debug info
    if ($b_display_debug_info) {
        ndu_display_debug_info($test_ref);
    }

    # Check if they want to stop
    print("\n\nDo you want to exit? (Yes=1 No=0):");
    $b_exit = <STDIN>;
    chomp($b_exit);
    if ($b_exit) {
        exit($status);
    }

    return;
}
#end ndu_log_error

sub ndu_check_response 
{
    my ($function, $request, $response_ref) = @_;
    my @response = @{$response_ref};
    my $line_index = 0;
    my $status = 0;

    ndu_log_info("request: $request");
    foreach $line (@response) {
        if ($line_index == 0) {
            ndu_log_info("response: $line ");
        }
        if (($line =~ m/(failed)\s*(.*)/)        ||
            ($line =~ m/(not)\s*(.*)/)           ||
            ($line =~ m/(rror occurred)\s*(.*)/) ||
            ($line =~ m/(usage)\s*(.*)/)            ) {
            ndu_log_info("$function: Failed: $line  ");
            return 1;
        }
        $line_index++;
    }

    return $status;
}
#end ndu_check_response

sub ndu_display_response 
{
    my ($response_ref) = @_;
    my @response = @{$response_ref};

    foreach $line (@response) {
        print("$line");
    }

    return $status;
}
#end ndu_display_response

sub ndu_check_ndu_status 
{
    my ($function, $request, $response_ref) = @_;
    my @response = @{$response_ref};
    my $line_index = 0;
    my $status = 1;

    ndu_log_info("request: $request");
    foreach $line (@response) {
        if ($line_index == 0) {
            ndu_log_info("response: $line ");
        }
        if ($line =~ m/(failed)\s*(.*)/) {
            ndu_log_info("$function: Failed: $line  ");
            return 2;
        }
        if ($line =~ m/(completed)\s*(.*)/) {
            ndu_log_info("$function: Complete: $line  ");
            return 0;
        }
        $line_index++;
    }

    return $status;
}
#end ndu_check_ndu_status

sub ndu_destroy_luns_and_rgs 
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    ndu_log_info("ndu_destroy_luns_and_rgs: target sp: $target_sp ");
    my $username = $test{username};
    my $password = $test{password};
    my $host_name = $test{host_name};
    my $status = 0;
    $test->{function} = "ndu_destroy_luns_and_rgs";

    # Remove from storage group
    my @storagegroup_list = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list`;
    my @storagegroup = "";
    my @hlu_list = "";
    foreach $line (@storagegroup_list) {
        if ($line =~ m/(Storage Group Name:)\s*(.*)/) {
            ndu_log_info("Add storagegroup: $2 ");
            push @storagegroup, $2;
        }
    }
    foreach $storagegroup (@storagegroup) {
        my @parse_hlu = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list -gname $storagegroup`;
        my $b_hlu_header_found = 0;
        my $b_parse_luns = 0;
        foreach $line (@parse_hlu) {
            if ($line =~ m/(HLU Number     ALU Number)\s*(.*)/) {
                $b_hlu_header_found = 1;
            } elsif ($line =~ m/(----------     ----------)\s*(.*)/) {
                if ($b_hlu_header_found) {
                    $b_parse_luns = 1;
                }
            } elsif ($line =~ m/(Shareable:)\s*(.*)/) {
                last;
            } elsif ($b_parse_luns) {
                $line =~ m/(    )\s*(.*)/;
                my @values = split ' ', $line;
                my $hlu = $values[0];
                ndu_log_info("Add hlu: $hlu ");
                push @hlu_list, $hlu;
            }
        }
        foreach $hlu (@hlu_list) {
            if ($hlu ne "") {
                ndu_log_info("Remove hlu: $hlu from: $storagegroup");
                `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -removehlu -gname $storagegroup -hlu $hlu -o`;
            }
        }
    }
    # Remove luns from storage pool
    my @storagepool_list = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagepool -list`;
    my @storagepool = "";
    my @lun_list = "";
    foreach $line (@storagepool_list) {
        if ($line =~ m/(Pool ID:)\s*(.*)/) {
            ndu_log_info("Add storagepool: $2 ");
            push @storagepool, $2;
        }
    }
    foreach $storagepool (@storagepool) {
        @parse_lun = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagepool -list -id $storagepool`;
        @luns = "";
        foreach $line (@parse_lun) {
            if ($line =~ m/(LUNs:)\s*(.*)/) {
                @lun_list = split ', ', $line;
                my @lun0 = split ' ', $lun_list[0];
                ndu_log_info("lun0: $lun0[1]");
                $lun_list[0] = $lun0[1];
                $num_luns = scalar(@lun_list);
                ndu_log_info("luns found: $num_luns first lun: $lun_list[0]");
                last;
            }
        }
        foreach $lun (@lun_list) {
            if ($lun ne "") {
                ndu_log_info("Remove lun: $lun from: $storagepool");
                `naviseccli -user $username -password $password -scope 0 -h $target_sp lun -destroy -l $lun -destroySnapshots -forceDetach -o`;
            }
        }
    }

    # Remove storage pools
    foreach $storagepool (@storagepool) {
        if ($storagepool ne "") {
            ndu_log_info("Remove storagepool: $storagepool ");
                `naviseccli -user $username -password $password -scope 0 -h $target_sp storagepool -destroy -id $storagepool -o`;
                sleep(30);
            }
    }

    my @unbind_array = "";
    my @lun_list = "";

    @lun_list = `naviseccli -user $username -password $password -scope 0 -h $target_sp getlun -name`; 
    #@lun_list = `navicli -h $target_sp getlun -name`;

    foreach $line (@lun_list)                                                   # loop thru list
    {
        if ($line =~ m/(LOGICAL UNIT NUMBER)\s*(.*)/)                           # parse only lun number
        {
            if ($3 =~ /223/)                                                    # don't unbind 223!
            {}
            
            else
            {
                unshift(@unbind_array, $2);                                     # store others in an array
            }
        }
    }

    foreach $lun (@unbind_array)
    {
        if ($lun ne "")
        {
            ndu_log_info("Unbinding LUN #$lun");                                     # unbind the LUNs in the array
                
               `naviseccli -user $username -password $password -scope 0 -h $target_sp unbind $lun -o`;
         #`navicli -h $target_sp unbind $lun -o`;
        }
    }

    # Get the list of groups
    my @group_list = `naviseccli -user $username -password $password -scope 0 -h $target_sp getrg -type`;                                   

    foreach $rg (@group_list)                                                   # loop thru list
    {
        if ($rg =~ m/(RaidGroup ID:)\s*(.*)/)                                   # parse only RAID group ID
        {
            if ($2 =~ /223/)                                                    # don't unbind 223!
            {}
             
            else
            {
               print ("\nDestroying Group #$2\n");
               `naviseccli -user $username -password $password -scope 0 -h $target_sp removerg $2`;
               #`navicli -h $target_sp removerg $2`;
            }
        }
    }
    
    # Now destroy storage groups
    foreach $storagegroup (@storagegroup) {
        if ($storagegroup ne "") {
            ndu_log_info("Disconnect host: $host_name from storagegroup: $storagegroup ");
            `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -disconnecthost -host $host_name -gname $storagegroup -o`;
            ndu_log_info("Destroy storagegroup: $storagegroup ");
            `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -destroy -gname $storagegroup -o`;
        }
    }

    return $status;
}
# end ndu_destroy_luns_and_rgs

sub ndu_create_rgs
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    ndu_log_info("ndu_create_rgs: target sp: $target_sp ");
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = ();
    my @selected_raid_types = @g_raid_types;
    my $function = "ndu_create_rgs";
    my $status = 0;
   
    # @note Search for `# disks:' to change the disks for each raid group.
    # first create the minimum config
    my $rg_index = 0;
    foreach my $raid_type (@selected_raid_types) {
        my %rg = ();
        my @disks = ();
        my @spare_disks = ();
        my @removed_disks = ();
        my $request = 0;
        my @response = ();
        my $width = 0;
        
        # Set the default value for all the test parameters for this raid group
        $rg{b_test_verify_enabled} = 0;
        $rg{b_test_verify_started} = 0;
        $rg{b_test_slf_enabled} = 0;
        $rg{b_test_slf_started} = 0;
        $rg{test_slf_position} = $invalid_position;
        $rg{b_test_copy_enabled} = 0;
        $rg{b_test_copy_started} = 0;
        $rg{test_copy_position} = $invalid_position;
        $rg{b_test_rebuild_enabled} = 0;
        $rg{b_test_rebuild_started} = 0;
        $rg{test_rebuild_position_1} = $invalid_position;
        $rg{test_rebuild_position_2} = $invalid_position;
        $rg{b_test_paged_enabled} = 0;
        $rg{b_test_paged_started} = 0;
        $rg{test_paged_position_1} = $invalid_position;
        $rg{test_paged_position_2} = $invalid_position;
        $rg{test_paged_position_3} = $invalid_position;

        # Based on index
        if ($rg_index == 0) {
            # Create id: 0 raid_type: RAID-0 width: 8 spare: 1_0_A8
            $width = 8;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 536;
            # Configure test parameters for RAID-0 width 8. tests:
            #   0-r0-test 1: SLF            - test position 1
            #   0-r0-test 2: paged metadata - test position 2
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_paged_enabled} = 1;
            $rg{test_slf_position} = int(rand($width));
            $rg{test_paged_position_1} = ($width - 1);
            if ($rg{test_slf_position} == ($width - 1)) {
                $rg{test_paged_position_1} = 0;
            }
            # disks:    pos 0  pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(1_0_A0 1_0_A1 1_0_A2 1_0_A3 1_0_A4 1_0_A5 1_0_A6 1_0_A7);
            @spare_disks = qw(1_0_A8);
            # @todo Yes this could be made common.
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                       " $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type`; 
        } elsif ($rg_index == 1) {
            # Create id: 1 raid_type: RAID-1 width: 2 spare: 1_0_A10
            $width = 2;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 268;
            # Configure test parameters for RAID-1 width 2. tests:
            #   1-r1-test 1: verify         - test position: n/a
            #   1-r1-test 2: copy           - test position 1
            #   1-r1-test 3: SLF            - test position 2
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_copy_enabled} = 1;
            $rg{test_copy_position} = int(rand($width));
            $rg{test_slf_position} = ($width - 1);
            if ($rg{test_copy_position} == ($width - 1)) {
                $rg{test_slf_position} = 0;
            }
            # disks:    pos 0 pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(0_0_0 1_0_A9);
            @spare_disks = qw(1_0_A10);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                       " $disks[0] $disks[1] -raidtype $raid_type";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] -raidtype $raid_type`;
        } elsif ($rg_index == 2) {
            # Create id: 2 raid_type: RAID-10 width: 4 spare: 0_0_14
            $width = 4;
            $rg{drive_type} = 'SATA FLASH';
            $rg{user_capacity_gb} = 183;
            # Configure test parameters for RAID-10 width 4. tests:
            #   2-r1_0-test 1: verify       - test position: n/a
            #   2-r1_0-test 2: SLF          - test position 1
            #   2-r1_0-test 3: rebuild      - test position 2
            #   2-r1_0-test 4: paged        - test position 2
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_rebuild_enabled} = 1;
            $rg{b_test_paged_enabled} = 1;
            $rg{test_rebuild_position_1} = int(rand($width));
            $rg{test_slf_position} = ($width - 1);
            if ($rg{test_rebuild_position_1} == ($width - 1)) {
                $rg{test_slf_position} = 0;
            }
            $rg{test_paged_position_1} = 0;
            $rg{test_paged_position_2} = 2;
            # disks:    pos 0  pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(0_0_10 0_0_11 0_0_12 0_0_13);
            @spare_disks = qw(0_0_14);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                        " $disks[0] $disks[1] $disks[2] $disks[3] -raidtype $raid_type";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] -raidtype $raid_type`;
        } elsif ($rg_index == 3) {
            # Create id: 3 raid_type: RAID-3 width: 9 spare: 1_0_B8
            $width = 9;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 536;
            # Configure test parameters for RAID-3 width 9. tests:
            #   3-r3-test 1: verify     - test position: n/a
            #   3-r3-test 2: SLF        - test position 1
            #   3-r3-test 3: rebuild    - test position 2
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_rebuild_enabled} = 1;
            $rg{test_rebuild_position_1} = int(rand($width));
            $rg{test_slf_position} = ($width - 1);
            if ($rg{test_rebuild_position_1} == ($width - 1)) {
                $rg{test_slf_position} = 0;
            }
            # disks:    pos 0   pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(1_0_A11 1_0_B0 1_0_B1 1_0_B2 1_0_B3 1_0_B4 1_0_B5 1_0_B6 1_0_B7);
            @spare_disks = qw(1_0_B8);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                       " $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] $disks[8] -raidtype $raid_type"; 
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] $disks[8] -raidtype $raid_type`;
        } elsif ($rg_index == 4) {
            # Create id: 4 raid_type: RAID-10 width: 8 spare: 1_0_C8
            $width = 8;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 1073;
            # Configure test parameters for RAID-10 width 8. tests:
            #   4-r1_0-test 1: verify   - test position: n/a
            #   4-r1_0-test 2: SLF      - test position 1
            #   4-r1_0-test 3: rebuild  - test position 2
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_rebuild_enabled} = 1;
            $rg{test_rebuild_position_1} = int(rand($width));
            $rg{test_slf_position} = ($width - 1);
            if ($rg{test_rebuild_position_1} == ($width - 1)) {
                $rg{test_slf_position} = 0;
            }
            # disks:    pos 0  pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(1_0_C0 1_0_C1 1_0_C2 1_0_C3 1_0_C4 1_0_C5 1_0_C6 1_0_C7);
            @spare_disks = qw(1_0_C8);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                        " $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type"; 
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type`;
            # @note If creating the raid group violates the sparing policy you
            #       need to uncomment the following line (i.e. allow the raid group
            #       to be created even if it violates the sparing policy).
            #`y';
        } elsif ($rg_index == 5) {
            # Create id: 5 raid_type: RAID-5 width: 5 spare: 1_0_D4
            $width = 5;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 268;
            # Configure test parameters for RAID-5 width 5. tests:
            #   5-r5-test 1: verify   - test position: n/a
            #   5-r5-test 2: SLF      - test position 1
            #   5-r5-test 3: copy     - test position 2
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_copy_enabled} = 1;
            $rg{test_copy_position} = int(rand($width));
            $rg{test_slf_position} = ($width - 1);
            if ($rg{test_copy_position} == ($width - 1)) {
                $rg{test_slf_position} = 0;
            }
            # disks:    pos 0  pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(0_0_1  1_0_D0 1_0_D1 1_0_D2 1_0_D3);
            @spare_disks = qw(1_0_D4);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                        " $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] -raidtype $raid_type";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] -raidtype $raid_type`;
        } elsif ($rg_index == 6) {
            # Create id: 6 raid_type: RAID-6 width: 8 spare: 0_1_10 0_1_11
            $width = 8;
            $rg{drive_type} = 'SAS';
            $rg{user_capacity_gb} = 268;
            # Configure test parameters for RAID-6 width 8. tests:
            #   6-r6-test 1: verify   - test position: n/a
            #   6-r6-test 2: SLF      - test position 1
            #   6-r6-test 3: copy     - test position 2
            #   6-r6-test 4: rebuild  - test position 3 and 4
            $rg{b_test_verify_enabled} = 1;
            $rg{b_test_slf_enabled} = 1;
            $rg{b_test_copy_enabled} = 1;
            $rg{b_test_rebuild_enabled} = 1;
            $rg{test_copy_position} = 0;
            $rg{test_rebuild_position_1} = 1;
            $rg{test_rebuild_position_2} = ($width - 2);
            $rg{test_slf_position} = ($width - 1);
            # disks:    pos 0  pos 1  pos 2  pos 3  pos 4  pos 5  pos 6  pos 7  pos 8  pos 9  pos A  pos B  pos C  pos D  pos E  pos F
            @disks = qw(0_1_2 0_1_3 0_1_4 0_1_5 0_1_6 0_1_7 0_1_8 0_1_9);
            @spare_disks = qw(0_1_10 0_1_11);
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index" .
                       " $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp createrg $rg_index $disks[0] $disks[1] $disks[2] $disks[3] $disks[4] $disks[5] $disks[6] $disks[7] -raidtype $raid_type`;
        } else {
            ndu_log_info("ndu_create_rgs: Unexpected rg index: $rg_index ");
            return 1;
        }

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 2;
        }

        # Now configure the raid group
        $rg{id} = $rg_index;
        $rg{raid_type} = $raid_type;
        $rg{width} = $width;
        my $num_disks = scalar(@disks);
        if ($width != $num_disks) {
            ndu_log_info("ndu_create_rgs: rg id: $rg_index num disks: $num_disks doesnt match width: $width");
            return 3;
        }
        $rg{disks} = \@disks;
        $rg{spare_disks} = \@spare_disks;

        $rg{removed_disks_handle} = \@removed_drives;
        push @rgs, \%rg;   

        # Print some info
        ndu_log_info("ndu_create_rgs: created rg index: $rg_index id: $rg{id} type: $rg{raid_type} width: $rg{width} ");
        ndu_log_info("ndu_create_rgs: disks: ");
        foreach my $disk (@disks) {
            print("$disk ")
        }
        print("\n");

        # Goto next
        $rg_index++;
    }

    # Set the address of the rgs
    $test_ref->{b_rg_created} = 1;
    $test_ref->{rgs_ref} = \@rgs;

    #Return success
    return $status;
}

sub ndu_bind_luns 
{
    my ($test_ref, $num_luns_per_rg) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $function = "ndu_bind_luns";
    my $status = 0;
    my $lun_index = 0;

    # Binds the specified luns with the specified capacity
    ndu_log_info("ndu_bind_luns: target sp: $target_sp first rg id: $rgs[0]{id}");
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $user_capacity_gb = $rg->{user_capacity_gb};
        my $lun_capacity_gb = int($user_capacity_gb / $num_luns_per_rg);
        my @luns = ();
        my $request = 0;
        my @response = ();

        # Bind the lun with both read and write cache enabled (the customer default)
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp bind $raid_type" .
                   " $lun_index -rg $rg_id -sq gb -cap $lun_capacity_gb -cnt $num_luns_per_rg -sp a -aa 1 -rc 1 -wc 1";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp bind $raid_type $lun_index -rg $rg_id -sq gb -cap $lun_capacity_gb -cnt $num_luns_per_rg -sp a -aa 1 -rc 1 -wc 1`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 2;
        }
        ndu_log_info("ndu_bind_luns: Bound $num_luns_per_rg luns on rg id: $rg_id raid type: $raid_type ");
        foreach (1..$num_luns_per_rg) {
            my $lun = $lun_index;
            push @luns, $lun;
            ndu_log_info("ndu_bind_luns: added lun: $lun to rg id: $rg_id");
            $lun_index++;
        }
        # @note Must use the address when setting values
        $rg->{luns_per_rg} = $num_luns_per_rg;
        $rg->{luns} = \@luns;
    }

    # Return status
    return $status;
}

sub ndu_create_storage_group
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $host_ip = $test{host_ip};
    my $host_name = $test{host_name};
    my @rgs = @{$test{rgs_ref}};
    my $storagegroup_name = "NDU_Test_Storagegroup";
    my $b_storagegroup_exist = 0;
    my $request = 0;
    my @response = ();
    my $function = "ndu_create_storage_group";
    my $status = 0;
    my $lun_index = 0;
    my $b_set_host_success = 0;

    # First check if the storage group exists
    ndu_log_info("ndu_create_storage_group: target sp: $target_sp create storage group: $storagegroup_name");
    my @storagegroup_list = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list`;
    my @storagegroup = "";
    my @hlu_list = "";
    foreach $line (@storagegroup_list) {
        if ($line =~ m/(Storage Group Name:)\s*(.*)/) {
            ndu_log_info("storagegroup: $2 ");
            if ($2 eq $storagegroup_name) {
                $b_storagegroup_exist = 1;
                last;
            }
        }
    }
    # If needed create
    if (!$b_storagegroup_exist) {
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -create -gname $storagegroup_name";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -create -gname $storagegroup_name`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 2;
        }
    }

    # Add the requested luns to the storage group
    $test_ref->{storagegroup_name} = $storagegroup_name;
    ndu_log_info("ndu_create_storage_group: target sp: $target_sp first rg id: $rgs[0]{id}");
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my @luns = @{$rg->{luns}};
        my $num_luns = scalar(@luns);

        # Add all the luns in the raid groups
        ndu_log_info("ndu_create_storage_group: add: $num_luns from rg id: $rg_id to storagegroup: $storagegroup_name");
        foreach my $lun (@luns) {
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -addhlu -o -gname" .
                       " $storagegroup_name -hlu $lun -alu $lun";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -addhlu -o -gname $storagegroup_name -hlu $lun -alu $lun`;

            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 3;
            }
        }
    }

    # Add the host to the storage group
    # May need to reboot host
    while (!$b_set_host_success) {
        my $b_retry = 0;
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -sethost -ip $host_ip -o";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -sethost -ip $host_ip -o`;
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            print("ndu_create_storage_group: sethost failed. You may need to reboot host. Continue?(1=Yes 0=No): ");
            my $b_retry = <STDIN>;
            chomp($b_retry);
            if (!$b_retry) {
                return 4;
            }
            print("ndu_create_storage_group: sethost failed. Enter 1 when host has been rebooted: ");
            my $b_retry = <STDIN>;
            chomp($b_retry);
        } else {
            $b_set_host_success = 1;
        }
    }

    # Now connect the host
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -connecthost -o -host $host_name" .
               " -gname $storagegroup_name";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -connecthost -o -host $host_name -gname $storagegroup_name`;
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 5;
    }
    
    # Display the storage group
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 6;
    }
    ndu_display_response(\@response);

    # Add the requested luns to the storage group
    $test_ref->{storagegroup_name} = $storagegroup_name;
    ndu_log_info("ndu_create_storage_group: target sp: $target_sp first rg id: $rgs[0]{id}");
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my @luns = @{$rg->{luns}};
        my $num_luns = scalar(@luns);

        # Add all the luns in the raid groups
        ndu_log_info("ndu_create_storage_group: add: $num_luns from rg id: $rg_id to storagegroup: $storagegroup_name");
        foreach my $lun (@luns) {
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -addhlu -o -gname" .
                       " $storagegroup_name -hlu $lun -alu $lun";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -addhlu -o -gname $storagegroup_name -hlu $lun -alu $lun`;

            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 3;
            }
        }
    }

    # Add the host to the storage group
    # May need to reboot host
    while (!$b_set_host_success) {
        my $b_retry = 0;
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -sethost -ip $host_ip -o";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -sethost -ip $host_ip -o`;
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            print("ndu_create_storage_group: sethost failed. You may need to reboot host. Continue?(1=Yes 0=No): ");
            my $b_retry = <STDIN>;
            chomp($b_retry);
            if (!$b_retry) {
                return 4;
            }
            print("ndu_create_storage_group: sethost failed. Enter 1 when host has been rebooted: ");
            my $b_retry = <STDIN>;
            chomp($b_retry);
        } else {
            $b_set_host_success = 1;
        }
    }

    # Now connect the host
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -connecthost -o -host $host_name" .
               " -gname $storagegroup_name";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -connecthost -o -host $host_name -gname $storagegroup_name`;
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 5;
    }
    
    # Display the storage group
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp storagegroup -list`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 6;
    }
    ndu_display_response(\@response);

    # Return status
    return $status;
}

sub ndu_create_rgs_and_luns
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    # @note Change the capacity and number of luns here
    my $lun_capacity_gb = 10;
    my $num_luns_per_rg = 4;
    my $status = 0;
    $test->{function} = "ndu_create_rgs_and_luns";

    # Debug 
    ndu_log_info("target spb: $test{spb} ");  
    ndu_log_info("username: $test{username} ");  
    ndu_log_info("password: $test{password} ");  

    # First remove any existing rgs and luns
    $status = ndu_destroy_luns_and_rgs($test_ref);
    if ($status != 0) {
        return $status;
    }

    # Now create the raid groups
    $status = ndu_create_rgs($test_ref);
    if ($status != 0) {
        return $status;
    }

    # Now bind the LUNs
    $status = ndu_bind_luns($test_ref, $test{luns_per_rg});
    if ($status != 0) {
        return $status;
    }

    # Return the $status
    return $status;
}

sub ndu_get_ndu_reboot_delay
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $target_sp_name = $test{active_sp_name};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $function = "ndu_get_ndu_reboot_delay";
    my $current_ndu_delay = $test{ndu_delay_before_primary_reboot};
    my $time_until_delay_complete = $test{seconds_before_primary_is_rebooted};
    my $status = 0;

    # Get the current delay
    #T:\ADE\contrib>naviseccli -h10.245.191.18 -user clariion1992 -password clariion1992 -scope 0 ndu -getdelay
    #NDU Delay Interval Time:      800000 secs.
    #Elapsed Time:                   120 secs.
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -getdelay";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -getdelay`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 1;
    }

    # Now locate the value 
    foreach $line (@response) {
        if ($line =~ m/(NDU Delay Interval Time:)\s*(.*)/) {
            ndu_log_info("ndu_get_ndu_reboot_delay: NDU Delay: $2");
            $current_ndu_delay = $2;
        } elsif ($line =~ m/(Elapsed Time:)\s*(.*)/) {
            ndu_log_info("ndu_get_ndu_reboot_delay: NDU Delay Time Elapsed: $2");
            if ($2 > 0) {
                $time_until_delay_complete = $current_ndu_delay - $2;
                ndu_log_info("ndu_get_ndu_reboot_delay: Seconds until: $target_sp_name is rebooted: $time_until_delay_complete");
            }
            last;
        }
    }

    # Now set the test values
    if ($current_ndu_delay != $test{ndu_delay_before_primary_reboot}) {
        $test_ref->{ndu_delay_before_primary_reboot} = $current_ndu_delay;
    }
    if ($time_until_delay_complete != $test{seconds_before_primary_is_rebooted}) {
        $test_ref->{seconds_before_primary_is_rebooted} = $time_until_delay_complete;
    }

    # Return the $status
    return $status;
}
#end ndu_get_ndu_reboot_delay

sub ndu_set_ndu_reboot_delay
{
    my ($test_ref, $delay_before_primary_reboot) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $delay_before_status_check = 10;
    my $function = "ndu_set_ndu_reboot_delay";
    my $status = 0;

    # First get the current values
    $status = ndu_get_ndu_reboot_delay($test_ref);
    if ($status != 0) {
        return 1;
    }
    ndu_log_info("Changed NDU delay from: $test{ndu_delay_before_primary_reboot} to: $delay_before_primary_reboot");

    # Set the delay
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -setdelay $delay_before_primary_reboot -o";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -setdelay $delay_before_primary_reboot -o`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 2;
    }
    $test_ref->{ndu_delay_before_primary_reboot} = $delay_before_primary_reboot;

    # Return the $status
    return $status;
}
#end ndu_set_ndu_reboot_delay

sub ndu_start_ndu 
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $delay_before_status_check = 10;
    my $function = "ndu_start_ndu";
    my $status = 0;
    $test->{function} = "ndu_start_ndu";

    ndu_log_info("Enter package name w/o extension: ");
    my $to_package_name = <STDIN>;
    chomp($to_package_name);
    $test_ref->{to_package_name} = $to_package_name;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 1;
    }

    # Now locate the value 
    foreach $line (@response) {
        if ($line =~ m/(NDU Delay Interval Time:)\s*(.*)/) {
            ndu_log_info("ndu_get_ndu_reboot_delay: NDU Delay: $2");
            $current_ndu_delay = $2;
        } elsif ($line =~ m/(Elapsed Time:)\s*(.*)/) {
            ndu_log_info("ndu_get_ndu_reboot_delay: NDU Delay Time Elapsed: $2");
            if ($2 > 0) {
                $time_until_delay_complete = $current_ndu_delay - $2;
                ndu_log_info("ndu_get_ndu_reboot_delay: Seconds until: $target_sp_name is rebooted: $time_until_delay_complete");
            }
            last;
        }
    }

    # Now set the test values
    if ($current_ndu_delay != $test{ndu_delay_before_primary_reboot}) {
        $test_ref->{ndu_delay_before_primary_reboot} = $current_ndu_delay;
    }
    if ($time_until_delay_complete != $test{seconds_before_primary_is_rebooted}) {
        $test_ref->{seconds_before_primary_is_rebooted} = $time_until_delay_complete;
    }

    # Return the $status
    return $status;
}
#end ndu_get_ndu_reboot_delay

sub ndu_set_ndu_reboot_delay
{
    my ($test_ref, $delay_before_primary_reboot) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $delay_before_status_check = 10;
    my $function = "ndu_set_ndu_reboot_delay";
    my $status = 0;

    # First get the current values
    $status = ndu_get_ndu_reboot_delay($test_ref);
    if ($status != 0) {
        return 1;
    }
    ndu_log_info("Changed NDU delay from: $test{ndu_delay_before_primary_reboot} to: $delay_before_primary_reboot");

    # Set the delay
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -setdelay $delay_before_primary_reboot -o";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -setdelay $delay_before_primary_reboot -o`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 2;
    }
    $test_ref->{ndu_delay_before_primary_reboot} = $delay_before_primary_reboot;

    # Return the $status
    return $status;
}
#end ndu_set_ndu_reboot_delay

sub ndu_start_ndu 
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $delay_before_status_check = 10;
    my $function = "ndu_start_ndu";
    my $status = 0;
    $test->{function} = "ndu_start_ndu";

    ndu_log_info("Enter package name w/o extension: ");
    my $to_package_name = <STDIN>;
    chomp($to_package_name);
    $test_ref->{to_package_name} = $to_package_name;

    # Start the NDU
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -install $to_package_name" . ".pbu" .
               " -force -skiprules";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -install $to_package_name.pbu -force -skiprules`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 1;
    }
    $test_ref->{b_ndu_started} = 1;
    ndu_log_info("ndu_start_ndu: NDU of: $to_package_name started. Delay: $delay_before_status_check then check status");

    # Check the status
    sleep($delay_before_status_check);
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 2;
    }
    ndu_log_info("ndu_start_ndu: NDU of: $to_package_name check status: SUCCESS");


    # @note MUST Start the NDU prior to setting the delay !!!
    # Set the delay before the primary is rebooted
    ndu_log_info("Enter delay in seconds before rebooting primary (default: 86400 seconds(1 day)): ");
    $extended_delay_before_primary_reboot = <STDIN>;
    chomp($extended_delay_before_primary_reboot);
    $status = ndu_set_ndu_reboot_delay($test_ref, $extended_delay_before_primary_reboot);
    if ($status != 0) {
        return 3;
    }

    # Return the $status
    return $status;
}
#end ndu_start_ndu

sub ndu_get_ndu_status 
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $function = "ndu_get_ndu_status";
    my $delay_before_status_check = 10;
    my $status = 0;
    $test->{function} = "ndu_get_ndu_status";

    # First check connectivity
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp getagent";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp getagent`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 1;
    }

    # Get the delay before the primary is rebooted
    $status = ndu_get_ndu_reboot_delay($test_ref);
    if ($status != 0) {
        return 2;
    }

    # Check if they want to sleep (why?)
    ndu_log_info("Enter delay in seconds before checking NDU status: ");
    $delay_before_status_check = <STDIN>;
    chomp($delay_before_status_check);
    ndu_log_info("ndu_get_ndu_status: Sleeping: $delay_before_status_check before checking status");
    if ($delay_before_status_check > 0) {
        sleep($delay_before_status_check);
        ndu_log_info("ndu_get_ndu_status: Delay after NDU start complete");
    }

    # Check the status
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 3;
    }

    # Return the $status
    return $status;
}
#end ndu_get_ndu_status

sub ndu_complete_ndu
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{passive_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $to_package_name = $test{to_package_name};
    my $b_wait_for_ndu_to_complete = 1;
    my $delay_before_status_check = 10;
    my $function = "ndu_complete_ndu";
    my $b_ndu_complete = 0;
    my $status = 0;
    $test->{function} = "ndu_complete_ndu";

    # Check if they want to wait for the NDU to complete
    ndu_log_info("Do you want to wait for the NDU to complete (1=Yes 0=No (package will not be committed): ");
    $b_wait_for_ndu_to_complete = <STDIN>;
    chomp($b_wait_for_ndu_to_complete);
    if (!$b_wait_for_ndu_to_complete) {
        ndu_log_info("ndu_complete_ndu: WARNING skipping wait for NDU complete");
        return $status;
    }

    # Time between NDU status checks
    ndu_log_info("Enter delay in seconds between each check for NDU complete: ");
    $delay_before_status_check = <STDIN>;
    chomp($delay_before_status_check);

    # Check the status until complete or user aborts
    while (!$b_ndu_complete) {
        # Get the NDU status on the passive
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status`;

        # Check the response
        $status = ndu_check_ndu_status($function, $request, \@response);
        if ($status == 2) {
            ndu_log_info("ndu_complete_ndu: WARNING NDU to: $to_package_name Failed !");
            return 2;
        } elsif ($status == 1) {
            my $b_continue = 1;
            ndu_log_info("ndu_complete_ndu: NDU not complete sleeping: $delay_before_status_check seconds");
            sleep($delay_before_status_check);
            ndu_log_info("ndu_complete_ndu: NDU not complete do you want to continue (1=Yes: 0=No): ");
            $b_continue = <STDIN>;
            chomp($b_continue);
            if (!$b_continue) {
                ndu_log_info("ndu_complete_ndu: WARNING skipping wait for NDU complete");
                return 0;
            }
        } elsif ($status == 0) {
            ndu_log_info("ndu_complete_ndu: NDU complete. Commit");
            $b_ndu_complete = 1;
            last;
        } else {            
            ndu_log_info("ndu_complete_ndu: Unexpected status: $status from: ndu_check_ndu_status ");
            return 3;
        }
    }

    # Commit the package on the active
    $target_sp = $test{active_sp};
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -commit $to_package_name";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -commit $to_package_name`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 4;
    }

    # Check the status
    sleep($delay_before_status_check);
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp ndu -status`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 5;
    }
    ndu_log_info("ndu_complete_ndu: NDU of: $to_package_name check status: SUCCESS");

    # Set the delay back to default
    $status = ndu_set_ndu_reboot_delay($test_ref, $default_delay_before_primary_reboot);
    if ($status != 0) {
        return 6;
    }

    # Return the $status
    return $status;
}
#end ndu_complete_ndu

sub ndu_change_trigger_spare_timer
{
    my ($test_ref, $b_increase, $trigger_spare_seconds) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $b_timer_changed = 0;
    my $status = 0;

    # Start I/O
    ndu_log_info("ndu_change_trigger_spare_timer: target sp: $target_sp increase: $b_increase seconds: $trigger_spare_seconds");

    # @todo Currently no access to FBE_CLI
    ndu_log_info("ndu_change_trigger_spare_timer: Please set FBE_CLI> hs -delay_swap $trigger_spare_seconds");
    sleep(10);
    while (!$b_timer_changed) {
        ndu_log_info("Enter 1 when hs -delay_swap set");
        $b_timer_changed = <STDIN>;
        chomp($b_timer_changed);
        if (!$b_timer_changed) {
            sleep(30);
        }
    }

    # Return $status
    return $status;
}

sub ndu_test_start_io
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $host_name = $test{host_name};
    my @rgs = @{$test{rgs_ref}};
    my $b_io_started = 0;
    my $status = 0;

    # Start I/O
    ndu_log_info("ndu_test_start_io: target sp: $target_sp first rg id: $rgs[0]{id}");

    # @todo Currently starting I/O is manual progress
    ndu_log_info("ndu_test_start_io: Please start I/O on host: $host_name (fail-over enabled) and enter 1 when ready ");
    sleep(10);
    while (!$b_io_started) {
        ndu_log_info("Enter 1 when I/O started");
        $b_io_started = <STDIN>;
        chomp($b_io_started);
        if (!$b_io_started) {
            sleep(30);
        }
    }

    # Return $status
    return $status;
}

sub ndu_set_eol_on_disk
{
    my ($test_ref, $disk) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $request = 0;
    my @response = ();
    my $function = "ndu_set_eol_on_disk";
    my $status = 0;

    ndu_log_info("ndu_set_eol_on_disk: target sp: $target_sp set EOL on disk: $disk");

    # @todo Currently there is no FBE_CLI interface so simply start a user copy
    $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp copytodisk $disk -o";
    @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp copytodisk $disk -o`;

    # Check the response
    $status = ndu_check_response($function, $request, \@response);
    if ($status != 0) {
        return 2;
    }
    ndu_log_info("ndu_set_eol_on_disk: copytodisk from: $disk started");

    # Return $status
    return $status;
}
# end ndu_set_eol_on_disk

sub ndu_clear_eol_on_disk
{
    my ($test_ref, $disk) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_clear_eol_on_disk";
    my $b_eol_cleared = 0;
    my $status = 0;

    ndu_log_info("ndu_set_clear_on_disk: target sp: $target_sp clear EOL on disk: $disk");

    # @todo Currently there is no FBE_CLI interface so simply clear EOL manually
    ndu_log_info("ndu_test_start_io: Please start I/O on host: $host_name (fail-over enabled) and enter 1 when ready ");
    sleep(10);
    while (!$b_eol_cleared) {
        ndu_log_info("Enter 1 when EOL cleared on disk: $disk");
        $b_eol_cleared = <STDIN>;
        chomp($b_eol_cleared);
        if (!$b_eol_cleared) {
            sleep(30);
        }
    }
    ndu_log_info("ndu_clear_eol_on_disk: EOL cleared on disk: $disk");

    # Return $status
    return $status;
}
# end ndu_clear_eol_on_disk

sub ndu_test_start_proactive_copy
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $wait_secs = $trigger_spare_short;
    my $status = 0;

    ndu_log_info("ndu_test_start_proactive_copy: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Start proactive copy tests
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my @disks = @{$rg->{disks}};
        my $b_is_proactive_copy_enabled = $rg->{b_test_copy_enabled};
        my $b_is_proactive_copy_started = $rg->{b_test_copy_started};
        my $copy_position = $rg->{test_copy_position};
        my $disk = $disks[$copy_position];

        # If the proactive copy is already started report an error
        if ($b_is_proactive_copy_started) {
            ndu_log_info("ndu_test_start_proactive_copy: rg id: $rg_id position: $copy_position disk: $disk copy already started");
            return 2;
        }
        
        # Only start if enabled
        if (!$b_is_proactive_copy_enabled) {
            next;
        }

        # Set EOL on disk
        ndu_log_info("ndu_test_start_proactive_copy: start proactive copy on position: $copy_position disk: $disk rg id: $rg_id on: $target_sp ");
        $status = ndu_set_eol_on_disk($test_ref, $disk);
        if ($status != 0) {
            return 3;
        }
        $rg->{b_test_copy_started} = 1;
        ndu_log_info("ndu_test_start_proactive_copy: Copy started on: $disk from rg id: $rg_id raid type: $raid_type ");
    }
    
    # Wait for copies to start
    ndu_log_info("ndu_test_start_proactive_copy: Wait: $wait_secs for copy to start");
    sleep($wait_secs);
    ndu_log_info("ndu_test_start_proactive_copy: wait complete");

    # Return $status
    return $status;
}
#end ndu_test_start_proactive_copy

sub ndu_test_start_verify
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_start_verify";
    my $verify_type = "SYSTEM"; # @todo Currently Read Only verify is not supported
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;

    ndu_log_info("ndu_test_start_verify: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Start SLF test
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_verify_enabled = $rg->{b_test_verify_enabled};
        my $b_is_verify_started = $rg->{b_test_verify_started};

        # If there is already started error
        if ($b_is_verify_started) {
            ndu_log_info("ndu_test_start_verify: rg id: $rg_id position: $verify_type verify already started");
            return 2;
        }
        
        # Skip if not enabled
        if (!$b_is_verify_enabled) {
            next;
        }

        # Start the R/W verify
        ndu_log_info("ndu_test_start_verify: start: $verify_type verify on rg id: $rg_id on: $target_sp ");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp setsniffer -rg $rg_id -bv -cr";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp setsniffer -rg $rg_id -bv -cr`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        $rg->{b_test_verify_started} = 1;
    }

    # Return $status
    return $status;
}
# end ndu_test_start_verify

sub ndu_test_start_rebuild
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_start_rebuild";
    my @rgs = @{$test{rgs_ref}};
    my $wait_secs = ($trigger_spare_short * 2);
    my $status = 0;

    ndu_log_info("ndu_test_start_rebuild: target sp: $target_sp first rg id: $rgs[0]{rg_id}");

    # Change the trigger spare timer to a small value
    $status = ndu_change_trigger_spare_timer($test_ref, 0, $trigger_spare_short);
    if ($status != 0) {
        return 1;
    }

    # Start rebuild test
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_rebuild_enabled = $rg->{b_test_rebuild_enabled};
        my $b_is_rebuild_started = $rg->{b_test_rebuild_started};
        my $rebuild_position_1 = $rg->{test_rebuild_position_1};
        my $rebuild_position_2 = $rg->{test_rebuild_position_2};
        my $disk1 = $disks[$rebuild_position_1];
        my $disk2 = $invalid_disk;
        my $b_insert_remove = 0;    # Remove the drives

        # If there is already a drive removed don't start
        if ($b_is_rebuild_started) {
            ndu_log_info("ndu_test_start_rebuild: rg id: $rg_id position: $rebuild_position_1 disk: $disk1 already rebuilding");
            return 2;
        }
        
        # If not enabled skip
        if (!$b_is_rebuild_enabled) {
            next;
        }

        # Remove on BOTH sides. First active
        if ($rebuild_position_2 != $invalid_position) {
            $disk2 = $disks[$rebuild_position_2];
        }
        ndu_log_info("ndu_test_start_rebuild: remove position: $rebuild_position_1 disk1: $disk1 rg id: $rg_id from active sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        if ($rebuild_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_start_rebuild: remove position: $rebuild_position_2 disk2: $disk2 rg id: $rg_id from both SPs (spa: $target_sp)");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 4;
            }
        }
        # Now passive
        $target_sp = $test{passive_sp};
        ndu_log_info("ndu_test_start_rebuild: remove position: $rebuild_position_1 disk1: $disk1 rg id: $rg_id from passive sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 5;
        }
        if ($rebuild_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_start_rebuild: remove position: $rebuild_position_2 disk2: $disk2 rg id: $rg_id from passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 6;
            }
        }
        $target_sp = $test{active_sp};
        $rg->{b_test_rebuild_started} = 1;
    }

    # Wait for copies to start
    ndu_log_info("ndu_test_start_rebuild: Wait: $wait_secs for rebuilds to start");
    sleep($wait_secs);
    ndu_log_info("ndu_test_start_rebuild: wait complete");

    # Change the trigger spare timer to the default value
    $status = ndu_change_trigger_spare_timer($test_ref, 1, $trigger_spare_default);
    if ($status != 0) {
        return 7;
    }

    # Return $status
    return $status;
}

sub ndu_test_start_slf
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_start_slf";
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;

    ndu_log_info("ndu_test_start_slf: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Start SLF test
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_slf_enabled = $rg->{b_test_slf_enabled};
        my $b_is_slf_started = $rg->{b_test_slf_started};
        my $slf_position = $rg->{test_slf_position};
        my $disk = $disks[$slf_position];
        my $b_insert_remove = 0;    # Remove on one SP

        # If there is already a drive removed don't start
        if ($b_is_slf_started) {
            ndu_log_info("ndu_test_start_slf: rg id: $rg_id position: $slf_position disk: $disk already in SLF");
            return 2;
        }
        
        # Skip if not enabled
        if (!$b_is_slf_enabled) {
            next;
        }

        # Remove on one side
        ndu_log_info("ndu_test_start_slf: remove position: $slf_position disk: $disk rg id: $rg_id on: $target_sp ");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk $b_insert_remove`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        $rg->{b_test_slf_started} = 1;
    }

    # Return $status
    return $status;
}
# end ndu_test_start_slf

sub ndu_test_start_paged
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_start_paged";
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;

    ndu_log_info("ndu_test_start_paged: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Change the trigger spare timer to a large value
    # @note We never change it back until `complete' !!!
    $status = ndu_change_trigger_spare_timer($test_ref, 1, 100000);
    if ($status != 0) {
        return 1;
    }

    # Start paged metadata verify (i.e. make the raid group go broken)
    # @note MAKE THE RAID GROUP GO BROKEN !!!!
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_paged_enabled = $rg->{b_test_paged_enabled};
        my $b_is_paged_started = $rg->{b_test_paged_started};
        my $paged_position_1 = $rg->{test_paged_position_1};
        my $paged_position_2 = $rg->{test_paged_position_2};
        my $paged_position_3 = $rg->{test_paged_position_3};
        my $disk1 = $disks[$paged_position_1];
        my $disk2 = $invalid_disk;
        my $disk3 = $invalid_disk;
        my $b_insert_remove = 0;    # Remove on both SPs

        # If there is already a drive removed don't start
        if ($b_is_paged_started) {
            ndu_log_info("ndu_test_start_paged: rg id: $rg_id position: $paged_position_1 disk1: $disk1 already removed");
            return 2;
        }
        
        # Skip if not enabled
        if (!$b_is_paged_enabled) {
            next;
        }

        # Remove on active side first
        if ($paged_position_2 != $invalid_position) {
            $disk2 = $disks[$paged_position_2];
        }
        if ($paged_position_3 != $invalid_position) {
            $disk3 = $disks[$paged_position_3];
        }
        ndu_log_info("ndu_test_start_paged: remove position: $paged_position_1 disk: $disk1 rg id: $rg_id from active sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        if ($paged_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_start_paged: remove position: $paged_position_2 disk2: $disk2 rg id: $rg_id from active sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 4;
            }
        }
        if ($paged_position_3 != $invalid_position) {
            ndu_log_info("ndu_test_start_paged: remove position: $paged_position_3 disk3: $disk3 rg id: $rg_id from active sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 5;
            }
        }
        # Now passive
        $target_sp = $test{passive_sp};
        ndu_log_info("ndu_test_start_paged: remove position: $paged_position_1 disk: $disk1 rg id: $rg_id from passive sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 6;
        }
        if ($paged_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_start_paged: remove position: $paged_position_1 disk2: $disk2 rg id: $rg_id from passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 6;
            }
        }
        if ($paged_position_3 != $invalid_position) {
            ndu_log_info("ndu_test_start_paged: remove position: $paged_position_3 disk3: $disk3 rg id: $rg_id from passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 5;
            }
        }
        $target_sp = $test{active_sp};
        $rg->{b_test_paged_started} = 1;
    }

    # Return $status
    return $status;
}

sub ndu_start_io
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_start_io";

    ndu_log_info("ndu_start_io: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Start I/O
    $status = ndu_test_start_io($test_ref);
    if ($status != 0) {
        return 1;
    }
    test_ref->{b_io_started} = 1;

    # Return $status
    return $status;
}
#end ndu_start_io

sub ndu_stop_io
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_start_io";

    ndu_log_info("ndu_stop_io: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Stop I/O
    $status = ndu_test_stop_io($test_ref);
    if ($status != 0) {
        return 1;
    }
    test_ref->{b_io_started} = 0;

    # Return $status
    return $status;
}
#end ndu_stop_io

sub ndu_start_pre_ndu_tests
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_start_pre_ndu_tests";

    ndu_log_info("ndu_start_pre_ndu_tests: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Start verify
    $status = ndu_test_start_verify($test_ref);
    if ($status != 0) {
        return 1;
    }

    # Step 2: Start proactive copy
    $status = ndu_test_start_proactive_copy($test_ref);
    if ($status != 0) {
        return 2;
    }

    # Return $status
    return $status;
}
# end ndu_start_pre_ndu_tests

sub ndu_start_during_ndu_tests
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_start_tests";

    ndu_log_info("ndu_start_during_ndu_tests: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Start SLF
    $status = ndu_test_start_slf($test_ref);
    if ($status != 0) {
        return 1;
    }

    # Step 2: Start rebuild
    $status = ndu_test_start_rebuild($test_ref);
    if ($status != 0) {
        return 2;
    }

    # Step 3: Cause raid group to go broken
    $status = ndu_test_start_paged($test_ref);
    if ($status != 0) {
        return 3;
    }

    # Step 4: Set the delay back to default
    $status = ndu_set_ndu_reboot_delay($test_ref, $default_delay_before_primary_reboot);
    if ($status != 0) {
        return 4;
    }

    # Return $status
    return $status;
}
# end ndu_start_during_ndu_tests

sub ndu_test_complete_proactive_copy
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $wait_secs = $trigger_spare_short;
    my $status = 0;

    ndu_log_info("ndu_test_complete_proactive_copy: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Wait for proactive copys to complete
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my @disks = @{$rg->{disks}};
        my $b_is_proactive_copy_enabled = $rg->{b_test_copy_enabled};
        my $b_is_proactive_copy_started = $rg->{b_test_copy_started};
        my $copy_position = $rg->{test_copy_position};
        my $disk = $disks[$copy_position];

        # If the proactive copy wasn;t started report an error
        if ($b_is_proactive_copy_enabled && !$b_is_proactive_copy_started) {
            ndu_log_info("ndu_test_complete_proactive_copy: rg id: $rg_id position: $copy_position disk: $disk copy wasnt started");
            return 2;
        }
        
        # Only complete if started 
        if (!$b_is_proactive_copy_started) {
            next;
        }

        # Clear EOL on disk
        ndu_log_info("ndu_test_complete_proactive_copy: Clear EOL on position: $copy_position disk: $disk rg id: $rg_id on: $target_sp ");
        $status = ndu_clear_eol_on_disk($test_ref, $disk);
        if ($status != 0) {
            return 3;
        }

        # @todo Currently we just delay (i.e. we don't wait for copy to complete)
        ndu_log_info("ndu_test_complete_proactive_copy: Wait: $wait_secs for copy to complete");
        sleep($wait_secs);
        ndu_log_info("ndu_test_complete_proactive_copy: wait complete for diks: $disk");
        # Copy is not longer? running
        $rg->{b_test_copy_started} = 0;
    }
    
    # Return $status
    return $status;
}
# end ndu_test_complete_proactive_copy

sub ndu_test_complete_rebuild
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_complete_rebuild";
    my @rgs = @{$test{rgs_ref}};
    my $wait_secs = ($trigger_spare_short * 2);
    my $status = 0;

    ndu_log_info("ndu_test_complete_rebuild: target sp: $target_sp first rg id: $rgs[0]{rg_id}");

    # Change the trigger spare timer back to default value
    $status = ndu_change_trigger_spare_timer($test_ref, 0, $trigger_spare_default);
    if ($status != 0) {
        return 1;
    }

    # Re-insert the removed drives
    # Wait for rebuilds to complete
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_rebuild_enabled = $rg->{b_test_rebuild_enabled};
        my $b_is_rebuild_started = $rg->{b_test_rebuild_started};
        my $rebuild_position_1 = $rg->{test_rebuild_position_1};
        my $rebuild_position_2 = $rg->{test_rebuild_position_2};
        my $disk1 = $disks[$rebuild_position_1];
        my $disk2 = $invalid_disk;
        my $b_insert_remove = 1; # Re-insert the drives

        # Rebuild should have been started
        if ($b_is_rebuild_enabled && !$b_is_rebuild_started) {
            ndu_log_info("ndu_test_complete_rebuild: rg id: $rg_id position: $rebuild_position_1 disk: $disk1 rebuild not started");
            return 2;
        }
        
        # If not started skip
        if (!$b_is_rebuild_started) {
            next;
        }

        # Insert on BOTH sides. First active
        if ($rebuild_position_2 != $invalid_position) {
            $disk2 = $disks[$rebuild_position_2];
        }
        ndu_log_info("ndu_test_complete_rebuild: insert removed disk1: $disk1 rg id: $rg_id on active sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        if ($rebuild_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_complete_rebuild: insert removed disk2: $disk2 rg id: $rg_id on active sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 4;
            }
        }
        # Now passive
        $target_sp = $test{passive_sp};
        ndu_log_info("ndu_test_complete_rebuild: insert removed disk1: $disk1 rg id: $rg_id on passive sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 5;
        }
        if ($rebuild_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_complete_rebuild: insert removed disk2: $disk2 rg id: $rg_id on passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 6;
            }
        }
        $target_sp = $test{active_sp};
        # @todo Check and wait for rebuild to complete
        ndu_log_info("ndu_test_start_rebuild: Wait: $wait_secs for rebuilds to complete");
        sleep($wait_secs);
        ndu_log_info("ndu_test_start_rebuild: wait for rg id: $rg_id rebuild to complete done");
        # Rebuild is no longer running?
        $rg->{b_test_rebuild_started} = 0;
    }

    # Return $status
    return $status;
}
# end ndu_test_complete_rebuild

sub ndu_test_complete_slf
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_complete_slf";
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;

    ndu_log_info("ndu_test_complete_slf: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Re-insert the drives that wer put into SLF
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_slf_enabled = $rg->{b_test_slf_enabled};
        my $b_is_slf_started = $rg->{b_test_slf_started};
        my $slf_position = $rg->{test_slf_position};
        my $disk = $disks[$slf_position];
        my $b_insert_remove = 1;    # Re-insert the drive on one side

        # If there is already a drive removed don't start
        if ($b_is_slf_enabled && !$b_is_slf_started) {
            ndu_log_info("ndu_test_complete_slf: rg id: $rg_id position: $slf_position disk: $disk SLF was not started");
            return 2;
        }
        
        # Skip if not started
        if (!$b_is_slf_started) {
            next;
        }

        # Insert on one side
        ndu_log_info("ndu_test_complete_slf: insert position: $slf_position disk: $disk rg id: $rg_id on: $target_sp ");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk $b_insert_remove`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
    }

    # Return $status
    return $status;
}
# end ndu_test_complete_slf

sub ndu_test_complete_verify
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_complete_verify";
    my $verify_type = "System";
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;

    ndu_log_info("ndu_test_complete_verify: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Chekc the verify status
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_verify_enabled = $rg->{b_test_verify_enabled};
        my $b_is_verify_started = $rg->{b_test_verify_started};

        # If there is already a drive removed don't start
        if ($b_is_verify_enabled && !$b_is_verify_started) {
            ndu_log_info("ndu_test_complete_verify: rg id: $rg_id $verify_type verify was not started");
            return 2;
        }
        
        # Skip if not started
        if (!$b_is_verify_started) {
            next;
        }

        # Insert on one side
        ndu_log_info("ndu_test_complete_verify: get $verify_type verify report for luns in rg id: $rg_id on: $target_sp ");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp getsniffer -rg $rg_id -curr";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp getsniffer -rg $rg_id -curr`;

        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
    }

    # Return $status
    return $status;
}
# end ndu_test_complete_verify

sub ndu_test_complete_paged
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{passive_sp};
    my $username = $test{username};
    my $password = $test{password};
    my $function = "ndu_test_complete_paged";
    my @rgs = @{$test{rgs_ref}};
    my $wait_secs = $trigger_spare_short;
    my $status = 0;
    $test->{function} = "ndu_test_complete_paged";

    ndu_log_info("ndu_test_complete_paged: target sp: $target_sp first rg id: $rgs[0]{id}");

    # Re-insert the drives removed that caused the raid group to go broken
    # Validate that the paged metadata verify is started
    foreach my $rg (@rgs) {
        my $rg_id = $rg->{id}; 
        my $raid_type = $rg->{raid_type};
        my $request = 0;
        my @response = ();
        my @disks = @{$rg->{disks}};
        my $b_is_paged_enabled = $rg->{b_test_paged_enabled};
        my $b_is_paged_started = $rg->{b_test_paged_started};
        my $paged_position_1 = $rg->{test_paged_position_1};
        my $paged_position_2 = $rg->{test_paged_position_2};
        my $paged_position_3 = $rg->{test_paged_position_3};
        my $disk1 = $disks[$paged_position_1];
        my $disk2 = $invalid_disk;
        my $disk3 = $invalid_disk;
        my $b_insert_remove = 1;    # Re-insert the removed drives

        # If there is already a drive removed don't start
        if ($b_is_paged_enabled && !$b_is_paged_started) {
            ndu_log_info("ndu_test_complete_paged: rg id: $rg_id position: $paged_position_1 disk1: $disk1 never started");
            return 2;
        }
        
        # Skip if not started
        if (!$b_is_paged_started) {
            next;
        }

        # Re-insert on passive side first
        if ($paged_position_2 != $invalid_position) {
            $disk2 = $disks[$paged_position_2];
        }
        if ($paged_position_3 != $invalid_position) {
            $disk3 = $disks[$paged_position_3];
        }
        ndu_log_info("ndu_test_complete_paged: insert position: $paged_position_1 disk: $disk1 rg id: $rg_id on active sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 3;
        }
        if ($paged_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_complete_paged: insert position: $paged_position_2 disk2: $disk2 rg id: $rg_id on active sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 4;
            }
        }
        if ($paged_position_3 != $invalid_position) {
            ndu_log_info("ndu_test_complete_paged: insert position: $paged_position_3 disk3: $disk3 rg id: $rg_id on active sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 5;
            }
        }
        # Now active
        $target_sp = $test{active_sp};
        ndu_log_info("ndu_test_complete_paged: insert position: $paged_position_1 disk: $disk1 rg id: $rg_id on passive sp: $target_sp");
        $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove";
        @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk1 $b_insert_remove`;
        # Check the response
        $status = ndu_check_response($function, $request, \@response);
        if ($status != 0) {
            return 6;
        }
        if ($paged_position_2 != $invalid_position) {
            ndu_log_info("ndu_test_complete_paged: insert position: $paged_position_1 disk2: $disk2 rg id: $rg_id on passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk2 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 6;
            }
        }
        if ($paged_position_3 != $invalid_position) {
            ndu_log_info("ndu_test_complete_paged: remove position: $paged_position_3 disk3: $disk3 rg id: $rg_id on passive sp: $target_sp");
            $request = "naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove";
            @response = `naviseccli -user $username -password $password -scope 0 -h $target_sp cru_on_off -messner $disk3 $b_insert_remove`;
            # Check the response
            $status = ndu_check_response($function, $request, \@response);
            if ($status != 0) {
                return 7;
            }
        }
        $target_sp = $test{active_sp};
        # @todo Check that the verify starts
        ndu_log_info("ndu_test_complete_paged: Wait: $wait_secs for Incomplete Write Verify to start");
        sleep($wait_secs);
        ndu_log_info("ndu_test_complete_paged: wait for verify to start on rg id: $rg_id complete");
        # No longer broken
        $rg->{b_test_paged_started} = 0;
    }

    # Change the trigger spare timer to default value
    $status = ndu_change_trigger_spare_timer($test_ref, 0, $trigger_spare_default);
    if ($status != 0) {
        return 8;
    }

    # Return $status
    return $status;
}
#end ndu_test_complete_paged

sub ndu_bring_broken_rgs_online
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{passive_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_bring_broken_rgs_online";

    ndu_log_info("ndu_bring_broken_rgs_online: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Re-insert drives that caused raid group to go broken
    $status = ndu_test_complete_paged($test_ref);
    if ($status != 0) {
        return 1;
    }

    # Return $status
    return $status;
}
# end ndu_bring_broken_rgs_online

sub ndu_complete_tests
{
    my ($test_ref) = @_;
    my %test = %$test_ref;
    my $target_sp = $test{active_sp};
    my $username = $test{username};
    my $password = $test{password};
    my @rgs = @{$test{rgs_ref}};
    my $status = 0;
    $test->{function} = "ndu_complete_tests";

    ndu_log_info("ndu_complete_tests: target sp: $target_sp first rg id: $rgs[0]{id}");
    
    # @note THE ORDER IS IMPORTANT !!!!

    # Step 1: Re-insert drives that caused raid group to go broken
    $status = ndu_test_complete_paged($test_ref);
    if ($status != 0) {
        return 1;
    }

    # Step 2: Wait for rebuilds to complete
    $status = ndu_test_complete_rebuild($test_ref);
    if ($status != 0) {
        return 2;
    }

    # Step 3: Wait for copies to complete (maybe)
    $status = ndu_test_complete_proactive_copy($test_ref);
    if ($status != 0) {
        return 3;
    }

    # Step 4: Re-insert SLF drives
    $status = ndu_test_complete_slf($test_ref);
    if ($status != 0) {
        return 4;
    }

    # Step 5: Wait for verify to complete
    $status = ndu_test_complete_verify($test_ref);
    if ($status != 0) {
        return 5;
    }

    # Return $status
    return $status;
}
# end ndu_complete_tests

