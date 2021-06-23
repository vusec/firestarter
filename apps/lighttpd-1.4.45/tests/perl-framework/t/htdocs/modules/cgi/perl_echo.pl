#!/usr/bin/perl
# WARNING: this file is generated, do not edit
# generated on Wed May 27 03:01:20 2020
# 01: Apache-Test/lib/Apache/TestConfig.pm:1007
# 02: Apache-Test/lib/Apache/TestConfig.pm:1099
# 03: Apache-Test/lib/Apache/TestMM.pm:142
# 04: Makefile.PL:46

BEGIN { eval { require blib && blib->import; } }

$Apache::TestConfig::Argv{'limitrequestlinex2'} = q|256|;

$Apache::TestConfig::Argv{'limitrequestline'} = q|128|;

$Apache::TestConfig::Argv{'apxs'} = q|/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs|;
#echo some data back to the client

print "Content-type: text/plain\n\n";

if (my $ct = $ENV{CONTENT_LENGTH}) {
    read STDIN, my $buffer, $ct;
    print $buffer;
}
elsif (my $qs = $ENV{QUERY_STRING}) {
    print $qs;
}
else {
    print "nada";
}
