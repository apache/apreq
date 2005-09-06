sub import {
    my $class = shift;
    return unless @_;
    my $pkg = caller;
    no strict 'refs';

    for (@_) {
        *{"$pkg\::$_"} = $class->can($_)
            or die "Can't find method $_ in class $class";
    }
}

sub param_status {
    my $req = shift;
    return $req->args_status || $req->body_status if wantarray;
    return ($req->args_status, $req->body_status);
}

sub upload {
    require APR::Request::Param;
    my $req = shift;
    my $body = $req->body or return;
    $body->param_class("APR::Request::Param");
    if (@_) {
        my @uploads = grep $_->upload, $body->get(@_);
        return wantarray ? @uploads : $uploads[0];
    }

    return map { $_->upload ? $_->name : () } values %$body
        if wantarray;

   return $body->uploads($req->pool);
}

package APR::Request::Custom;
our @ISA = qw/APR::Request/;
