package TestApReq::request;

use strict;
use warnings FATAL => 'all';
use APR;
use Apache::RequestRec;
use Apache::RequestIO;
use Apache::Request ();

sub handler {
    my $r = shift;
    my $apr = Apache::Request->new($r);

    $r->content_type('text/plain');

    my $test  = $apr->param('test');
    my $value = $apr->param('value');

#   return DECLINED unless defined $test;

    if ($test eq 'param') {
        $r->print($value);
    }
    elsif ($test eq 'upload') {
        return -1;
        my $upload = $apr->upload;
        my $fh = $upload->fh;
        local $/;
        my $data = <$fh>;
        $r->print($data);
    } 
    else {

    }

    return 0;
}
1;
__END__
