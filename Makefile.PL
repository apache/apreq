require 5.006;

use strict;
use warnings;

use Getopt::Long qw(GetOptions);
use Config;

use constant PERL_PATH => $Config{perlpath}; # XXX

my @opts = qw(with-apache2-apxs with-perl);

my %opts;

# grab from @ARGV only the options that we expect
GetOptions(\%opts, (map "$_=s", @opts));

$opts{"with-perl"} = PERL_PATH;

my $opts = join " ", map {"--$_=$opts{$_}"} @opts;

qx{./configure $opts};

push @ARGV, ('-apxs', $opts{"with-apache2-apxs"}) 
    if exists $opts{"with-apache2-apxs"};

qx{cd "glue/perl" && $opts{"with-perl"} Makefile.PL @ARGV};
