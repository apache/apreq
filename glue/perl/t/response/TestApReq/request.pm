package TestApReq::request;

use strict;
use warnings FATAL => 'all';
use APR;
use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();

sub handler {
    my $r = shift;
    my $req = Apache::Request->new($r);

    $req->content_type('text/plain');

    my $test  = $req->param('test');
    my $value = $req->param('value');

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

    if ($test eq 'param') {
        $req->print($value);
    }
    elsif ($test eq 'upload') {
        my ($upload) = values %{$req->upload};
        $bb = $upload->bb;
        my $b = $bb->first;
        while ($b = $bb->first) {
            $b->read(my $buffer);
            $r->print($buffer);
            $b->remove;
        }
    }

    return 0;
}
1;
__END__
