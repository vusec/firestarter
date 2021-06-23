use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestRequest;
use Apache::TestUtil;

plan tests => 1, need_php;

# Regression test for http://bugs.php.net/bug.php?id=19840

ok t_cmp((GET_BODY "/php/getenv.php"),
         "GET",
         "getenv(REQUEST_METHOD)"
);
