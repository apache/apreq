package Apache::Request;
use APR::Request::Apache2;
use Apache::RequestRec;
push our @ISA, qw/Apache::RequestRec APR::Request::Apache2/;

sub param {
    return &APR::Request::args, &APR::Request::body
        if wantarray;

   return &APR::Request::param
       if @_ == 2;

    my $req = shift;
    return APR::Request::params($req, $req->pool);

}


package Apache::Upload;
use APR::Request::Param;
push our @ISA, qw/APR::Request::Param/;

1;
