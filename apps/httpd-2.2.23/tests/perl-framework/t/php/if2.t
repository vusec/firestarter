use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestRequest;

## Testing user-defined function falling out of an If into another

plan tests => 1, need_php4;

my $expected = "1\n";

my $result = GET_BODY "/php/if2.php";
ok $result eq $expected;
