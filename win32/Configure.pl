#!C:/Perl/bin/perl
use strict;
use warnings;
use Getopt::Long;
my ($apache, $debug, $help);
my $result = GetOptions( 'with-apache=s' => \$apache,
			 'debug' => \$debug,
			 'help' => \$help,
                       );
usage() if $help;

$apache ||= search();
check($apache);
$apache =~ s!/!\\!g;

my $cfg = $debug ? 'Debug' : 'Release';
open my $make, '>Makefile' or die qq{Cannot open Makefile: $!};
print $make <<"END";
# Microsoft Developer Studio Generated NMAKE File.

CFG=$cfg
APACHE=$apache
END

print $make $_ while (<DATA>);
close $make;

print << 'END';

A Makefile has been generated. You can now run

  nmake               - builds the libapreq library
  nmake test          - runs the supplied tests
  nmake mod_apreq     - builds mod_apreq
  nmake libapreq_cgi  - builds libapreq_cgi
  nmake clean         - clean

END


sub usage {
    print <<'END';

 Usage: perl Configure.pl [--with-apache=C:\Path\to\Apache] [--debug]
        perl Configure.pl --help

Options:

  --with-apache=C:\Path\to\Apache2 : specify the top-level Apache2 directory
  --debug                          : build a debug version
  --help                           : print this help message

With no options specified, an attempt will be made to find a suitable 
Apache2 directory, and if found, a non-debug version will be built.

END
    exit;
}

sub search {
    my $apache;
  SEARCH: {
        for my $drive ('C' .. 'Z') {
            for my $p ('Apache2', 'Program Files/Apache2', 
                       'Program Files/Apache Group/Apache2') {
                if (-d "$drive:/$p/bin") {
                    $apache = "$drive:/$p";
                    last SEARCH;
                }
            }
        }
    }
    require ExtUtils::MakeMaker;
    ExtUtils::MakeMaker->import('prompt');
    unless (-d $apache) {
        $apache = prompt("Where is your Apache2 installed?", $apache);
    }
    die "Can't find a suitable Apache2 directory!" unless -d $apache;

    my $ans = prompt(qq{Use "$apache" for your Apache2 directory?}, 'yes');
    unless ($ans =~ /^y/i) {
        die <<'END';

Please run this configuration script again, and give
the --with-apache=C:\Path\to\Apache2 option to specify
the desired top-level Apache2 directory.

END
    }

    require Win32;
    return Win32::GetShortPathName($apache);
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

!IF  "$(CFG)" == "Release"
INTDIR=.\Release
OUTDIR=.\Release
!ELSE
INTDIR=.\Debug
OUTDIR=.\Debug
!ENDIF

ALL : "$(LIBAPREQ)"

$(LIBAPREQ):
	$(MAKE) /nologo /f $(LIBAPREQ).mak CFG="$(LIBAPREQ) - Win32 $(CFG)" APACHE="$(APACHE)"

CLEAN:
	$(MAKE) /nologo /f $(LIBAPREQ).mak CFG="$(LIBAPREQ) - Win32 $(CFG)" APACHE="$(APACHE)" clean
	$(MAKE) /nologo /f $(TESTALL).mak CFG="$(TESTALL) - Win32 $(CFG)" APACHE="$(APACHE)" clean
	$(MAKE) /nologo /f $(MOD).mak CFG="$(MOD) - Win32 $(CFG)" APACHE="$(APACHE)" clean
	$(MAKE) /nologo /f $(CGI).mak CFG="$(CGI) - Win32 $(CFG)" APACHE="$(APACHE)" clean

TEST: $(LIBAPREQ)
	$(MAKE) /nologo /f $(TESTALL).mak CFG="$(TESTALL) - Win32 $(CFG)" APACHE="$(APACHE)"
	cd $(INTDIR) && $(TESTALL).exe -v

$(MOD): $(LIBAPREQ)
	$(MAKE) /nologo /f $(MOD).mak CFG="$(MOD) - Win32 $(CFG)" APACHE="$(APACHE)"

$(CGI): $(LIBAPREQ)
	$(MAKE) /nologo /f $(CGI).mak CFG="$(CGI) - Win32 $(CFG)" APACHE="$(APACHE)"

