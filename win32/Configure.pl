#!C:/Perl/bin/perl
use strict;
use warnings;
use FindBin;
use Getopt::Long;
require File::Spec;
use Cwd;
require Win32;
use ExtUtils::MakeMaker;
use File::Basename;
use Archive::Tar;
use File::Path;
use LWP::Simple;
my ($apache, $debug, $help, $no_perl, $perl);
my $result = GetOptions( 'with-apache2=s' => \$apache,
			 'debug' => \$debug,
			 'help' => \$help,
                         'with-perl=s' => \$perl,
                         'disable-perl-glue' => \$no_perl,
                       );
usage() if $help;

my @path_ext;
path_ext();
$apache ||= search();
my $apreq_home = Win32::GetShortPathName($FindBin::Bin);
$apreq_home =~ s!/?win32$!!;
$apreq_home =~ s!/!\\!g;

my $doxygen = which('doxygen');
my $cfg = $debug ? 'Debug' : 'Release';

check_depends();

if (not $no_perl and $] >= 5.008) {
    my @args = ($^X, "$apreq_home/build/xsbuilder.pl", 'run', 'run');
    chdir "$apreq_home/glue/perl";
    system(@args) == 0 or die "system @args failed: $?";
    chdir $apreq_home;
}

open(my $make, '>Makefile') or die qq{Cannot open Makefile: $!};
print $make <<"END";
# Microsoft Developer Studio Generated NMAKE File.

APREQ_HOME=$apreq_home
CFG=$cfg
APACHE=$apache
PERL=$^X
RM_F=\$(PERL) -MExtUtils::Command -e rm_f
DOXYGEN_CONF=\$(APREQ_HOME)\\build\\doxygen.conf.win32

END

print $make $_ while (<DATA>);

my $apxs = which('apxs');
unless ($apxs) {
    $apxs = fetch_apxs() ? which('apxs') : '';
}

my $test = << 'END';
TEST: $(LIBAPREQ) $(MOD)
	$(MAKE) /nologo /f $(CFG_HOME)\$(TESTALL).mak CFG="$(TESTALL) - Win32 $(CFG)" APACHE="$(APACHE)" APREQ_HOME="$(APREQ_HOME)"
        set PATH=%PATH%;$(APACHE)\bin;$(APREQ_HOME)\win32\libs
        cd $(LIBDIR) && $(TESTALL).exe -v
        cd $(APREQ_HOME)
	$(MAKE) /nologo /f $(CFG_HOME)\$(CGITEST).mak CFG="$(CGITEST) - Win32 $(CFG)" APACHE="$(APACHE)" APREQ_HOME="$(APREQ_HOME)"
        copy $(LIBDIR)\test_cgi.exe $(APREQ_ENV)\t\cgi-bin\test_cgi.exe
        cd $(APREQ_HOME)
END

my $clean = << 'END';
CLEAN:
        cd $(LIBDIR)
        $(RM_F) *.pch *.exe *.exp *.lib *.pdb *.ilk *.idb *.so *.dll *.obj
        cd $(APREQ_ENV)
        $(PERL) t\TEST.PL -clean
        cd $(APREQ_HOME)
!IF EXIST("$(PERLGLUE)\Makefile")
        cd $(PERLGLUE)
        $(MAKE) /nologo clean
        cd $(APREQ_HOME)
!ENDIF
END

if ($apxs) {
    $test .= << "END";
        cd \$(APREQ_ENV)
        \$(PERL) t\\TEST.PL -apxs $apxs
        cd \$(APREQ_HOME)
END
    $clean .= << 'END';
        cd $(APREQ_ENV)
        $(PERL) t\TEST.PL -clean
        cd $(APREQ_HOME)
END
}

print $make "\n", $test, "\n";
print $make "\n", $clean, "\n";

if ($doxygen) {
    print $make <<"END";

docs:   \$(DOXYGEN_CONF)
	cd \$(APREQ_HOME)
        "$doxygen" \$(DOXYGEN_CONF)
	cd \$(APREQ_HOME)

END

    my $bin_abspath = Win32::GetShortPathName(dirname(which('doxysearch')));
    open(my $conf, "$apreq_home/build/doxygen.conf") 
        or die "Cannot read $apreq_home/build/doxygen.conf: $!";
    open(my $win32_conf, ">$apreq_home/build/doxygen.conf.win32")
        or die "Cannot write to $apreq_home/build/doxygen.conf.win32: $!";
    while (<$conf>) {
        s/^(PERL_PATH\s+=\s+).*/$1"$^X"/;
        s/^(BIN_ABSPATH\s+=\s+).*/$1$bin_abspath/;
        print $win32_conf $_;
    }
    close $conf;
    close $win32_conf;
}

close $make;
# generate_defs();

print << "END";

A Makefile has been generated in $apreq_home.
You can now run

  nmake               - builds the libapreq2 library
  nmake test          - runs the supplied tests
  nmake mod_apreq     - builds mod_apreq
  nmake clean         - clean
  nmake install       - install the C libraries
  nmake perl_glue     - build the perl glue
  nmake perl_test     - test the perl glue
  nmake perl_install  - install the perl glue
  nmake help          - list the nmake targets
END
    if ($doxygen) {
print << 'END';
  nmake docs          - builds documents

END
}

my @args = ($^X, "$apreq_home/win32/apreq2_win32.pl", 
            "--with-apache2=$apache");
system(@args) == 0 or warn "system @args failed: $?";

sub usage {
    print <<'END';

 Usage: perl Configure.pl [--with-apache2=C:\Path\to\Apache2] [--debug]
        perl Configure.pl --help

Options:

  --with-apache2=C:\Path\to\Apache2 : specify the top-level Apache2 directory
  --debug                           : build a debug version
  --disable-perl-glue               : skip building the perl glue
  --help                            : print this help message

With no options specified, an attempt will be made to find a suitable 
Apache2 directory, and if found, a non-debug version will be built.

END
    exit;
}

sub search {
    my $apache;
    if (my $bin = which('Apache')) {
       (my $candidate = dirname($bin)) =~ s!bin$!!;
        if (-d $candidate and check($candidate)) {
            $apache = $candidate;
        }
    }
    unless ($apache and -d $apache) {
        $apache = prompt("Please give the path to your Apache2 installation:",
                         $apache);
    }
    die "Can't find a suitable Apache2 installation!" 
        unless ($apache and -d $apache and check($apache));
    
    $apache = Win32::GetShortPathName($apache);
    $apache =~ s!\\!/!g;
    $apache =~ s!/$!!;
    my $ans = prompt(qq{\nUse "$apache" for your Apache2 directory?}, 'yes');
    unless ($ans =~ /^y/i) {
        die <<'END';

Please run this configuration script again, and give
the --with-apache2=C:\Path\to\Apache2 option to specify
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
    die qq{"$apache" does not appear to be version 2.x}
        unless $vers =~ m!Apache/2.\d!;
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
    return unless $program;
    my @extras = ();
    my @drives = drives();
    if (@drives > 0) {
        for my $drive (@drives) {
            for ('Apache2', 'Program Files/Apache2',
                 'Program Files/Apache Group/Apache2') {
                my $bin = File::Spec->catpath($drive, $_, 'bin');
                push @extras, $bin if (-d $bin);
            }
        }
    }
    my @a = map {File::Spec->catfile($_, $program) } 
        (File::Spec->path(), @extras);
    for my $base(@a) {
        return $base if -x $base;
        for my $ext (@path_ext) {
            return "$base.$ext" if -x "$base.$ext";
        }
    }
    return;
}

sub generate_defs {
    my $preamble =<<'END';
LIBRARY

EXPORTS

END
    chdir "$apreq_home/env";
    my $match = qr{^apreq_env};
    my %fns = ();
    open my $fh, "<mod_apreq.c"
        or die "Cannot open env/mod_apreq.c: $!";
    while (<$fh>) {
        next unless /^APREQ_DECLARE\([^\)]+\)\s*(\w+)/;
        my $fn = $1;
        $fns{$fn}++ if $fn =~ /$match/;
    }
    close $fh;
    open my $def, ">../win32/mod_apreq.def"
        or die "Cannot open win32/mod_apreq.def: $!";
    print $def $preamble;
    print $def $_, "\n" for (sort keys %fns);
    close $def;
}

sub check_depends {
    my $rep = $] < 5.008 ? 
        'http://theoryx5.uwinnipeg.ca/ppmpackages' :
            'http://theoryx5.uwinnipeg.ca/ppms';
    for my $mod (qw(ExtUtils::XSBuilder Apache::Test)) {
        eval "require $mod";
        if ($@) {
            if ($] < 5.008 and $mod eq 'ExtUtils::XSBuilder') {
                warn "Perl 5.8 or higher is required for the Perl glue\n";
                next;
            }
            print "I could not find $mod. ";
            my $ans = prompt('Would you like to install it?', 'yes');
            next unless $ans =~ /^y/i;
            (my $ppd = $mod) =~ s!(\w+)::(\w+)!$1-$2.ppd!;
            my @args = ('ppm', 'install', "$rep/$ppd");
            system(@args) == 0
                or warn "system @args failed: $?";
        }
    }
}

sub fetch_apxs {
    print << 'END';

I could not find an apxs utility on your system, which is
needed to run tests in the env/ subdirectory. The apxs 
utiltity (and apr-config and apu-config utilties) have not
yet been ported to Apache2 on Win32, but a development port
is available, which I can install for you, if you like.

END

    my $ans = prompt('Install apxs?', 'yes');
    return unless $ans =~ /^y/i;
    my $file = 'apxs_win32.tar.gz';
    my $remote = 'http://perl.apache.org/dist/win32-bin/' . $file;
    print "Fetching $remote ... ";
    unless (is_success(getstore($remote, $file))) {
        warn "Download of $remote failed";
        return;
    }
    print " done!\n";
    
    my $arc = Archive::Tar->new($file, 1);
    $arc->extract($arc->list_files());
    my $dir = 'apxs';
    unless (-d $dir) {
        warn "Unpacking $file failed";
        return;
    }
    print "chdir $dir\n";
    chdir $dir or do {
        warn "chdir to $dir failed: $!";
        return;
    };
    my @args = ($^X, 'Configure.pl');
    print "@args\n\n";
    system(@args) == 0 or do {
         warn "system @args failed: $?";
         return;
     };
    chdir '..';
    rmtree($dir, 1, 1) or warn "rmtree of $dir failed: $!";
    print "unlink $file\n";
    unlink $file or warn "unlink of $file failed: $!";
    return 1;
}

__DATA__

LIBAPREQ=libapreq2
TESTALL=testall
CGITEST=test_cgi
MOD=mod_apreq

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

CFG_HOME=$(APREQ_HOME)\win32
LIBDIR=$(CFG_HOME)\libs
PERLGLUE=$(APREQ_HOME)\glue\perl
APACHE_LIB=$(APACHE)\lib
APREQ_ENV=$(APREQ_HOME)\env

ALL : "$(LIBAPREQ)"

$(LIBAPREQ):
	$(MAKE) /nologo /f $(CFG_HOME)\$(LIBAPREQ).mak CFG="$(LIBAPREQ) - Win32 $(CFG)" APACHE="$(APACHE)" APREQ_HOME="$(APREQ_HOME)"

$(MOD): $(LIBAPREQ)
	$(MAKE) /nologo /f $(CFG_HOME)\$(MOD).mak CFG="$(MOD) - Win32 $(CFG)" APACHE="$(APACHE)" APREQ_HOME="$(APREQ_HOME)"

PERL_GLUE: $(MOD)
        cd $(PERLGLUE)
	$(PERL) Makefile.PL
        $(MAKE) /nologo
        cd $(APREQ_HOME)

PERL_TEST: $(MOD)
        cd $(PERLGLUE)
!IF !EXIST("$(PERLGLUE)\Makefile")
	$(PERL) Makefile.PL
!ENDIF
        $(MAKE) /nologo test
        cd $(APREQ_HOME)

PERL_INSTALL: $(MOD)
        cd $(PERLGLUE)
!IF !EXIST("$(PERLGLUE)\Makefile")
	$(PERL) Makefile.PL
!ENDIF
        $(MAKE) /nologo install
        cd $(APREQ_HOME)

INSTALL: $(LIBAPREQ)
        cd $(LIBDIR)
!IF EXIST("$(LIBDIR)\$(MOD).so")
	copy "$(MOD).so" "$(APACHE)\modules\$(MOD).so"
	copy "$(MOD).lib" "$(APACHE_LIB)\$(MOD).lib"
!ENDIF
!IF EXIST("$(LIBDIR)\$(LIBAPREQ).lib")
	copy "$(LIBAPREQ).lib" "$(APACHE_LIB)\$(LIBAPREQ).lib"
!ENDIF
!IF EXIST("$(LIBDIR)\$(LIBAPREQ).dll")
        copy "$(LIBAPREQ).dll" "$(APACHE)\bin\$(LIBAPREQ).dll"
!ENDIF
        cd $(APREQ_HOME)

HELP:
	@echo nmake               - builds the libapreq2 library
	@echo nmake test          - runs the supplied tests
	@echo nmake mod_apreq     - builds mod_apreq
	@echo nmake clean         - clean
	@echo nmake install       - install the C libraries
	@echo nmake perl_glue     - build the perl glue
	@echo nmake perl_test     - test the perl glue
	@echo nmake perl_install  - install the perl glue
	@echo nmake docs          - builds documents (requires doxygen)
