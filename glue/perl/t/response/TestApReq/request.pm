package TestApReq::request;

use strict;
use warnings FATAL => 'all';

use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();
use Apache::Connection;
use Apache::Upload;

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
    elsif ($test eq 'slurp') {
        my ($upload) = values %{$req->upload};
        $upload->slurp(my $data);
        $req->print($data);
    }
    elsif ($test eq 'bb_read') {
        my ($upload) = $req->upload("HTTPUPLOAD");
        my $bb = $upload->bb;
        my $e;
        while ($e = $bb->first) {
            $e->read(my $buf);
            $req->print($buf);
            $e->remove;
        }
    }
    elsif ($test eq 'fh_read') {
        my (undef, $upload) = each %{$req->upload};
        my $fh = $upload->fh;
        $r->print(<$fh>);
    }

    return 0;
}
1;
__END__
