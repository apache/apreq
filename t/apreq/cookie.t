use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY);
use HTTP::Cookies;

plan tests => 1, &need_lwp;

my $location = "/TestApReq__cookie";

{
    # basic param() test
    my $test  = 'basic';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    ok t_cmp(qq{"$value"},
             GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $test);
}

