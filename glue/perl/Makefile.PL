use strict;
use Apache2;
use ModPerl::MM;

use 5.005;

use Apache::Test5005compat;

use Apache::TestMM qw(test clean);
use Apache::TestReport ();
use Apache::TestSmoke ();
use Apache::TestRun ();
use Apache::TestConfigPerl ();
use File::Find qw(finddepth);
use File::Basename;
use Apache::Build;
use constant WIN32 => Apache::Build::WIN32;
use Cwd;
use ExtUtils::XSBuilder::ParseSource;

my $version = "2.XX-dev"; # DUMMY VALUE

my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base directory";
my $base_dir = $1;
my $src_dir = "$base_dir/src";
my $xs_dir = "$base_dir/glue/perl/xsbuilder";

sub slurp($$)
{
    open my $file, $_[1] or die "Can't open $_[1]: $!";
    read $file, $_[0], -s $file;
}

my ($apache_includes, $apr_libs, $apreq_libname);

if (WIN32) {
    # XXX May need fixing, Randy!
    slurp my $config => "$base_dir/configure.ac";
    $config =~ /^AC_INIT[^,]+,\s*([^,\s]+)/m or 
        die "Can't find version string";
    $version = $1;
    slurp my $make => "$base_dir/Makefile";
    $make =~ /^APACHE=(\S+)/m or
        die "Cannot find top-level Apache directory";
    my $apache_dir = $1;
    ($apache_includes = "-I$apache_dir" . '/include') =~ s!\\!/!g;
    ($apr_libs = "-L$apache_dir" . '/lib') =~ s!\\!/!g;
    $make =~ /^APR_LIB=(\S+)/m or
        die "Cannot find apr lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $make =~ /^APU_LIB=(\S+)/m or
        die "Cannot find aprutil lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $apreq_libname = 'apreq2';
}
else {
    slurp my $config => "$base_dir/config.status";
    $config =~ /^s,\@APACHE2_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes = $1;
    $config =~ /^s,\@APR_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes .= " $1";
    $config =~ /^s,\@APU_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes .= " $1";

    $config =~ m/^s,\@APR_LDFLAGS\@,([^,]+)/m or
        die "Can't find apr ldflags";
    $apr_libs = $1;
    $config =~ m/^s,\@APU_LDFLAGS\@,([^,]+)/m or
        die "Can't find apu ldflags";
    $apr_libs .= " $1";

    # need -laprutil befpre -lapr
    $config =~ m/^s,\@APU_LDLIBS\@,([^,]+)/m or
        die "Can't find apu libraries";
    $apr_libs = $1;
    $config =~ m/^s,\@APR_LDLIBS\@,([^,]+)/m or
        die "Can't find apr libraries";
    $apr_libs .= " $1";

    $config =~ m/^s,\@APREQ_LIBNAME\@,([^,]+)/m or
        die "Can't find apreq libname";
    $apreq_libname = $1;

    $config =~ m/^s,\@PACKAGE_VERSION\@,([^,]+)/m or
        die "Can't find package version";
    $version = $1;

}

my $apreq_libs = WIN32 ?
    qq{-L$base_dir/win32/libs -llib$apreq_libname } :
    qq{-L$src_dir/.libs -l$apreq_libname};

my $mp2_typemaps = Apache::Build->new->typemaps;

package My::ParseSource;
use base qw/ExtUtils::XSBuilder::ParseSource/;
use constant WIN32 => ($^O =~ /Win32/i);
my @dirs = ("$base_dir/src");
sub package {'Apache::libapreq2'}
sub unwanted_includes {[qw/apreq_tables.h apreq_config.h/]}

# ParseSource.pm v 0.23 bug: line 214 should read
# my @dirs = @{$self->include_dirs};
# for now, we override it here just to work around the bug

sub find_includes {
    my $self = shift;
    return $self->{includes} if $self->{includes};
    require File::Find;
    my(@dirs) = @{$self->include_dirs};
    unless (-d $dirs[0]) {
        die "could not find include directory";
    }
    # print "Will search @dirs for include files...\n" if ($verbose) ;
    my @includes;
    my $unwanted = join '|', @{$self -> unwanted_includes} ;

    for my $dir (@dirs) {
        File::Find::finddepth({
                               wanted => sub {
                                   return unless /\.h$/;
                                   return if ($unwanted && (/^($unwanted)/o));
                                   my $dir = $File::Find::dir;
                                   push @includes, "$dir/$_";
                               },
                               follow => not WIN32,
                              }, $dir);
    }
    return $self->{includes} = $self -> sort_includes (\@includes) ;
}

sub include_dirs {\@dirs}

package My::WrapXS;
use base qw/ExtUtils::XSBuilder::WrapXS/;
our $VERSION = $version;

##################################################
# Finally, we get to the actual script...

__PACKAGE__ -> run;

my @scripts = ();

use File::Spec::Functions qw(catfile);

File::Find::finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();
Apache::TestMM::generate_script("t/TEST");

ModPerl::MM::WriteMakefile(
    NAME => 'libapreq2',
    DIR => [qw(xs)],
    clean => { FILES => "xs t/logs t/TEST @scripts" },
    realclean => { FILES => "xsbuilder/tables" },
);

# That's the whole script - below is just a bunch of local overrides
##################################################

sub copy_docs {
    my @lines;
    my $dfs = '$(DIRFILESEP)';

    foreach my $file (@_) {
        my @dirs;
        $file =~ /(\w+\.pod)$/ or next;
        if ($1 eq "Error.pod" or $1 eq "Table.pod") {
            push @dirs, "Apache$dfs$_" for qw/Request Cookie Upload/;
            push @dirs, join $dfs, qw/Apache Cookie Jar/ if $1 eq "Error.pod";
        }
        else {
            push @dirs, "Apache";
        }
        push @lines, map <<EOT, @dirs;
subdirs :: \$(INST_LIBDIR)$dfs$_$dfs$1

\$(INST_LIBDIR)$dfs$_$dfs$1: $file
	\$(NOECHO) \$(MKPATH) \$(INST_LIBDIR)$dfs$_
	\$(CP) $file \$(INST_LIBDIR)$dfs$_$dfs$1

EOT
    }
    return join "", @lines;
}

sub test_docs {
    my ($pods, $tests) = @_;

    my $pod2test = Apache::TestConfig::which('pod2test');
    unless ($pod2test) {
        # try under the perl's bin
        require Config;
        my $bin = $Config::Config{bin};
        $pod2test = catfile $bin, "pod2test";
    }

    return "" unless $pod2test and -e $pod2test;

    return join "", map <<EOT, 0..$#$pods;
$$tests[$_]: $$pods[$_]
	\$(FULLPERLRUN) $pod2test $$pods[$_] $$tests[$_]

EOT
}

sub MY::postamble {
    my @docs = <docs/*.pod>;
    my @tests = @docs;
    s/pod$/t/ for @tests;

    return copy_docs(@docs) . test_docs(\@docs,\@tests) . <<EOT;
doc_test : @tests
	\$(FULLPERLRUN) "-Mblib" "-MTest::Harness" "-e" "runtests(\@ARGV)" @tests

test :: doc_test

EOT

}


sub parsesource_objects {[My::ParseSource->new]}
sub new_typemap {My::TypeMap->new(shift)}
sub h_filename_prefix {'apreq_xs_'}
sub my_xs_prefix {'apreq_xs_'}
sub xs_include_dir { $xs_dir }

sub mod_pod {
    my($self, $module, $complete) = @_;
    my $dirname = $self->class_dirname($module);
    my @parts = split '::', $module;
    my $mod_pod = "$dirname/$parts[-1]_pod";
    for ($self -> xs_incsrc_dir, @{ $self->{glue_dirs} }) {
        my $file = "$_/$mod_pod";
        $mod_pod = $file if $complete;
        print "mod_pod $mod_pod $file $complete\n" ;
        return $mod_pod if -e $file;
    }
    undef;
}

sub write_docs {
    my ($self, $module, $functions) = @_;
    my $podfile = $self->mod_pod($module, 1) or return;
    my $fh = $self->open_class_file($module, '.pod');
    open my $pod, "<", $podfile or die $!;
    while (<$pod>) {
        print $fh $_;
    }
}
sub pm_text {
    my($self, $module, $isa, $code) = @_;

    my $text = <<"EOF";
$self->{noedit_warning_hash}

package $module;
require DynaLoader ;

use strict;
use warnings FATAL => 'all';

use vars qw{\$VERSION \@ISA} ;
$isa
push \@ISA, 'DynaLoader' ;
\$VERSION = '$version';
bootstrap $module \$VERSION ;
EOF

    $text .= <<'EOF';

if ($ENV{MOD_PERL}) {
    require mod_perl;
    my $env = __PACKAGE__->env || '';
    if ($mod_perl::VERSION > 1.99) {
        die __PACKAGE__ . ": httpd must load mod_apreq.so first"
            if $env ne "Apache::RequestRec";
    }
    else {
        die "Unsupported mod_perl version number: $modperl::VERSION";
    }
}
EOF

    $text .= <<"EOF";

$code

1;
__END__
EOF

        return $text;
}
sub makefilepl_text {
    my($self, $class, $deps,$typemap) = @_;

    my @parts = split (/::/, $class) ;
    my $mmargspath = '../' x @parts ;
    $mmargspath .= 'mmargs.pl' ;

    my $txt = qq{
$self->{noedit_warning_hash}
use Apache2;
use ModPerl::MM;

local \$MMARGS ;

if (-f '$mmargspath')
    {
    do '$mmargspath' ;
    die \$\@ if (\$\@) ;
    }

\$MMARGS ||= {} ;


ModPerl::MM::WriteMakefile(
    'NAME'      => '$class',
    'VERSION'   => '$version',
    'TYPEMAPS'  => [qw(@$mp2_typemaps $typemap)],
    'INC'       => "-I$base_dir/glue/perl/xs -I$src_dir -I$xs_dir $apache_includes",
    'LIBS'      => "$apreq_libs $apr_libs",
} ;
$txt .= "'depend'  => $deps,\n" if ($deps) ;
$txt .= qq{    
    \%\$MMARGS,
);

} ;

}

# another bug in WrapXS.pm - 
# must insert a space before typemap definition

sub write_typemap
{
    my $self = shift;
    my $typemap = $self->typemap;
    my $map = $typemap->get;
    my %seen;

    my $fh = $self->open_class_file('', 'typemap');
    print $fh "$self->{noedit_warning_hash}\n";

    while (my($type, $t) = each %$map) {
        my $class = $t -> {class} ;
        $class ||= $type;
        next if $seen{$type}++ || $typemap->special($class);

        my $typemap = $t -> {typemapid} ;
        if ($class =~ /::/) {
            next if $seen{$class}++ ;
            $class =~ s/::$// ;
            print $fh "$class\t$typemap\n";
        }
        else {
            print $fh "$type\t$typemap\n";
        }
    }

    my $typemap_code = $typemap -> typemap_code ;

    foreach my $dir ('INPUT', 'OUTPUT') {
        print $fh "\n$dir\n" ;
        while (my($type, $code) = each %{$typemap_code}) {
            print $fh "$type\n\t$code->{$dir}\n\n" if ($code->{$dir}) ;
        }
    }

    close $fh;
}


package My::TypeMap;
use base 'ExtUtils::XSBuilder::TypeMap';

sub null_type {
    my($self, $type) = @_;
    my $t = $self->get->{$type};
    my $class = $t -> {class} ;

    if ($class =~ /APREQ_COOKIE_VERSION/) {
        return 'APREQ_COOKIE_VERSION_DEFAULT';
    }
    else {
        return $self->SUPER::null_type($type);
    }
}

# XXX this needs serious work
sub typemap_code
{
    {
        T_APREQ_COOKIE  => {
                            INPUT  => '$var = apreq_xs_sv2(cookie,$arg)',
                            perl2c => 'apreq_xs_sv2(cookie,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,"\${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
        T_APREQ_PARAM   => {
                            INPUT  => '$var = apreq_xs_sv2param($arg)',
                            perl2c => 'apreq_xs_sv2param(sv)',
                            OUTPUT => '$arg = apreq_xs_param2sv($var);',
                            c2perl => 'apreq_xs_param2sv(ptr)',
                           },
        T_APREQ_REQUEST => {
                            INPUT  => '$var = apreq_xs_sv2(request,$arg)',
                            perl2c => 'apreq_xs_sv2(request,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,\"${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
        T_APREQ_JAR     => {
                            INPUT  => '$var = apreq_xs_sv2(jar,$arg)',
                            perl2c => 'apreq_xs_sv2(jar,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,\"${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
 T_APREQ_COOKIE_VERSION => {
                            INPUT  => '$var = ((apreq_cookie_version_t)SvTRUE($arg))',
                            OUTPUT => '$arg = boolSV((bool)$var);',
                           },
        T_HASHOBJ       => {
                            INPUT => <<'EOT', # '$var = modperl_hash_tied_object(aTHX_ \"${ntype}\", $arg)'
    if (sv_derived_from($arg, \"${ntype}\")) {
        if (SVt_PVHV == SvTYPE(SvRV($arg))) {
            SV *hv = SvRV($arg);
            MAGIC *mg;
            if (SvMAGICAL(hv)) {
                if ((mg = mg_find(hv, PERL_MAGIC_tied))) {
                    $var = (void *)MgObjIV(mg);
                }
                else {
                    Perl_warn(aTHX_ \"Not a tied hash: (magic=%c)\", mg);
                    $var = NULL;
                }
            }
            else {
                Perl_warn(aTHX_ \"SV is not tied\");
                $var = NULL;
            }
        }
        else {
            $var = (void *)SvObjIV($arg);
        }
    }
    else {
        Perl_croak(aTHX_
                   \"argument is not a blessed reference \"
                   \"(expecting an %s derived object)\", \"${ntype}\");
    }
EOT

                            OUTPUT => <<'EOT', # '$arg = modperl_hash_tie(aTHX_ \"${ntype}\", $arg, $var);'
  {
    SV *hv = (SV*)newHV();
    SV *rsv = $arg;
    sv_setref_pv(rsv, \"${ntype}\", $var);
    sv_magic(hv, rsv, PERL_MAGIC_tied, Nullch, 0);
    $arg = SvREFCNT_inc(sv_bless(sv_2mortal(newRV_noinc(hv)),
                                 gv_stashpv(\"${ntype}\", TRUE)));
  }
EOT

                           },
    }
}
