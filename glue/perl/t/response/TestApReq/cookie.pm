package TestApReq::cookie;

use strict;
use warnings;# FATAL => 'all';

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
    warn "apache => $cookies{apache}";
    my $test = $apr->param('test');
    my $key  = $apr->param('key');

#    return DECLINED unless defined $test;

    if ($test eq 'basic') {
        if ($cookies{$key}) {
            $r->print($cookies{$key}->value);
        }
    }


    return 0;
}

1;

__END__
