require 5.006;

use strict;
use warnings;

use Getopt::Long qw(GetOptions);
use Config;

use constant PERL_PATH => $Config{perlpath}; # XXX
use constant WIN32 => ($^O =~ /Win32/);

sub test_prereq {
    system (PERL_PATH, "build/version_check.pl", @_) == 0
        or die "Please upgrade $_[0] first.";
}

test_prereq perl => PERL_PATH;
test_prereq "apache_test";
test_prereq "mp2";

if (WIN32) {
    push @ARGV, '--with-perl=' . PERL_PATH;
    my @args = (PERL_PATH, 'win32/Configure.pl', @ARGV);
    print "@args\n";
    system(@args) == 0 or die "system @args failed: $?";
}
else {
    my @opts = qw(with-apache2-apxs with-apache2-src
                  with-apr-config with-apu-config apxs);

    my @flags = qw/enable-maintainer-mode enable-perl-glue disable-perl-glue/;
    my %opts;
# grab from @ARGV only the options that we expect
    GetOptions(\%opts, (map "$_=s", @opts), @flags);

    $opts{"with-perl"} = PERL_PATH;
    my $opts = "";
    $opts .= "--enable-maintainer-mode " if $opts{"enable-maintainer-mode"};
    $opts .= "--enable-perl-glue " unless exists $opts{"disable-perl-glue"};

    delete @opts{@flags};
    $opts{"with-apache2-apxs"} = delete $opts{apxs}
        if exists $opts{apxs} and not exists $opts{"with-apache2-apxs"};

    $opts .= join " ", map {"--$_=$opts{$_}"} keys %opts;
    print "./configure $opts\n";
    exec  "./configure $opts";
}
