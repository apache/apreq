use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestConfig;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC);
use constant WIN32 => Apache::TestConfig::WIN32;

my @key_len = (5, 100, 305);
my @key_num = (5, 15, 26);
my @keys    = ('a'..'z');

my @big_key_len = (100, 500, 5000, 10000);
my @big_key_num = (5, 15, 25);
my @big_keys    = ('a'..'z');

plan tests => 5 + @key_len * @key_num + @big_key_len * @big_key_num;

my $script = WIN32 ? '/cgi-bin/cgi_test.exe' : '/cgi-bin/cgi_test';
my $line_end = WIN32 ? "\r\n" : "\n";

ok t_cmp("\tfoo => 1$line_end", 
         POST_BODY("$script?foo=1"), "simple get");
ok t_cmp("\tfoo => ?$line_end\tbar => hello world$line_end", 
         GET_BODY("$script?foo=%3F&bar=hello+world"), "simple get");

my $filler = "0123456789" x 5; # < 64K

my $body = POST_BODY("/$script", content => 
                     "aaa=$filler;foo=1;bar=2;filler=$filler");
ok t_cmp("\tfoo => 1$line_end\tbar => 2$line_end", 
         $body, "simple post");

$body = POST_BODY("/$script?foo=1", content => 
                  "intro=$filler&bar=2&conclusion=$filler");
ok t_cmp("\tfoo => 1$line_end\tbar => 2$line_end", 
         $body, "simple post");

$body = UPLOAD_BODY("/$script?foo=0", content => $filler);
ok t_cmp("\tfoo => 0$line_end", 
         $body, "simple upload");

# GET
for my $key_len (@key_len) {
    for my $key_num (@key_num) {
        my @query = ();
        my $len = 0;

        for my $key (@keys[0..($key_num-1)]) {
            my $pair = "$key=" . 'd' x $key_len;
            $len += length($pair) - 1;
            push @query, $pair;
        }
        my $query = join ";", @query;

        t_debug "# of keys : $key_num, key_len $key_len";
        my $body = GET_BODY "$script?$query";
        ok t_cmp($len,
                 $body,
                 "GET long query");
    }

}

# POST
for my $big_key_len (@big_key_len) {
    for my $big_key_num (@big_key_num) {
        my @query = ();
        my $len = 0;

        for my $big_key (@big_keys[0..($big_key_num-1)]) {
            my $pair = "$big_key=" . 'd' x $big_key_len;
            $len += length($pair) - 1;
            push @query, $pair;
        }
        my $query = join ";", @query;

        t_debug "# of keys : $big_key_num, big_key_len $big_key_len";
        $body = POST_BODY($script, content => "$query;$filler");
        ok t_cmp($len,
                 $body,
                 "POST big data");
    }

}
