use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 4;
my $location = "/TestApReq__inherit";
my @response = split/\r?\n/, GET_BODY($location, Cookie=>"apache=2");
ok t_cmp("method => GET", $response[0], "inherit method");
ok t_cmp("cookie => apache=2", $response[1], "inherit cookie");
ok t_cmp("DESTROYING TestApReq::inherit object", $response[2], "first object cleanup");
ok t_cmp("DESTROYING TestApReq::inherit object", $response[3], "second object cleanup");
