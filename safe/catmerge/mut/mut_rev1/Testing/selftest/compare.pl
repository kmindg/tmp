#############################################################################
#  Copyright (C) EMC Corporation 2007
#  All rights reserved.
#  Licensed material -- property of EMC Corporation
#############################################################################

#############################################################################
#                                 complare.pl
#############################################################################
# 
#  DESCRIPTION: MUT Compare Utility
# 
#  FUNCTIONS:
# 
#  NOTES:
#     Returns 0 if log files are equal, else 1
#  
#  HISTORY:
#     11/11/2007   Kirill Timofeev    initial version
#     02/13/2008   Igor Bobrovnikov   Line number masking removed
# 
#############################################################################

sub read_log {
    my($file) = @_;
    $out = "";
    while(<$file>) {
        s|\d\d/\d\d/\d\d \d\d:\d\d:\d\d||;
        $out .= $_;
    }
    return $out
}

open(f1, "<", $ARGV[0]);
open(f2, "<", $ARGV[1]);

$str1 = read_log(f1);
$str2 = read_log(f2);

exit(0) if ($str1 eq $str2);
exit(1);