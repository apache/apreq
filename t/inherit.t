use strict;
use Apache::test;
use Apache::src ();
use Cwd qw(fastcwd);

require HTTP::Request::Common; 
require CGI;

$HTTP::Request::Common::VERSION ||= '1.00'; #-w 
unless ($CGI::VERSION >= 2.39 and  
	$HTTP::Request::Common::VERSION >= 1.08) {
    print "CGI.pm: $CGI::VERSION\n";
    print "HTTP::Request::Common: $HTTP::Request::Common::VERSION\n";
    skip_test;
} 

my $PWD = fastcwd;
my @binary = "$PWD/book.gif";

my $tests = 1;

print "1..$tests\n";
my $i = 0;
my $ua = LWP::UserAgent->new;

use DirHandle ();

inherit_test();

sub inherit_test {
    my $cv = sub { HTTP::Request::Common::GET(shift) };
    my $url = "http://localhost:$ENV{PORT}/request-inherit.pl";
    my $response = $ua->request($cv->($url));
    my $page = $response->content;
    print $response->as_string unless $response->is_success;
    my $expect = <<EOF;
method => GET
hostname => localhost
EOF
    my $ok = $page eq $expect;
    test ++$i, $ok;
    print $response->as_string unless $ok;
}
