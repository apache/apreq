use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY GET_HEAD);
use HTTP::Cookies;

plan tests => 5;

my $location = "/apreq_cookie_test";

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
{
    my $test  = 'bake';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = "$key=$value";
    my ($header) = GET_HEAD("$location?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie:\s+(.+)/m;

    ok t_cmp($cookie, $header, $test);
}
{
    my $test  = 'bake2';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    my ($header) = GET_HEAD("$location?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie2:\s+(.+)/m;
    ok t_cmp(qq{$key="$value"; Version=1; path="$location"}, $header, $test);
}
