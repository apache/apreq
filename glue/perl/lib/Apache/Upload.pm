package Apache::Upload;
use Apache::Request;
push our @ISA, qw/APR::Request::Param/;
no strict 'refs';
for (qw/slurp type size link tempname/) {
    *{$_} = *{"APR::Request::Param::upload_$_"}{CODE};
}
sub Apache::Request::upload {
    my $req = shift;
    my $body = $req->body;
    $body->param_class(__PACKAGE__);
    my @uploads = grep $_->upload,
                  @_ ? $body->get(@_) : values %$body;
    wantarray ? @uploads : $uploads[0];
}


1;
