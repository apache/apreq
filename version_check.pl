# version_check.pl tool version
use strict;
use warnings 'FATAL';
use Getopt::Long qw/GetOptions/;
GetOptions(\my %opts, "version=s"),
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

my %cvs = (
                libtool => { test => \&gnu_version, version => "1.4.2" },
               autoconf => { test => \&gnu_version, version => "2.53"  },
               automake => { test => \&gnu_version, version => "1.6.3" },
                doxygen => { test => \&gnu_version, version => "1.3"   },
          );

my %build = (
                apache2 => { test => \&exe_version, version => "2.0.46"},
                    apr => { test => \&gnu_version, version => "0.9.4",
                             comment => "bundled with apache2 2.0.46"},
                    apu => { test => \&gnu_version, version => "0.9.4",
                             comment => "bundled with apache2 2.0.46"},
                   perl => { test => \&gnu_version, version => "5.6.1" },
            );

my %perl_glue = (
         "Apache::Test" => { test => \&a_t_version, version => "1.04",
                             comment => "bundled with mod_perl 1.99_09"},
  "ExtUtils::XSBuilder" => { test => \&xsb_version, version => "0.23"  },
              mod_perl  => { test => \&mp2_version, version => "1.99_09"},
                );

sub print_prereqs ($$) {
    my ($preamble, $prereq) = @_;
    print $preamble, "\n";
    for (sort keys %$prereq) {
        my ($version, $comment) = @{$prereq->{$_}}{qw/version comment/};
        if ($opts{version} or not defined $comment) {
            $comment = "";
        }
        else {
            $comment = "   ($comment)";
        }

        printf "%30s:  %s$comment\n", $_, $version;
    }
    print "\n";
}

if (@ARGV == 0) {
    if ($opts{version}) {
        print <<EOT;
name: libapreq2
version: $opts{version}
installdirs: site
distribution_type: module
generated_by: $0
EOT
        print_prereqs "requires:", \%perl_glue;
    }
    else {
        print "=" x 50, "\n";
        print_prereqs "Build system (core C API) prerequisites\n", \%build;
        print "=" x 50, "\n";
        print_prereqs "Perl glue (Apache::Request) prerequisites\n", \%perl_glue;
        print "=" x 50, "\n";
        print_prereqs "Additional prerequisites for httpd-apreq-2 cvs builds\n",\%cvs;
    }
    exit 0;
}

my %prereq = (%cvs, %build, %perl_glue);
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
