#!perl
use strict;
use Config;
use Getopt::Long;
require Win32;
use ExtUtils::MakeMaker;
use warnings;
use FindBin;

BEGIN {
    die 'This script is intended for Win32' unless $^O =~ /Win32/i;
}

my $license = <<'END';
# ====================================================================
# The Apache Software License, Version 1.1
#
# Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The end-user documentation included with the redistribution,
#    if any, must include the following acknowledgment:
#       "This product includes software developed by the
#        Apache Software Foundation (http://www.apache.org/)."
#    Alternately, this acknowledgment may appear in the software itself,
#    if and wherever such third-party acknowledgments normally appear.
#
# 4. The names "Apache" and "Apache Software Foundation" must
#    not be used to endorse or promote products derived from this
#    software without prior written permission. For written
#    permission, please contact apache@apache.org.
#
# 5. Products derived from this software may not be called "Apache",
#    nor may "Apache" appear in their name, without prior written
#    permission of the Apache Software Foundation.
#
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
# ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# ====================================================================
#
# This software consists of voluntary contributions made by many
# individuals on behalf of the Apache Software Foundation.  For more
# information on the Apache Software Foundation, please see
# <http://www.apache.org/>.

# APR-util script designed to allow easy command line access to APR-util
# configuration parameters.

END

my $file = 'apreq2-config.pl';
my $apreq_home = Win32::GetShortPathName($FindBin::Bin);
$apreq_home =~ s!/?win32$!!;
$apreq_home =~ s!/!\\!g;
require "$apreq_home/win32/util.pl";

my ($prefix, $help);
GetOptions('with-apache2=s' => \$prefix, 'help' => \$help) or usage($0);
usage($0) if $help;

unless (defined $prefix and -d $prefix) {
    $prefix = prompt("Please give the path to your Apache2 installation:",
		     $prefix);
}
die "Can't find a suitable Apache2 installation!" 
    unless (-d $prefix and check($prefix));

$prefix = Win32::GetShortPathName($prefix);
$prefix =~ s!\\!/!g;

my $src_version = "$apreq_home/src/apreq_version.h";
my $apache_version = "$prefix/include/apreq_version.h";

my $apreq_version = -e $src_version ? $src_version : $apache_version;
open(my $inc, $apreq_version)
    or die "Cannot open $apreq_version: $!";
my %vers;
while (<$inc>) {
    if (/define\s+APREQ_(MAJOR|MINOR|PATCH)_VERSION\s+(\d+)/) {
        $vers{$1} = $2;
    }
}
close $inc;
my $dotted = "$vers{MAJOR}.$vers{MINOR}.$vers{PATCH}";
my $src_dir = -d $apreq_home ? $apreq_home : '';

my %apreq_args = (APREQ_MAJOR_VERSION => $vers{MAJOR},
                  APREQ_DOTTED_VERSION => $dotted,
                  APREQ_LIBNAME => 'libapreq2.lib',
                  prefix => $prefix,
                  exec_prefix => $prefix,
                  bindir => "$prefix/bin",
                  libdir => "$prefix/lib",
                  datadir => $prefix,
                  installbuilddir => "$prefix/build",
                  includedir => "$prefix/include",
                
                  CC => $Config{cc},
                  CPP => $Config{cpp},
                  LD => $Config{ld},
                  SHELL => $ENV{comspec},
                  CPPFLAGS => '',
                  CFLAGS => q{ /nologo /MD /W3 /O2 /D "WIN32" /D "_WINDOWS" /D "NDEBUG" },
                  LDFLAGS => q{ kernel32.lib /nologo /subsystem:windows /dll /machine:I386 },
                  LIBS => '',
                  EXTRA_INCLUDES => '',
                  APREQ_SOURCE_DIR => $src_dir,
                  APREQ_SO_EXT => $Config{dlext},
                  APREQ_LIB_TARGET => '',
               );

my $apreq_usage = << 'EOF';
Usage: apreq2-config [OPTION]

Known values for OPTION are:
  --prefix[=DIR]    change prefix to DIR
  --bindir          print location where binaries are installed
  --includedir      print location where headers are installed
  --libdir          print location where libraries are installed
  --cc              print C compiler name
  --cpp             print C preprocessor name and any required options
  --ld              print C linker name
  --cflags          print C compiler flags
  --cppflags        print cpp flags
  --includes        print include information
  --ldflags         print linker flags
  --libs            print additional libraries to link against
  --srcdir          print APR-util source directory
  --installbuilddir print APR-util build helper directory
  --link-ld         print link switch(es) for linking to APREQ
  --apreq-so-ext    print the extensions of shared objects on this platform
  --version         print the APR-util version as a dotted triple
  --help            print this help

When linking, an application should do something like:
  APREQ_LIBS="\`apreq2-config --link-ld --libs\`"

An application should use the results of --cflags, --cppflags, --includes,
and --ldflags in their build process.
EOF

my $full = "$prefix/bin/$file";
open(my $fh, ">$full") or die "Cannot open $full: $!";
print $fh <<"END";
#!$^X
use strict;
use warnings;
use Getopt::Long;

$license
sub usage {
    print << 'EOU';
$apreq_usage
EOU
    exit(1);
}

END

foreach my $var (keys %apreq_args) {
    print $fh qq{my \${$var} = q[$apreq_args{$var}];\n};
}
print $fh $_ while <DATA>;
close $fh;

my @args = ('pl2bat', $full);
system(@args) == 0 or die "system @args failed: $?";
print qq{apreq2-config.bat has been created under $prefix/bin.\n\n};

__DATA__

my %opts = ();
GetOptions(\%opts,
           'prefix:s',
           'bindir',
           'includedir',
           'libdir',
           'cc',
           'cpp',
           'ld',
           'cflags',
           'cppflags',
           'includes',
           'ldflags',
           'libs',
           'srcdir',
           'installbuilddir',
           'link-ld',
           'apreq-so-ext',
           'version',
           'help'
          ) or usage();

usage() if ($opts{help} or not %opts);

if (exists $opts{prefix} and $opts{prefix} eq "") {
    print qq{$prefix\n};
    exit(0);
}
my $user_prefix = defined $opts{prefix} ? $opts{prefix} : '';
my $flags = '';

SWITCH : {
    local $\ = "\n";
    $opts{bindir} and do {
        print $user_prefix ? "$user_prefix/bin" : $bindir;
        last SWITCH;
    };
    $opts{includedir} and do {
        print $user_prefix ? "$user_prefix/include" : $includedir;
        last SWITCH;
    };
    $opts{libdir} and do {
        print $user_prefix ? "$user_prefix/lib" : $libdir;
        last SWITCH;
    };
    $opts{installbuilddir} and do {
        print $user_prefix ? "$user_prefix/build" : $installbuilddir;
        last SWITCH;
    };
    $opts{srcdir} and do {
        print $APREQ_SOURCE_DIR;
        last SWITCH;
    };
    $opts{cc} and do {
        print $CC;
        last SWITCH;
    };
    $opts{cpp} and do {
        print $CPP;
        last SWITCH;
    };
    $opts{ld} and do {
        print $LD;
        last SWITCH;
    };
    $opts{cflags} and $flags .= " $CFLAGS ";
    $opts{cppflags} and $flags .= " $CPPFLAGS ";
    $opts{includes} and do {
        my $inc = $user_prefix ? "$user_prefix/include" : $includedir;
        $flags .= qq{ /I"$inc" $EXTRA_INCLUDES };
    };
    $opts{ldflags} and $flags .= " $LDFLAGS ";
    $opts{libs} and $flags .= " $LIBS ";
    $opts{'link-ld'} and do {
        my $libpath = $user_prefix ? "$user_prefix/lib" : $libdir;
        $flags .= qq{ /libpath:"$libpath" $APREQ_LIBNAME };
    };
    $opts{'apreq-so-ext'} and do {
        print $APREQ_SO_EXT;
        last SWITCH;
    };
    $opts{version} and do {
        print $APREQ_DOTTED_VERSION;
        last SWITCH;
    };
    print $flags if $flags;
}
exit(0);
