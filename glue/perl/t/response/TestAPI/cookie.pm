package TestAPI::cookie;

use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Cookie;
use APR::Request::Apache2;

sub handler {
    my $r = shift;
    plan $r, tests => 1;

    my $req = APR::Request::Apache2->new($r);
    ok not defined $req->jar;

    return 0;
}


1;
