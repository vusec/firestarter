use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestRequest;

## nested functions test.

plan tests => 1, need_php;

my $expected = "4 Hello 4";

my $result = GET_BODY "/php/func6.php";
ok $result eq $expected;
