use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 11, have_lwp;

my $location = "/TestApReq__request";
#print GET_BODY $location;

{
    # basic param() test
    my $test  = 'param';
    my $value = '42.5';
    ok t_cmp($value,
             GET_BODY("$location?test=$test&value=$value"),
             "basic param");
}

for my $test (qw/slurp bb_read fh_read tempfile bad;query=string%%/) {
    # upload a string as a file
    my $value = 'DataUpload' x 100_000;
    my $result = UPLOAD_BODY("$location?test=$test", content => $value); 
    ok t_cmp($value, $result, "basic upload");
    my $i;
    for ($i = 0; $i < length $value; ++$i) {
        last if substr($value,$i,1) ne substr($result,$i,1);
    }

    ok t_cmp(length($value), $i, "basic upload length");    
}
