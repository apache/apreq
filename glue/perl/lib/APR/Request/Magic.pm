package APR::Request::Magic;
require base;
eval { require APR::Request::Apache2; };
if ($@) {
    require APR::Request::CGI;
    require APR::Pool;
    base->import("APR::Pool");
    *handle = sub { APR::Request::CGI->handle(@_) };
    *new = sub { bless APR::Pool->new, shift };
}
else {
    require Apache2::RequestRec;
    require Apache2::RequestUtil;
    base->import("Apache2::RequestRec");
    *handle = sub { APR::Request::Apache2->handle(@_) };
    *new = sub { bless Apache2::RequestUtil->request, shift };
}

1;
