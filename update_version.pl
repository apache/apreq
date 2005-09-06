#!/usr/bin/perl
# update_version.pl - update all package version strings for apreq

use strict;
use warnings FATAL => "all";

my $version = shift;


# .pod
#  glue/perl/xsbuilder
#  "This manpage documents version (\d+.\S+)"

use File::Find;
my @pod;
find(sub { push @pod, $File::Find::name if /\.pod$/ },
     qw(glue/perl/xsbuilder));

my $substitution = "s/(?<=This manpage documents version )\\S+/$version/";

system "perl -i -ple '$substitution' @pod";


# .pm
#  glue/perl/lib/Apache2/*
#  our $VERSION = "[^"]+"
my @pm;
find(sub { push @pm, $File::Find::name if /\.pm$/ },
     qw(glue/perl/lib/Apache2));

system "perl -i -ple '$substitution' @pm";

#
#  configure.ac -

my $pattern1 = "(?<=AC_INIT\\(Apache HTTP Server Request Library, )(\\S+)(?=,)";
my $pattern2 = "(?<=AM_INIT_AUTOMAKE\\(libapreq2, )(\\S+)(?=\\))";

system "perl -i -ple 's/$pattern1/$version/ or s/$pattern2/$version/' configure.ac";
