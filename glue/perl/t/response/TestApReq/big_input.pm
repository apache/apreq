package TestApReq::big_input;

use strict;
use warnings FATAL => 'all';
use Apache::Request ();
use Apache::RequestIO;
use Apache::RequestRec;

sub handler {
    my $r = shift;
    my $req = Apache::Request->new($r);
    my $len = 0;

    for ($req->param) {
        $len += length($_) + length($req->param($_)) + 2; # +2 ('=' and '&')
    }
    $len--; # the stick with two ends one '&' char off

    $req->content_type('text/plain');
    $req->print($len);

    return 0;
}

1;

__END__
