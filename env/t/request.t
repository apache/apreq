use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC);

plan tests => 11;

foreach my $location ('/apreq_request_test', '/apreq_access_test') {

    ok t_cmp("ARGS:\n\ttest => 1\n", 
            GET_BODY("$location?test=1"), "simple get");

    ok t_cmp("ARGS:\n\ttest => 2\nBODY:\n\tHTTPUPLOAD => b\n",
            UPLOAD_BODY("$location?test=2", content => "unused"), 
            "simple upload");
}

ok t_cmp(403, GET_RC("/apreq_access_test"), "access denied");

my $filler = "0123456789" x 6400; # < 64K
my $body = POST_BODY("/apreq_access_test?foo=1;", 
                     content => "bar=2&quux=$filler;test=6&more=$filler");
ok t_cmp(<<EOT, $body, "prefetch credentials");
ARGS:
\tfoo => 1
BODY:
\tbar => 2
\tquux => $filler
\ttest => 6
\tmore => $filler
EOT

ok t_cmp(403, GET_RC("/apreq_redirect_test"), "missing 'test' authorization");

foreach my $location ('/apreq_request_test', '/apreq_access_test') {
    ok t_cmp("ARGS:\n\ttest => redirect\n", 
            GET_BODY("/apreq_redirect_test?test=ok&location=$location%3Ftest=redirect"), 
            "redirect GET");

    $body = POST_BODY("/apreq_redirect_test?location=$location%3Ffoo=bar", content => 
            "quux=$filler;test=redirect+with+prefetch;more=$filler");

    ok t_cmp(<<EOT, $body, "redirect with prefetch");
ARGS:
\tfoo => bar
BODY:
\tquux => $filler
\ttest => redirect with prefetch
\tmore => $filler
EOT
}
