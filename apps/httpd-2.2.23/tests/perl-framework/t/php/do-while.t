use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestRequest;

plan tests => 1, need_php;

my $expected = "321";

my $result = GET_BODY "/php/do-while.php";
ok $result eq $expected;
