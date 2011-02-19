package APR::Request::Magic;
require base;
our $VERSION = "2.14";
my $ctx;
eval { local $ENV{PERL_DL_NONLAZY} = 1; require APR::Request::Apache2; };
if ($@) {
    require APR::Request::CGI;
    base->import("APR::Pool");
    *handle = sub { $ctx ||= bless APR::Pool->new; APR::Request::CGI->handle($ctx, @_) };
    our $MODE = "CGI";
}
else {
    require Apache2::RequestUtil;
    base->import("Apache2::RequestRec");
    *handle = sub { APR::Request::Apache2->handle(Apache2::RequestUtil->request, @_) };
    our $MODE = "Apache2";
}

1;

# Notes:
#
# 1) the way to use this module is trivial:
#
#    use APR::Request::Magic;
#
#    my $apreq = APR::Request::Magic->handle(@typical_args_sans_the_first_one);
#    # do stuff with $apreq which is an APR::Request object
#
# 2) Be sure PerlOptions +GlobalRequest is set for mp2.
