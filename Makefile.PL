require 5.006;

use strict;
use warnings;

use Getopt::Long qw(GetOptions);
use Config;

use constant PERL_PATH => $Config{perlpath}; # XXX
use constant WIN32 => ($^O =~ /Win32/);

if (WIN32) {
    push @ARGV, '--with-perl=' . PERL_PATH;
    my @args = (PERL_PATH, 'win32/Configure.pl', @ARGV);
    print "@args\n";
    system(@args) == 0 or die "system @args failed: $?";
}
else {
    my @opts = qw(with-apache2-apxs with-perl with-apache2-src
                  with-apr-config with-apu-config);

    my %opts;
    
# grab from @ARGV only the options that we expect
    GetOptions(\%opts, (map "$_=s", @opts),
               qw/enable-maintainer-mode enable-perl-glue disable-perl-glue/);

    $opts{"with-perl"} = PERL_PATH;
    $opts{"enable-perl-glue"} = "on" unless
        exists $opts{"disable-perl-glue"} or exists $opts{"enable-perl-glue"};

    my $opts = join " ", map {"--$_=$opts{$_}"} keys %opts;
    exec "./configure --enable-perl-glue $opts";
}
