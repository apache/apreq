package APR::Request::Magic;
require base;
eval { require APR::Request::Apache2; };
if ($@) {
    require APR::Request::CGI;
    require APR::Pool;
    base->import("APR::Pool");
    *handle = *APR::Request::CGI::handle;
    *new = sub { bless APR::Pool->new, shift; };
    return 1;
}
require Apache2::RequestRec;
require Apache2::RequestUtil;
base->import("Apache2::RequestRec");
*handle = *APR::Request::Apache2::handle;
*new = sub { bless Apache2::RequestUtil->request, shift; }

1;
