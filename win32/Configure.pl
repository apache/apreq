#!C:/Perl/bin/perl
use strict;
use warnings;
use Getopt::Long;
require File::Spec;
require Win32;
use ExtUtils::MakeMaker;
use File::Basename;
my ($apache, $debug, $help, $no_perl);
my $result = GetOptions( 'with-apache=s' => \$apache,
			 'debug' => \$debug,
			 'help' => \$help,
                         'without-perl' => \$no_perl,
                       );
usage() if $help;

my @path_ext;
path_ext();
$apache ||= search();

my $doxygen = which('doxygen');
my $cfg = $debug ? 'Debug' : 'Release';

unless ($no_perl) {
    eval {require ExtUtils::XSBuilder;};
    if ($@) {
        warn "ExtUtils::XSBuilder must be installed for Perl glue\n";
    }
    else {
        my @args = ($^X, '../../build/xsbuilder.pl', 'run', 'run');
        chdir '../glue/perl';
        system(@args) == 0 or die "system @args failed: $!";
        chdir '../../win32';
    }
}

open(my $make, '>Makefile') or die qq{Cannot open Makefile: $!};
print $make <<"END";
# Microsoft Developer Studio Generated NMAKE File.

CFG=$cfg
APACHE=$apache
PERL=$^X
RM_F=\$(PERL) -MExtUtils::Command -e rm_f

END

print $make $_ while (<DATA>);

if ($doxygen) {
    print $make <<"END";

docs: 
	cd ..
	"$doxygen" build\\doxygen.conf.win32
	cd win32

END

    my $bin_abspath = Win32::GetShortPathName(dirname(which('doxysearch')));
    open(my $conf, '../build/doxygen.conf') 
        or die "Cannot read ../build/doxygen.conf: $!";
    open(my $win32_conf, '>../build/doxygen.conf.win32')
        or die "Cannot write to ../build/doxygen.conf.win32: $!";
    while (<$conf>) {
        s/^(PERL_PATH\s+=\s+).*/$1"$^X"/;
        s/^(BIN_ABSPATH\s+=\s+).*/$1$bin_abspath/;
        print $win32_conf $_;
    }
    close $conf;
    close $win32_conf;
}

close $make;
generate_defs();

print << 'END';

A Makefile has been generated. You can now run

  nmake               - builds the libapreq library
  nmake test          - runs the supplied tests
  nmake mod_apreq     - builds mod_apreq
  nmake libapreq_cgi  - builds libapreq_cgi
  nmake clean         - clean
  nmake perl_glue     - build the perl glue
  nmake perl_test     - test the perl glue
END
    if ($doxygen) {
print << 'END';
  nmake docs          - builds documents

END
}

sub usage {
    print <<'END';

 Usage: perl Configure.pl [--with-apache=C:\Path\to\Apache2] [--debug]
        perl Configure.pl --help

Options:

  --with-apache=C:\Path\to\Apache2 : specify the top-level Apache2 directory
  --debug                          : build a debug version
  --without-perl                   : skip initializing the perl glue
  --help                           : print this help message

With no options specified, an attempt will be made to find a suitable 
Apache2 directory, and if found, a non-debug version will be built.

END
    exit;
}

sub search {
    my $apache;
  SEARCH: {
        my $candidate;
        my $bin = which('Apache');
        if (my $bin = which('Apache')) {
            ($candidate = $bin) =~ s!bin$!!;
            if (-d $candidate and check($candidate)) {
                $apache = $candidate;
                last SEARCH;
            }
        }
        my @drives = drives();
        last SEARCH unless (@drives > 0);
        for my $drive (@drives) {
            for ('Apache2', 'Program Files/Apache2',
                 'Program Files/Apache Group/Apache2') {
                $candidate = File::Spec->catpath($drive, $_);
                if (-d $candidate and check($candidate)) {
                    $apache = $candidate;
                    last SEARCH;
                }
            }
        }
    }
    unless (-d $apache) {
        $apache = prompt("Please give the path to your Apache2 installation:",
                         $apache);
    }
    die "Can't find a suitable Apache2 installation!" 
        unless (-d $apache and check($apache));
    
    $apache = Win32::GetShortPathName($apache);
    $apache =~ s!\\!/!g;
    my $ans = prompt(qq{Use "$apache" for your Apache2 directory?}, 'yes');
    unless ($ans =~ /^y/i) {
        die <<'END';

Please run this configuration script again, and give
the --with-apache=C:\Path\to\Apache2 option to specify
the desired top-level Apache2 directory.

END

    }
    return $apache;
}

sub drives {
    my @drives = ();
    eval{require Win32API::File;};
    return map {"$_:\\"} ('C' .. 'Z') if $@;
    my @r = Win32API::File::getLogicalDrives();
    return unless @r > 0;
    for (@r) {
        my $t = Win32API::File::GetDriveType($_);
        push @drives, $_ if ($t == 3 or $t == 4);
    }
    return @drives > 0 ? @drives : undef;
}

sub check {
    my $apache = shift;
    die qq{No libhttpd library found under $apache/lib}
        unless -e qq{$apache/lib/libhttpd.lib};
    die qq{No httpd header found under $apache/include}
        unless -e qq{$apache/include/httpd.h};
    my $vers = qx{"$apache/bin/Apache.exe" -v};
    die qq{"$apache" does not appear to be version 2.0}
        unless $vers =~ m!Apache/2.0!;
    return 1;
}

sub path_ext {
    if ($ENV{PATHEXT}) {
        push @path_ext, split ';', $ENV{PATHEXT};
        for my $ext (@path_ext) {
            $ext =~ s/^\.*(.+)$/$1/;
        }
    }
    else {
        #Win9X: doesn't have PATHEXT
        push @path_ext, qw(com exe bat);
    }
}

sub which {
    my $program = shift;
    return undef unless $program;
    my @a = map {File::Spec->catfile($_, $program) } File::Spec->path();
    for my $base(@a) {
        return $base if -x $base;
        for my $ext (@path_ext) {
            return "$base.$ext" if -x "$base.$ext";
        }
    }
}

sub generate_defs {
    my $preamble =<<'END';
LIBRARY

EXPORTS

END
    chdir '../env';
    my $match = qr{^apreq_env};
    foreach my $file(qw(mod_apreq libapreq_cgi)) {
        my %fns = ();
        open my $fh, "<$file.c"
            or die "Cannot open env/$file.c: $!";
        while (<$fh>) {
            next unless /^APREQ_DECLARE\([^\)]+\)\s*(\w+)/;
            my $fn = $1;
            $fns{$fn}++ if $fn =~ /$match/;
        }
        close $fh;
        open my $def, ">../win32/$file.def"
            or die "Cannot open win32/$file.def: $!";
        print $def $preamble;
        print $def $_, "\n" for (sort keys %fns);
        print $def "apreq_env\n";
        close $def;
    }
    
}

__DATA__


LIBAPREQ=libapreq
TESTALL=testall
MOD=mod_apreq
CGI=libapreq_cgi

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE CFG="Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE Please run Configure.bat to specify a valid Apache directory.
!ERROR
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

LIBDIR=.\libs
PERLGLUE=..\glue\perl

ALL : "$(LIBAPREQ)"

$(LIBAPREQ):
	$(MAKE) /nologo /f $(LIBAPREQ).mak CFG="$(LIBAPREQ) - Win32 $(CFG)" APACHE="$(APACHE)"

CLEAN:
        cd $(LIBDIR)
        $(RM_F) *.pch *.exe *.exp *.lib *.pdb *.ilk *.idb *.so *.dll *.obj
        cd ..
!IF EXIST("$(PERLGLUE)\Makefile")
        cd $(PERLGLUE)
        $(MAKE) /nologo clean
        cd ..\..\win32
!ENDIF

TEST: $(LIBAPREQ)
	$(MAKE) /nologo /f $(TESTALL).mak CFG="$(TESTALL) - Win32 $(CFG)" APACHE="$(APACHE)"
        set PATH=%PATH%;$(APACHE)\bin
        cd $(LIBDIR) && $(TESTALL).exe -v
        cd ..

$(MOD): $(LIBAPREQ)
	$(MAKE) /nologo /f $(MOD).mak CFG="$(MOD) - Win32 $(CFG)" APACHE="$(APACHE)"

$(CGI): $(LIBAPREQ)
	$(MAKE) /nologo /f $(CGI).mak CFG="$(CGI) - Win32 $(CFG)" APACHE="$(APACHE)"

PERL_GLUE: $(MOD)
        cd $(PERLGLUE)
	$(PERL) Makefile.PL
        $(MAKE) /nologo
        cd ..\..\win32

PERL_TEST: $(MOD)
        cd $(PERLGLUE)
!IF !EXIST("$(PERLGLUE)\Makefile")
	$(PERL) Makefile.PL
!ENDIF
        $(MAKE) /nologo test
        cd ..\..\win32
