package TestApReq::big_input;

use strict;
use warnings FATAL => 'all';
use Apache::Request ();
use Apache::RequestIO;
use Apache::RequestRec;
use Apache::Connection;
use APR;
use APR::Brigade;
use APR::Bucket;
use Apache::Filter;

sub handler {
    my $r = shift;
    my $apr = Apache::Request->new($r);
    my $f = $r->input_filters;
    my $method = $r->method;
    my $bb = APR::Brigade->new($r->pool,
                               $r->connection->bucket_alloc);
    my $len = 0;

    # ~ $apr->parse ???
    if ($method eq "POST") {
        do {
            $bb->destroy;
            $f->get_brigade($bb, 0, 0, 8000);
        } while $bb->last && !$bb->last->is_eos;
    }

    for ($apr->param) {
        $len += length($_) + length($apr->param($_)) + 2; # +2 ('=' and '&')
    }
    $len--; # the stick with two ends one '&' char off

    $apr->content_type('text/plain');
    $apr->print($len);

    return 0;
}

1;

__END__
