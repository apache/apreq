use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 1;
my $location = "/TestApReq::inherit";
ok t_cmp(<< 'VALUE', GET_BODY($location), "inheritance");
method => GET
VALUE

