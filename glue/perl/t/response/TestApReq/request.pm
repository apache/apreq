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
    elsif ($test eq 'upload') {
        my ($upload) = values %{$req->upload};
        $upload->slurp(my $data);
        $r->print($data);
    }

    return 0;
}
1;
__END__
