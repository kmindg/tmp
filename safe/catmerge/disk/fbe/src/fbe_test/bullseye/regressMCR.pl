#!/usr/bin/perl
use File::Spec
# add the directory contain this script to the Path
my $program = $0;
my $index;
my $dir;
if ($^O eq 'MSWin32') {
    $index = rindex($program, "\\");
    $dir = substr($program, 0, $index);
    $ENV{'PATH'} =   $dir . ";" . $ENV{'PATH'};
} else {
    $index = rindex($program, "/");
    $dir = substr($program, 0, $index);
    $ENV{'PATH'} =   $dir . ":" . $ENV{'PATH'};
}
#
# just a wrapper to mutRegression.pl 
# that sets -configDir FabricArray/Cache/Test
#
my $configDir = File::Spec->catdir('disk', 'fbe', 'src', 'fbe_test', 'bullseye');
my $exec_file;
if ($dir) {
    $exec_file = File::Spec->catfile($dir, 'MCR_mutRegress.pl');
} else {
    $exec_file = 'MCR_mutRegress.pl';
}
exit(system($^X, $exec_file, "-configDir", $configDir, @ARGV));
