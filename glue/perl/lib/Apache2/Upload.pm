package Apache2::Upload;
use Apache2::Request;
push our @ISA, qw/APR::Request::Param/;
no strict 'refs';
for (qw/slurp type size link tempname fh io filename/) {
    *{$_} = *{"APR::Request::Param::upload_$_"}{CODE};
}
sub Apache2::Request::upload {
    my $req = shift;
    my $body = $req->body;
    $body->param_class(__PACKAGE__);
    my @uploads;
    if (@_) {
        @uploads = grep $_->upload, $body->get(@_);
        return wantarray ? @uploads : $uploads[0];
    }

    return map { $_->upload ? $_->name : () } values %$body
        if wantarray;

   return $body->uploads($req->pool);

}

*bb = *APR::Request::Param::upload;

1;
