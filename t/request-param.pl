#!perl
use strict;
use Apache::test;

my $r = Apache->request;
$r->send_http_header('text/plain');

eval {
    require Apache::Request;
};

unless (have_module "Apache::Request" and Apache::Request->can('upload')) {
    print "1..0\n";
    print $@ if $@;
    print "$INC{'Apache/Request.pm'}\n";
    return;
}

my $apr = Apache::Request->new($r);

for ($apr->param) {
    my(@v) = $apr->param($_);
    print "param $_ => ", join ",", @v;
    print $/;
}

#for my $table ($apr->query_params, $apr->post_params) {
#    my ($k,$v) = each %$table;
#    print "param $k => $v$/";
#}

