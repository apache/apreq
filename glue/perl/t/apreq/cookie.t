use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY);
use HTTP::Cookies;

plan tests => 3;

my $location = "/TestApReq__cookie";

{
    my $test  = 'netscape';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{$key=$value};
    ok t_cmp($value,
             GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $test);
}
{
    my $test  = 'rfc';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    ok t_cmp(qq{"$value"},
             GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $test);
}
{
    my $test  = 'encoded value with space';
    my $key   = 'apache';
    my $value = 'okie dokie';
    my $cookie = "$key=" . join '',
        map {/ / ? '+' : sprintf '%%%.2X', ord} split //, $value;
    ok t_cmp($value,
             GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $test);
}
