package Apache::Request;
use APR::Request::Param;
use APR::Request::Apache2 qw/args/;
use Apache::RequestRec;
push our @ISA, qw/Apache::RequestRec APR::Request::Apache2/;

1;
