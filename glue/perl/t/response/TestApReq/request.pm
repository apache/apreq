package TestApReq::request;

use strict;
use warnings FATAL => 'all';

use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();
use Apache::Connection;
use Apache::Upload;
use APR::Pool;
use APR::PerlIO;

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
    elsif ($test eq 'bb') {
        my ($upload) = $req->upload("HTTPUPLOAD");
        my $bb = $upload->bb;
        my $e = $bb->first;
        while ($e) {
            $e->read(my $buf);
            $req->print($buf);
            $e = $bb->next($e);
        }
    }
    elsif ($test eq 'tempname') {
        my $upload = $req->upload("HTTPUPLOAD");
        my $name = $upload->tempname;
        open my $fh, "<:APR", $name, $upload->pool or die "Can't open $name: $!";
        $r->print(<$fh>);
    }
    elsif ($test eq 'fh') {
        my $upload = $req->upload(($req->upload)[0]);
        my $fh = $upload->fh;
        read $upload->fh, my $fh_contents, $upload->size;
        $upload->slurp(my $slurp_data);
        die 'fh contents != slurp data'
            unless $fh_contents eq $slurp_data;
        read $fh, $fh_contents, $upload->size;
        die '$fh contents != slurp data'
            unless $fh_contents eq $slurp_data;
        seek $fh, 0, 0;
        $r->print(<$fh>);
    }
    elsif ($test eq 'io') {
        my $upload = $req->upload(($req->upload)[0]);
        my $io = $upload->io;
        read $upload->io, my $io_contents, $upload->size;
        $upload->slurp(my $slurp_data);
        die "io contents != slurp data" unless $io_contents eq $slurp_data;
        my $bb = $upload->bb;
        my $e = $bb->first;
        my $bb_contents = "";
        while ($e) {
            $e->read(my $buf);
            $bb_contents .= $buf;
            $e = $bb->next($e);
        }
        die "io contents != brigade contents" 
            unless $io_contents eq $bb_contents;
        $r->print(<$io>);
    }
    elsif ($test eq 'bad') {
        eval {my $q = $req->args('query')};
        if (ref $@ eq "Apache::Request::Error") {
            $req->upload("HTTPUPLOAD")->slurp(my $data);
            $req->print($data);
        }
    }
    elsif ($test eq 'type') {
        my $upload = $req->upload("HTTPUPLOAD");
        die "content-type mismatch" 
            unless $upload->info->{"Content-Type"} eq $upload->type;
        $r->print($upload->type);
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
