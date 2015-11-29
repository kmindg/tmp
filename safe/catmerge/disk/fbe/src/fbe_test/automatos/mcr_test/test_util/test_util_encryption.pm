package mcr_test::test_util::test_util_encryption;

use strict;
use base qw(Exporter);
our @EXPORT = do {
    no strict 'refs';
    grep {
        defined &$_ and not /^_/;
    } keys %{ __PACKAGE__ . '::'};
};
use mcr_test::platform_shim::platform_shim_device;
use mcr_test::platform_shim::platform_shim_sp;
use mcr_test::platform_shim::platform_shim_raid_group;

our $FBE_LBA_INVALID = '0xFFFFFFFFFFFFFFFF';
our $rekey_complete_checkpoint = Math::BigInt->from_hex($FBE_LBA_INVALID);

sub test_util_encryption_get_encryption_status 
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my %results;

    %results = $device->getEncryptionStatus();
    $tc->platform_shim_test_case_log_info("Encryption Status: ".$results{status});
    return $results{status};
}

sub test_util_encryption_is_encryption_supported
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my %results;

    %results = $device->getEncryptionInfo();
    $tc->platform_shim_test_case_log_info("Encryption Supported: ".$results{supported});
    return $results{supported};
}

sub test_util_encryption_get_encryption_mode 
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my %results;

    %results = $device->getEncryptionInfo();
    $tc->platform_shim_test_case_log_info("Encryption Mode: ".$results{mode});
    return $results{mode};
}

sub test_util_encryption_get_encryption_state 
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my %results;

    %results = $device->getEncryptionInfo();
    $tc->platform_shim_test_case_log_info("Encryption State: ".$results{encryption_state});
    return $results{encryption_state};
}

sub test_util_encryption_get_percent_encrypted
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my %results;

    %results = $device->getEncryptionInfo();
    $tc->platform_shim_test_case_log_info("Percent Encrypted: ".$results{percent_encrypted});
    return $results{percent_encrypted};
}

sub test_util_encryption_activate_encryption
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my $encryption_status;
    my $status = 0;

    $encryption_status = test_util_encryption_get_encryption_status($tc);
    if ($encryption_status =~ /Encryption mode not enabled/) {
        $tc->platform_shim_test_case_log_info("Activating Encryption");
        eval { 
            $device->activateEncryption();
        };
        if (my $e = Exception::Class->caught()) {
            my $msg = ref($e) ? $e->full_message() : $e;
            print "Activating encryption failed \n" . $msg;
        }
        $tc->platform_shim_test_case_log_info("Waiting for Encryption to be Activated");
        $device->waitForEncryptionState(state => 'Any', timeout => '10M');
        $encryption_status = test_util_encryption_get_encryption_status($tc);
        if ($encryption_status =~ /Encryption mode not enabled/) {
            $tc->platform_shim_test_case_log_error("Failed to activate encryption");
            $status = 1;
        }
    }
    return $status;
}


sub test_util_encryption_install_enabler
{
    my ($tc, $encryption_enabler) = @_;
    my $device = $tc->{device};
    my @package_files = ($encryption_enabler);
    my $encryption_supported;
    my $status = 0;

    $tc->platform_shim_test_case_log_info("Installing CBE enabler via NDU: $encryption_enabler");
    $device->nduInstall(package_files => \@package_files);

    $tc->platform_shim_test_case_log_info("Waiting for CBE enabler install to complete");
    $device->waitForNduComplete(timeout => '2H');

    $tc->platform_shim_test_case_log_info("Ensuring that the CBE enabler has been installed");
    $encryption_supported = test_util_encryption_is_encryption_supported($tc);
    if ($encryption_supported =~ /Yes/) {
        $tc->platform_shim_test_case_log_info("Encryption enabler successfully installed");
    } else {
        $tc->platform_shim_test_case_log_error("Install of encyption enabler failed");
        $status = 1;
    }
    return $status;
}

sub test_util_is_rekey_complete
{
	my ($rekey_checkpoint) = @_;

    if ($rekey_checkpoint == $rekey_complete_checkpoint) {
        return 1;
    }

    return 0;
}

sub test_util_encryption_wait_for_encryption_complete
{
    my ($tc, $timeout) = @_;
    my $device = $tc->{device};
    my $percent_encrypted = 0;
    my $wait_until = $timeout + time();

    $percent_encrypted = test_util_encryption_get_percent_encrypted($tc);
    while (time() < $wait_until) {
        if ($percent_encrypted =~ /100/i) {
            $tc->platform_shim_test_case_log_info("Encryption complete");
            return 0;
        }
        sleep(10);
        $percent_encrypted = test_util_encryption_get_percent_encrypted($tc);
    }
    $tc->platform_shim_test_case_log_error("Timed-out wait for encryption complete");
    return 1; 
}

sub test_util_encryption_start_rekey
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my $status = 0;
    my $percent_encrypted = 0;

    # Cannot start rekey unless encryption is complete
    $percent_encrypted = test_util_encryption_get_percent_encrypted($tc);
    if ($percent_encrypted =~ /100/i) {
        $tc->platform_shim_test_case_log_info("Starting Rekey operation, to restart encryption");
        $device->rekeyEncryption();

        $tc->platform_shim_test_case_log_info("Waiting for encryption rekey to be in progress");
        $device->waitForEncryptionState(state => 'In progress', timeout => '10M');
    } else {
        $tc->platform_shim_test_case_log_error("Start Rekey: Cannot start rekey until encryption is complete - percent complete: $percent_encrypted");
        return 1;
    }
    return $status;
}

sub test_util_encryption_get_rg_rekey_checkpoint 
{
    my ($rg) = @_;
    my $rekey_checkpoint = Math::BigInt->from_hex($FBE_LBA_INVALID);

    $rekey_checkpoint = Math::BigInt->from_hex(platform_shim_raid_group_get_property($rg, 'rekey_checkpoint'));
    return $rekey_checkpoint;
}

sub test_util_encryption_check_rekey_progress
{
	my ($tc, $rg, $timeout_sec) = @_;	
	
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
    my $start_rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg);
    my $start_rekey_checkpoint_hex = $start_rekey_checkpoint->as_hex();
    my $start_time = time;
    my $elapsed_secs = time - $start_time;
    my $timeout_sec = $start_time + $timeout_sec;

    # Check if the rekey is already complete
    if (test_util_is_rekey_complete($start_rekey_checkpoint)) {
        $tc->platform_shim_test_case_log_info("Check rekey progress for disk: $rg_id checkpoint: " .
                                              $start_rekey_checkpoint_hex . " complete");
        return $start_rekey_checkpoint;
    }

    # @todo Currently we log this to debug
    $tc->platform_shim_test_case_log_info("Check rekey progress for rg id: $rg_id start checkpoint: " .
                                          $start_rekey_checkpoint_hex);
    my $current_rekey_checkpoint = $start_rekey_checkpoint; 
    my $current_rekey_checkpoint_hex = $current_rekey_checkpoint->as_hex();
	while (time < $timeout_sec) {
        $elapsed_secs = time - $start_time;
        $current_rekey_checkpoint_hex = $current_rekey_checkpoint->as_hex();
        if (test_util_is_rekey_complete($current_rekey_checkpoint)) {
            $tc->platform_shim_test_case_log_info("Check rekey progress - checkpoint: " . 
                                                  $current_rekey_checkpoint_hex . " rekey complete");
            return $current_rekey_checkpoint;
        }
        if ($current_rekey_checkpoint > $start_rekey_checkpoint) {
            $tc->platform_shim_test_case_log_info("Rekey checkpoint for rg id: $rg_id increased from: " . 
                                                  $start_rekey_checkpoint_hex . " to: " .
                                                  $current_rekey_checkpoint_hex . " in $elapsed_secs seconds");
            return $current_rekey_checkpoint;
        }
		sleep(1);
        $current_rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg); 
	}
	
	platform_shim_test_case_throw_exception("Timeout waiting for encryption rekey checkpoint to increase");
    return $current_rekey_checkpoint;
}

sub test_util_encryption_check_rg_rekey_no_progress
{
	my ($tc, $rg, $timeout_sec) = @_;	
	
    my $rg_id = platform_shim_raid_group_get_property($rg, 'id');
    my $start_rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg);
    my $start_rekey_checkpoint_hex = $start_rekey_checkpoint->as_hex();
    my $start_time = time;
    my $elapsed_secs = time - $start_time;
	$timeout_sec = $start_time + $timeout_sec;
	
    # If the copy was never started return now
	$tc->platform_shim_test_case_log_info("Verify no rekey progress on rg id: $rg_id ");
    my $current_rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg);
    my $current_rekey_checkpoint_hex = $current_rekey_checkpoint->as_hex(); 
	while (time < $timeout_sec) {
        $elapsed_secs = time - $start_time;
        if ($current_rekey_checkpoint > $start_rekey_checkpoint) {
            platform_shim_test_case_throw_exception("Verify no rekey progress checkpoint: " .
                                                    $current_rekey_checkpoint_hex . " has increased from: " .
                                                    $start_rekey_checkpoint . " in: $elapsed_secs seconds");
            return $current_rekey_checkpoint;
        }
		sleep(1);
        $current_rekey_checkpoint = test_util_encryption_get_rg_rekey_checkpoint($rg);
        $current_rekey_checkpoint_hex = $current_rekey_checkpoint->as_hex();
	}
	
    $tc->platform_shim_test_case_log_info("Verify no rekey progress checkpoint: " .
                                          $current_rekey_checkpoint_hex . " has not changed in $elapsed_secs seconds");
    return $current_rekey_checkpoint;
}

sub test_util_encryption_get_audit_log
{
    my ($tc, $auditLogPath) = @_;
    my $device = $tc->{device};
    my %auditLogInfo;
    my $auditLogName = 'none';

    $tc->platform_shim_test_case_log_info("Retrieving Encryption Audit to host path: $auditLogPath");
    eval {
        %auditLogInfo = $device->getEncryptionAuditLog(path => $auditLogPath);#defaults to all
        $auditLogName = $auditLogInfo{audit_log_filename};
    };
    $tc->platform_shim_test_case_log_info("Retrieved Encryption Audit Log to host file: $auditLogName");
}

sub test_util_encryption_add_user_security
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my $originalNaviUser = 'admin';
    my $originalNaviPassword = 'admin';
    my $status = 0;

    $tc->platform_shim_test_case_log_info("Checking if User Security has been setup");
    eval {
        $device->getUserSecurityInfo()
    };
    if (Automatos::Exception::Base->caught()) {
        $tc->platform_shim_test_case_log_info("User Security is not setup. Add user: $originalNaviUser password: $originalNaviPassword");
        $device->addUserSecurity(user => $originalNaviUser, 
                                 password => $originalNaviPassword, 
                                 scope => 'global', 
                                 role => 'administrator');
    }
    return $status;
}

sub test_util_encryption_remove_user_security
{
    my ($tc) = @_;
    my $device = $tc->{device};
    my $originalNaviUser = 'admin';
    my $status = 0;

    $tc->platform_shim_test_case_log_info("Checking if User Security has been setup");
    eval {
        $device->getUserSecurityInfo()
    };
    if (Automatos::Exception::Base->caught()) {
    } else {
        $tc->platform_shim_test_case_log_info("User Security is setup. Remove user: $originalNaviUser ");
        $device->removeUserSecurity(user => $originalNaviUser, 
                                    scope => 'global' ); 
    }
    return $status;
}

1;
