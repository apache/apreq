#! perl
# version_check.pl tool version
use strict;
use warnings 'FATAL';

my %vstring = (
                libtool => sub { scalar qx/libtool --version/ },
               autoconf => sub {scalar qx/autoconf --version/ },
               automake => sub {scalar qx/automake --version/ },
);

my $usage = "Usage: version_check.pl tool version_string";

die "$usage: exactly two arguments are required." unless @ARGV == 2;
my ($tool, $version) = @ARGV;
my $test = $vstring{$tool};
die "$usage: tool '$tool' (first argument) is unsupported." unless $test;
die <<EOM unless $version =~ /^\d[.\d]*$/;
$usage: version_string '$version' (second argument) must match /^\d+[.\d]*$/
EOM
my @version = split /\./, $version;

for ($test->()) {
    die "$0 failed: no version_string found in '$_'." unless /(\d[.\d]+)/;
    my $saw = $1;
    my @saw = split /\./, $saw;
    $_ = 0 for @saw[@saw .. $#version]; # ensure @saw has enough entries
    for (0.. $#version) {
        exit 0 if $version[$_] < $saw[$_];
        die <<EOM if $version[$_] > $saw[$_];
$0 failed: $tool version $saw is not supported ($version or greater is required).
EOM
    }
}

exit 0;
