package Apache::Request;
use APR::Request::Apache2;
use Apache::RequestRec;
push our @ISA, qw/Apache::RequestRec APR::Request::Apache2/;

sub param {
    return &APR::Request::args, &APR::Request::body
        if wantarray;

    my ($req, $key) = @_;

    return APR::Request::params($req, $req->pool)
        if @_ == 1;

    return APR::Request::param($req, $key);
}


package Apache::Upload;
use APR::Request::Param;
push our @ISA, qw/APR::Request::Param/;

1;
