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
use MIME::Base64;
use strict;
use warnings;

print "Content-type: text/plain\r\n";
print decode_base64($ENV{QUERY_STRING}), "\r\n";
print "\r\n";
print "Hi!\n";
print "SERVERNAME=$ENV{SERVER_NAME}\n";
print "HTTP_HOST=$ENV{HTTP_HOST}\n";
