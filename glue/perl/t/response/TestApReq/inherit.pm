package TestApReq::inherit;
use base 'Apache::Request';
use strict;
use warnings FATAL => 'all';
use APR;
use Apache::RequestRec;
use Apache::RequestIO;
use Devel::Peek;
sub handler {
    my $r = shift;
    $r->content_type('text/plain');
    $r = __PACKAGE__->new($r); # tickles refcnt bug in apreq-1
    Dump($r);
    die "Wrong package: ", ref $r unless $r->isa('TestApReq::inherit');
    $r->print(sprintf "method => %s\n", $r->method);
    return 0;
}

1;
