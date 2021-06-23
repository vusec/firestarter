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
use File::Temp qw/:POSIX/;

my $caroot = $ENV{SSL_CA_ROOT};

if (! -d $caroot) {
    print <<EOT
Status: 500 Internal Server Error
Content-Type: text/plain

Cannot find CA root at "$ENV{SSL_CA_ROOT}"
EOT
    ;
    print STDERR "SSL_CA_ROOT env var not set or can't find CA root.\n";
    exit(1);
}

chdir($caroot);

my $filein = tmpnam();
my $fileout = tmpnam();

# Enable slurp mode (read all lines at once)
local $/;

# Copy STDIN to $filein, which will be used as input for openssl
open(IN, '>', "$filein") or die "Could not open file '$filein' for write: $!";
binmode IN;
print IN <STDIN>;
close(IN);

my $cmd = 'openssl ocsp -CA certs/ca.crt'.
    ' -index index.txt'.
    ' -rsigner certs/server.crt'.
    ' -rkey keys/server.pem'.
    ' -reqin ' . $filein .
    ' -respout ' . $fileout;
system($cmd);

# Check system result
my $err = '';
if ($? == -1) {
    my $err = "failed to execute '$cmd': $!\n";
}
elsif ($? & 127) {
    my $err = sprintf("child '$cmd' died with signal %d, %s coredump\n",
        ($? & 127),  ($? & 128) ? 'with' : 'without');
}
else {
    my $rc = $? >> 8;
    my $err = "child '$cmd' exited with value $rc\n" if $rc;
}

unlink($filein);

if ($err ne '') {
    print <<EOT
Status: 500 Internal Server Error
Content-Type: text/plain

$err
EOT
    ;
    print STDERR $err;
    exit(1);
}

print <<EOT
Content-Type: application/ocsp-response

EOT
;

# Copy openssl result from $fileout to STDOUT
open(OUT, '<', "$fileout") or die "Could not open file '$fileout' for read: $!";
binmode OUT;
print <OUT>;
close(OUT);
unlink($fileout);
