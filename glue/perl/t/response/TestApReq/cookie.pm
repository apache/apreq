package TestApReq::cookie;

use strict;
use warnings FATAL => 'all';

use Apache::Request ();
use Apache::RequestIO;
use Apache::RequestRec;
use Apache::Connection;

use Apache::Cookie ();
use Apache::Request ();

sub handler {
    my $r = shift;
    my $req = Apache::Request->new($r);
    my %cookies = Apache::Cookie->fetch($r);

    $r->content_type('text/plain');
    my $test = $req->param('test');
    my $key  = $req->param('key');

    if ($key and $cookies{$key}) {
        if ($test eq "bake") {
            $cookies{$key}->bake;
        }
        elsif ($test eq "bake2") {
            $cookies{$key}->bake2;
        }
        $r->print($cookies{$key}->value);
    }
    else {
        my @expires;
        @expires = ("expires", $req->param('expires')) if $req->param('expires');

        my $cookie = Apache::Cookie->new($r, name => "foo",
                                            value => "bar", @expires);
        if ($test eq "bake") {
            $cookie->bake;
        }
        elsif ($test eq "bake2") {
            $cookie->bake2;
        }
        $r->print($cookie->value);
    }


    return 0;
}

1;

__END__
