package Apache2::Request;
use APR::Request::Param;
use APR::Request::Apache2 qw/args/;
use Apache2::RequestRec;
push our @ISA, qw/Apache2::RequestRec APR::Request::Apache2/;

1;
