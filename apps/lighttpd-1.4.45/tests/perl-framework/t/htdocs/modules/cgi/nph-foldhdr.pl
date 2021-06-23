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
# produces output with folded response headers

print "HTTP/1.0 200 OK\r\n";

for (1..50) {
    print "X-Foo-Bar-$_:\n " . 'x'x($_*10) . "\n";
    print "X-Bar-$_:\n gamm\r\n beta\n theta\r\n";
}

print "Content-type: \n  text/plain\n\n";

print "hello, world";
