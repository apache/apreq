package TestAPI::error;

use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Apache2;

sub handler {
    my $r = shift;
    plan $r, tests => 1;

    my $req = APR::Request::Apache2->new($r);
    ok $req->isa("APR::Request");

    # XXX export some constants, and test apreq_xs_strerror

    return 0;
}


1;
