use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC);

my $num_tests = 16;
$num_tests *= 2 if Apache::Test::have_ssl();
plan tests => $num_tests, have_lwp;
my $scheme = "http";

START_TESTS:
Apache::TestRequest::scheme($scheme);

foreach my $location ('/apreq_request_test', '/apreq_access_test') {

    ok t_cmp(GET_BODY("$location?test=1"), "ARGS:\n\ttest => 1\n", 
             "simple get");

    ok t_cmp(UPLOAD_BODY("$location?test=2", content => "unused"), 
             "ARGS:\n\ttest => 2\nBODY:\n\tHTTPUPLOAD => b\n",
             "simple upload");
}

ok t_cmp(GET_RC("/apreq_access_test"), 403, "access denied");

my $filler = "0123456789" x 6400; # < 64K
my $body = POST_BODY("/apreq_access_test?foo=1;", 
                     content => "bar=2&quux=$filler;test=6&more=$filler");
ok t_cmp($body, <<EOT, "prefetch credentials");
ARGS:
\tfoo => 1
BODY:
\tbar => 2
\tquux => $filler
\ttest => 6
\tmore => $filler
EOT

ok t_cmp(GET_RC("/apreq_redirect_test"), 403, "missing 'test' authorization");

foreach my $location ('/apreq_request_test', '/apreq_access_test') {
    ok t_cmp(GET_BODY("/apreq_redirect_test?test=ok&location=$location%3Ftest=redirect"), 
             "ARGS:\n\ttest => redirect\n", 
             "redirect GET");

    $body = POST_BODY("/apreq_redirect_test?location=$location%3Ffoo=bar", content => 
            "quux=$filler;test=redirect+with+prefetch;more=$filler");

    ok t_cmp($body, <<EOT, "redirect with prefetch");
ARGS:
\tfoo => bar
BODY:
\tquux => $filler
\ttest => redirect with prefetch
\tmore => $filler
EOT
}


# internal redirect to plain text files (which are non-apreq requests)

my $index_html = do {local (@ARGV,$/) = "t/htdocs/index.html"; <> };
link "t/htdocs/index.html", "t/htdocs/index.txt";
ok t_cmp(GET_BODY("/apreq_redirect_test?test=redirect_index_txt_GET&location=/index.txt"), $index_html, 
        "redirect /index.txt (GET)");

ok t_cmp(POST_BODY("/apreq_redirect_test?test=redirect_index_txt_POST",
        content => "quux=$filler;location=/index.txt;foo=$filler"), $index_html, 
        "redirect /index.txt (POST)");


# output filter tests

sub filter_content ($) {
   my $body = shift;      
   $body =~ s/^.*--APREQ OUTPUT FILTER--\s+//s;
   return $body;
}

ok t_cmp(GET_RC("/index.html"), 200, "/index.html");
ok t_cmp(filter_content GET_BODY("/index.html?test=13"),
         "ARGS:\n\ttest => 13\n", "output filter GET");

ok t_cmp(filter_content POST_BODY("/index.html?test=14", content => 
         "post+data=foo;more=$filler;test=output+filter+POST"), 
         <<EOT,
ARGS:
\ttest => 14
BODY:
\tpost data => foo
\tmore => $filler
\ttest => output filter POST
EOT
          "output filter POST");

if (Apache::Test::have_ssl() and $scheme ne 'https') {    
    $scheme = 'https';
    goto START_TESTS;
}
