#!/usr/bin/perl
# WARNING: this file is generated, do not edit
# generated on Wed May 27 03:01:20 2020
# 01: Apache-Test/lib/Apache/TestConfig.pm:1007
# 02: Apache-Test/lib/Apache/TestConfig.pm:1099
# 03: Apache-Test/lib/Apache/TestMM.pm:142
# 04: Makefile.PL:46

BEGIN { eval { require blib && blib->import; } }

$Apache::TestConfig::Argv{'apxs'} = q|/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs|;

$Apache::TestConfig::Argv{'limitrequestline'} = q|128|;

$Apache::TestConfig::Argv{'limitrequestlinex2'} = q|256|;
print "HTTP/1.0 200 OK\r\n";
print "Transfer-Encoding: chunked\r\n";
print "\r\n";

$| = 1;

sub dripfeed {
    my $s = shift;

    while (length($s)) {
        select(undef, undef, undef, 0.2);
        print substr($s, 0, 1);
        $s = substr($s, 1);
    }
}

dripfeed "0005\r\nabcde\r\n1; foo=bar\r\nf\r\n0\r\n\r\n";
