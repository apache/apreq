use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC);

plan tests => 6;

foreach my $location ('/apreq_request_test', '/apreq_access_test') {

    ok t_cmp("ARGS:\n\ttest => 1\n", 
            GET_BODY("$location?test=1"), "simple get");

    ok t_cmp("ARGS:\n\ttest => 2\nBODY:\n\tHTTPUPLOAD => b\n",
            UPLOAD_BODY("$location?test=2", content => "unused"), 
            "simple upload");
}

ok t_cmp(403, GET_RC("/apreq_access_test"), "access denied");

# XXX 8000 filler chars bombs out- why?
my $filler = "1234567" x 1000;
my $body = POST_BODY("/apreq_access_test?foo=1;", 
                     content => "bar=2&quux=$filler;test=6");
ok t_cmp(<<EOT, $body, "prefetch credentials");
ARGS:
\tfoo => 1
BODY:
\tbar => 2
\tquux => $filler
\ttest => 6
EOT
