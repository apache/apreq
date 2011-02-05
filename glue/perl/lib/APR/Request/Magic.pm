package APR::Request::Magic;
require base;
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
# 1) module authors should accept an APR::Request object in their
#    args to new() instead of pulling out an apreq handle internally.
#    the reason is that in a cgi context you'll need to share the
#    pool with your users if they are to use apreq outside of your module,
#    which is probably more trouble than it's worth.
#
# 2) the way to use this module is trivial:
#
#    use APR::Request::Magic;
#
#    my $apreq = APR::Request::Magic->handle(@typical_args_sans_the_first_one);
#    # do stuff with $apreq which is an APR::Request object
#
# 3) Be sure PerlOptions +GlobalRequest is set for mp2.
