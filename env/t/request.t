use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 2;

my $location = '/apreq_request_test';

ok t_cmp("ARGS:\n\ttest => 1\n", 
        GET_BODY("$location?test=1"), "simple get");

ok t_cmp("ARGS:\n\ttest => 2\nBODY:\n\tHTTPUPLOAD => b\n",
        UPLOAD_BODY("$location?test=2", content => "unused"), 
        "simple upload");

