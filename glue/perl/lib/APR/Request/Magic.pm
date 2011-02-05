package APR::Request::Magic;
require base;
my $ctx;
eval { local $ENV{PERL_DL_NONLAZY} = 1; require APR::Request::Apache2; };
if ($@) {
    require APR::Request::CGI;
    require APR::Pool;
    base->import("APR::Pool");
    *handle = sub { APR::Request::CGI->handle(@_) };
    *new = sub { $ctx ||= bless APR::Pool->new, shift; return $ctx };
    our $MODE = "CGI";
}
else {
    require Apache2::RequestRec;
    require Apache2::RequestUtil;
    base->import("Apache2::RequestRec");
    *handle = sub { APR::Request::Apache2->handle(@_) };
    *new = sub { bless Apache2::RequestUtil->request, shift };
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
#    my $apreq_ctx    = APR::Request::Magic->new;
#    my $apreq_handle = $apreq_ctx->handle(@typical_args_without_the_first_one);
#    # do stuff with $apreq_handle which is an APR::Request object
