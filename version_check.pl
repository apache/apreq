# version_check.pl tool version
use strict;
use warnings 'FATAL';

my ($tool, $path) = @ARGV;
$path = $tool unless defined $path;

sub exe_version { scalar qx/$path -v/ }
sub gnu_version { scalar qx/$path --version/ }

sub xsb_version { 
    package ExtUtils::XSBuilder;
    require ExtUtils::XSBuilder;
    return our $VERSION;
}

sub a_t_version {
    require Apache::Test;
    $Apache::Test::VERSION;
}

sub mp2_version {
    eval {
        require Apache2;
        require mod_perl;
        $mod_perl::VERSION;
    } or do {
        require mod_perl;
        $mod_perl::VERSION;
    };
}

my %prereq = (
                libtool => { test => \&gnu_version, version => "1.4.2" },
               autoconf => { test => \&gnu_version, version => "2.53"  },
               automake => { test => \&gnu_version, version => "1.6.3" },
                apache2 => { test => \&exe_version, version => "2.0.46"},
                    apr => { test => \&gnu_version, version => "0.9.4" },
                    apu => { test => \&gnu_version, version => "0.9.4" },
                   perl => { test => \&gnu_version, version => "5.6.1" },
              xsbuilder => { test => \&xsb_version, version => "0.23"  },
                    mp2 => { test => \&mp2_version, version => "1.99_09"},
                doxygen => { test => \&gnu_version, version => "1.3"   },
            apache_test => { test => \&a_t_version, version => "1.03"  },
             );

if (@ARGV == 0) {
    printf "%12s:  %s\n", $_, $prereq{$_}->{version} for sort keys %prereq;
    exit 0;
}

die "$0 failed: unknown tool '$tool'.\n" unless $prereq{$tool};
my $version = $prereq{$tool}->{version};
my @version = split /\./, $version;

$_ = $prereq{$tool}->{test}->();
die "$0 failed: no version_string found in '$_'.\n" unless /(\d[.\d]+)/;
my $saw = $1;
my @saw = split /\./, $saw;
$_ = 0 for @saw[@saw .. $#version]; # ensure @saw has enough entries
for (0.. $#version) {
    last if $version[$_] < $saw[$_];
    die <<EOM if $version[$_] > $saw[$_];
$0 failed: $tool version $saw unsupported ($version or greater is required).
EOM
}
print "$tool: $saw ok\n";
exit 0;
