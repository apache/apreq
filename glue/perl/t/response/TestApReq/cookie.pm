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
    my $apr = Apache::Request->new($r);
    my %cookies = Apache::Cookie->fetch($r);

    $r->content_type('text/plain');
    my $test = $apr->param('test');
    my $key  = $apr->param('key');


    if ($cookies{$key}) {
        if ($test eq "bake") {
            $cookies{$key}->bake;
        }
        elsif ($test eq "bake2") {
            $cookies{$key}->bake2;
        }
        $r->print($cookies{$key}->value);
    }


    return 0;
}

1;

__END__
