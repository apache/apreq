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
        if ($upload->size != length $data) {
            $req->print("Size mismatch: size() reports ", $upload->size,
                        " but slurp() length is ", length $data, "\n");
        }
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
        my $upload = $req->upload(($req->upload)[0]);
        my $fh = $upload->fh;
        return unless $upload->info->{"Content-Type"} eq $upload->type;
        $r->print(<$fh>);
    }
    elsif ($test eq 'tempname') {
        my $upload = $req->upload("HTTPUPLOAD");
        open my $fh, "<", $upload->tempname or die $!;
        $r->print(<$fh>);
    }
    elsif ($test eq 'bad') {
        eval {my $q = $req->args('query')};
        if (ref $@ eq "Apache::Request::Error") {
            $req->upload("HTTPUPLOAD")->slurp(my $data);
            $req->print($data);
        }
    }
    elsif ($test eq 'disable_uploads') {
        $req->config(DISABLE_UPLOADS => 1);
        eval {my $upload = $req->upload('HTTPUPLOAD')};
        if (ref $@ eq "Apache::Request::Error") {
            $req->print("ok");
        }
    }

    return 0;
}
1;
__END__
