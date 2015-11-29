#!/usr/bin/perl

############################################
# Prerequisites:
#
#	1. An automatos supported perl version must be installed
#	   Create an environment variable called AUTOMATOSPERL and 
#      set it to the supported perl version i.e. C:\perl516\bin
#   2. Automatos must be installed with supported perl modules.
#	   Create an environment variable called AUTOMATOS and
#      set it to the Automatos bin folder 
#      i.e. D:\Automatos\Automatos\Framework\Dev\bin
#   3. PERL5LIB paths must include the necessary Framework perl folders
#      i.e. T:\ADE\perlInc;D:\Automatos\Automatos;D:\Automatos\Automatos\Framework\Dev\
#           bin;D:\Automatos\Automatos\Framework\Dev\lib;D:\Automatos\Automatos\Tests\Dev;D:\Automatos\Automatos
#           \UtilityLibraries\Dev;D:\Automatos\Automatos\Framework\Dev;D:\views\starburst\safe\catmerge\disk\fbe
#           \src\fbe_test\automatos
#   4. This script must be executed from the workspace directory in order to set the workspace correctly
#      i.e. D:\views\starburst\safe\catmerge\disk\fbe\src\fbe_test\automatos
#
############################################

use strict;

use File::Basename;
use FindBin qw($Bin);
use lib "$Bin/../lib";
use lib "$Bin/..";

my $workspace = dirname(__FILE__);


$ENV{WORKSPACE} 	= $workspace;
$ENV{PATH}			= $ENV{AUTOMATOSPERL}.";".$ENV{AUTOMATOS};

my $perl5addon = $workspace;
$perl5addon =~ s/\\/\\\\/ig;
if ($ENV{PERL5LIB} !~ m/$perl5addon/ig) {
	$ENV{PERL5LIB} = $ENV{PERL5LIB}.";$workspace";
}

my $perlscript = $ENV{'AUTOMATOS'}."/automatos.pl";

unshift @ARGV, $perlscript;
unshift @ARGV, "perl";
my $result = system(@ARGV);


1;