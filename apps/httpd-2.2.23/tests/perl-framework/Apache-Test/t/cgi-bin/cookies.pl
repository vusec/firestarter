#!/usr/bin/perl
# WARNING: this file is generated, do not edit
# generated on Wed May 27 03:01:20 2020
# 01: Apache-Test/lib/Apache/TestConfig.pm:1007
# 02: Apache-Test/lib/Apache/TestConfig.pm:1099
# 03: Apache-Test/lib/Apache/TestMM.pm:142
# 04: ./Makefile.PL:50
# 05: /usr/share/perl/5.22/ExtUtils/MakeMaker.pm:241
# 06: /usr/share/perl/5.22/ExtUtils/MakeMaker.pm:228
# 07: /usr/share/perl/5.22/ExtUtils/MakeMaker.pm:228
# 08: /usr/share/perl/5.22/ExtUtils/MakeMaker.pm:728
# 09: /usr/share/perl/5.22/ExtUtils/MakeMaker.pm:67
# 10: Makefile.PL:55

BEGIN { eval { require blib && blib->import; } }
#!perl -wT

use strict;

use CGI;
use CGI::Cookie;

my %cookies = CGI::Cookie->fetch;
my $name = 'ApacheTest';
my $c = ! exists $cookies{$name}
    ? CGI::Cookie->new(-name=>$name, -value=>time)
    : '';

print "Set-Cookie: $c\n" if $c;
print "Content-Type: text/plain\n\n";
print ($c ? 'new' : 'exists'), "\n";
