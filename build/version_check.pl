# version_check.pl tool version
use strict;
use warnings 'FATAL';
use Getopt::Long qw/GetOptions/;
GetOptions(\my %opts, "version=s");
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

sub tm_version {
    require Test::More;
    $Test::More::VERSION;
}

sub mm_version {
    require ExtUtils::MakeMaker;
    $ExtUtils::MakeMaker::VERSION;
}

sub mp2_version {
    eval {
        require mod_perl2;
        $mod_perl::VERSION;
    };
}

my %svn = (
                libtool => { version => "1.4.3",   test => \&gnu_version },
               autoconf => { version => "2.53",    test => \&gnu_version },
               automake => { version => "1.4.0",   test => \&gnu_version },
                doxygen => { version => "1.2",     test => \&gnu_version },
                   perl => { version => "5.6.1",   test => \&gnu_version },
  "ExtUtils::XSBuilder" => { version => "0.23",    test => \&xsb_version },
            );

my %build = (
                apache2 => { version => "2.0.46",  test => \&exe_version },
                    apr => { version => "0.9.4",   test => \&gnu_version,
                             comment => "bundled with apache2 2.0.46"    },
                    apu => { version => "0.9.4",   test => \&gnu_version,
                             comment => "bundled with apache2 2.0.46"    },
                   perl => $svn{perl},
            );

my %perl_glue = (
                  perl  => $svn{perl},
  "ExtUtils::XSBuilder" => $svn{"ExtUtils::XSBuilder"},
         "Apache::Test" => { version => "1.04",    test => \&a_t_version,
                             comment => "Win32 requires version 1.06"    },
                 # mp2 does not contain "_" in its reported version number
              mod_perl  => { version => "1.9915", test => \&mp2_version },
  "ExtUtils::MakeMaker" => { version => "6.15",    test => \&mm_version },
           "Test::More" => { version => "0.47",    test => \&tm_version },
                );

sub print_prereqs ($$) {
    my ($preamble, $prereq) = @_;
    print $preamble, "\n";
    for (sort keys %$prereq) {
        my ($version, $comment) = @{$prereq->{$_}}{qw/version comment/};
        if ($opts{version}) {
            print "  $_: $version\n";
        }
        else {
            $comment = defined $comment ? "  ($comment)" : "";
            printf "%30s:  %s%s\n", $_, $version, $comment;
        }
    }
}

if (@ARGV == 0) {

    if ($opts{version}) {      # generate META.yml file content
        print <<EOT;
--- #YAML:1.0 (see http://module-build.sourceforge.net/META-spec.html)
name: libapreq2
version: $opts{version}
license: open_source
installdirs: site
distribution_type: module
dynamic_config: 1
provides:
  Apache::Request:
    version: $opts{version}
  Apache::Cookie:
    version: $opts{version}
  Apache::Upload:
    version: $opts{version}
generated_by: $0
EOT
        my %runtime_prereqs =  (
                                mod_perl => $perl_glue{mod_perl},
                                    perl => $perl_glue{perl},
                               );
        print_prereqs "requires:", \%runtime_prereqs;
        print_prereqs "build_requires:", \%perl_glue;
    }

    else {                     # generate PREREQUISITES file content
        print "=" x 50, "\n";
        print_prereqs
            "Build system (core C API) prerequisites\n", \%build;
        print "\n", "=" x 50, "\n";
        print_prereqs
            "Perl glue (Apache::Request) prerequisites\n", \%perl_glue;
        print "\n", "=" x 50, "\n";
        print_prereqs
            "Additional prerequisites for apreq subversion builds\n", \%svn;
    }

    exit 0;
}


# test prerequisites from the command-line arguments

my %prereq = (%svn, %build, %perl_glue);
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
