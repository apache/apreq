package Apache::Request;
use APR::Request::Apache2;
use Apache::RequestRec;
push our @ISA, qw/Apache::RequestRec APR::Request::Apache2/;


package Apache::Upload;
use APR::Request::Param;
push our @ISA, qw/APR::Request::Param/;

1;
