use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestConfig;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC);
use constant WIN32 => Apache::TestConfig::WIN32;

plan tests => 2;

my $script = WIN32 ? '/cgi-bin/cgi_test.exe' : '/cgi-bin/cgi_test';
my $line_end = WIN32 ? "\r\n" : "\n";

ok t_cmp("ARGS:$line_end\tfoo => 1$line_end", 
         GET_BODY("$script?foo=1"), "simple get");
ok t_cmp("ARGS:$line_end\tbar => hello world$line_end\tfoo => ?$line_end", 
         GET_BODY("$script?bar=hello+world;foo=%3F"), "simple get");
