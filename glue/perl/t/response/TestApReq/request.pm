package TestApReq::request;

use strict;
use warnings FATAL => 'all';
use APR;
use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();
use Apache::Connection;

sub handler {
    my $r = shift;
    my $req = Apache::Request->new($r);

    $req->content_type('text/plain');

    my $test  = $req->args('test');
    my $method = $r->method;

    if ($test eq 'param') {
        my $value = $req->param('value');
        $req->print($value);
    }
    elsif ($test eq 'upload') {
        my ($upload) = values %{$req->upload};
#        unlink("/home/joe/tmp/foo");
        my $bb = $upload->bb;
        while (my $b = $bb->first) {
            $b->read(my $buffer);
            $r->print($buffer);
            $b->remove;
        }
#        $upload->link("/home/joe/tmp/foo");
    }

    return 0;
}
1;
__END__
