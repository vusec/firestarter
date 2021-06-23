#!/usr/bin/perl
# WARNING: this file is generated, do not edit
# generated on Wed May 27 03:01:20 2020
# 01: Apache-Test/lib/Apache/TestConfig.pm:1007
# 02: Apache-Test/lib/Apache/TestConfig.pm:1099
# 03: Apache-Test/lib/Apache/TestMM.pm:142
# 04: Makefile.PL:46

BEGIN { eval { require blib && blib->import; } }

$Apache::TestConfig::Argv{'apxs'} = q|/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs|;

$Apache::TestConfig::Argv{'limitrequestlinex2'} = q|256|;

$Apache::TestConfig::Argv{'limitrequestline'} = q|128|;
# This is a regression test for PR 31247. 

# By sleeping, it ensures that the CGI bucket is left in the brigade
# (the first 8K will be morphed into a HEAP bucket), and hence *must*
# be setaside correctly when the byterange filter calls
# ap_save_brigade().

# Without the fix for PR 31247, the STDOUT content does not get
# consumed as expected, so the server will deadlock as it tries to
# consume STDERR after script execution in mod_cgi, whilst the script
# tries to write to STDOUT.  So close STDERR to avoid that.

close STDERR;

print "Content-type: text/plain\n\n";

print "x"x8192;

sleep 1;

print "x"x8192;

