package TestApReq::request;

use strict;
use warnings FATAL => 'all';
use APR;
use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();
use Apache::Connection;
use APR::Brigade;
use APR::Bucket;
use Apache::Filter;

sub handler {
    my $r = shift;
    my $req = Apache::Request->new($r);

    $req->content_type('text/plain');

    my $test  = $req->args('test');
    my $method = $r->method;

    if ($method eq "POST") {
        # ~ $apr->parse ???
        my $f = $r->input_filters;
        my $bb = APR::Brigade->new($r->pool,
                                   $r->connection->bucket_alloc);
        while ($f->get_brigade($bb,0,0,8000) == 0) {
            last if $bb->last->is_eos;
            $bb->destroy;
        }
        $bb->destroy;
    }

    if ($test eq 'param') {
        my $value = $req->param('value');
        $req->print($value);
    }
    elsif ($test eq 'upload') {
        my ($upload) = values %{$req->upload};
#        unlink("/tmp/foo");
        my $bb = $upload->bb;
        while (my $b = $bb->first) {
            $b->read(my $buffer);
            $r->print($buffer);
            $b->remove;
        }
#        $upload->link("/tmp/foo");
    }

    return 0;
}
1;
__END__
