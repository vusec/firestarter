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

# numbers.pl
# program rewrite map for mod_rewrite testing
#
$|=1;
my %map = (	1	=>	'one',
		2	=>	'two',
		3	=>	'three',
		4	=>	'four',
		5	=>	'five',
		6	=>	'six'	);

while (<STDIN>) {
	chomp;

	print STDERR "GOT: ->$_<-\n";
	my $m = $map{$_};
	print STDERR "MAPPED: ->$_<-\n";

	if ($m) {
		print STDOUT "$m\n";
	} else {
		print STDOUT "NULL";
	}
}
close (LOG);
