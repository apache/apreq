#!perl
use strict;
use Apache::src ();
#use lib qw(lib blib/lib blib/arch);
eval 'require Apache::Cookie' or die $@;
#warn "No CGI::Cookie" and skip_test unless have_module "CGI::Cookie";
#warn "$@:No Apache::Cookie" and skip_test unless have_module "Apache::Cookie";

#unless (Apache::src->mmn_eq) {
#    skip_test if not $Is_dougm;
#}

my $ua = LWP::UserAgent->new;
my $cookie = "one=bar-one&a; two=bar-two&b; three=bar-three&c";
my $url = "http://localhost:$ENV{PORT}/request-cookie.pl";
my $request = HTTP::Request->new('GET', $url);
$request->header(Cookie => $cookie);
my $response = $ua->request($request, undef, undef); 
print $response->content;
 
